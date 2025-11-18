//----------------------------------------------------------------------------------------------------
// JSGameLogicJob.cpp
// Phase 1: Async Architecture - JavaScript Worker Thread Job Implementation
//----------------------------------------------------------------------------------------------------

#include "Game/Framework/JSGameLogicJob.hpp"

#include "Engine/Script/IJSGameLogicContext.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Script/ScriptSubsystem.hpp"

// Suppress V8 header warnings (unreferenced formal parameters, etc.)
#pragma warning(push)
#pragma warning(disable: 4100)  // 'identifier': unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4324)  // 'structname': structure was padded due to alignment specifier
#include <v8.h>
#pragma warning(pop)

#include <thread>
#include <chrono>

//----------------------------------------------------------------------------------------------------
// Constructor
//
// Initializes synchronization primitives and dependencies.
//----------------------------------------------------------------------------------------------------
JSGameLogicJob::JSGameLogicJob(IJSGameLogicContext* context,
                               RenderCommandQueue*  commandQueue,
                               EntityStateBuffer*   entityBuffer)
    : m_context(context),
      m_commandQueue(commandQueue),
      m_entityBuffer(entityBuffer),
      m_frameRequested(false),
      m_frameComplete(true),   // Initially complete (ready for first frame)
      m_shutdownRequested(false),
      m_shutdownComplete(false),
      m_totalFrames(0),
      m_isolate(nullptr)
{
    // Validate dependencies
    if (!m_context)
    {
        ERROR_AND_DIE("JSGameLogicJob: IJSGameLogicContext pointer cannot be null");
    }
    if (!m_commandQueue)
    {
        ERROR_AND_DIE("JSGameLogicJob: RenderCommandQueue pointer cannot be null");
    }
    if (!m_entityBuffer)
    {
        ERROR_AND_DIE("JSGameLogicJob: EntityStateBuffer pointer cannot be null");
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, "JSGameLogicJob: Initialized (ready for worker thread execution)");
}

//----------------------------------------------------------------------------------------------------
// Destructor
//
// Ensures clean shutdown warning if not properly shut down.
//----------------------------------------------------------------------------------------------------
JSGameLogicJob::~JSGameLogicJob()
{
    // Warn if shutdown was not requested
    if (!m_shutdownComplete.load(std::memory_order_relaxed))
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                   "JSGameLogicJob: Destroyed without proper shutdown (call RequestShutdown() and wait for completion)");
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
               Stringf("JSGameLogicJob: Destroyed - Total frames executed: %llu",
                   m_totalFrames.load(std::memory_order_relaxed)));
}

//----------------------------------------------------------------------------------------------------
// Execute (Job Interface Override)
//
// Main worker thread execution loop.
// Runs continuously until shutdown requested.
//
// Algorithm:
//   1. Initialize V8 thread-local data
//   2. Loop:
//      a. Wait for frame trigger (conditional variable)
//      b. Execute JavaScript frame
//      c. Signal frame completion
//      d. Check shutdown flag
//   3. Clean up and exit
//
// Thread Safety:
//   - Executed by JobSystem worker thread
//   - All V8 API calls protected by v8::Locker
//   - Synchronization via std::mutex + std::condition_variable
//----------------------------------------------------------------------------------------------------
void JSGameLogicJob::Execute()
{
    DAEMON_LOG(LogScript, eLogVerbosity::Display, "JSGameLogicJob: Worker thread started");

    // Initialize V8 thread-local state
    InitializeWorkerThreadV8();

    // Main worker loop
    while (!m_shutdownRequested.load(std::memory_order_relaxed))
    {
        // Wait for frame trigger from main thread
        {
            std::unique_lock lock(m_mutex);
            m_frameStartCV.wait(lock, [this]()
            {
                return m_frameRequested.load(std::memory_order_relaxed) ||
                    m_shutdownRequested.load(std::memory_order_relaxed);
            });

            // Exit if shutdown requested
            if (m_shutdownRequested.load(std::memory_order_relaxed))
            {
                break;
            }

            // Clear frame request flag
            m_frameRequested.store(false, std::memory_order_relaxed);
            m_frameComplete.store(false, std::memory_order_relaxed);
        }

        // Execute JavaScript frame (outside lock to avoid blocking main thread)
        ExecuteJavaScriptFrame();

        // Signal frame completion
        {
            std::lock_guard lock(m_mutex);
            m_frameComplete.store(true, std::memory_order_release);
            m_frameCompleteCV.notify_one();
        }

        // Increment frame counter
        m_totalFrames.fetch_add(1, std::memory_order_relaxed);
    }

    // Signal shutdown complete
    m_shutdownComplete.store(true, std::memory_order_release);

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               Stringf("JSGameLogicJob: Worker thread exited - Total frames: %llu",
                   m_totalFrames.load(std::memory_order_relaxed)));
}

//----------------------------------------------------------------------------------------------------
// TriggerNextFrame (Main Thread API)
//
// Wakes worker thread to start next JavaScript frame execution.
//
// Precondition: Previous frame must be complete (check IsFrameComplete())
// Thread Safety: Call from main thread only
//----------------------------------------------------------------------------------------------------
void JSGameLogicJob::TriggerNextFrame()
{
    std::lock_guard lock(m_mutex);

    // Warn if triggering before previous frame complete (indicates timing issue)
    if (!m_frameComplete.load(std::memory_order_relaxed))
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                   "JSGameLogicJob: TriggerNextFrame() called before previous frame complete (frame skip)");
    }

    // Set frame request flag and wake worker thread
    m_frameRequested.store(true, std::memory_order_relaxed);
    m_frameStartCV.notify_one();
}

//----------------------------------------------------------------------------------------------------
// IsFrameComplete (Main Thread API)
//
// Checks if worker thread has completed current frame execution.
//
// Returns:
//   true  - JavaScript finished, safe to swap buffers
//   false - JavaScript still executing, continue with last state
//
// Thread Safety: Safe to call from main thread
//----------------------------------------------------------------------------------------------------
bool JSGameLogicJob::IsFrameComplete() const
{
    return m_frameComplete.load(std::memory_order_acquire);
}

//----------------------------------------------------------------------------------------------------
// RequestShutdown (Main Thread API)
//
// Signals worker thread to exit gracefully after completing current frame.
//
// Thread Safety: Safe to call from main thread
//----------------------------------------------------------------------------------------------------
void JSGameLogicJob::RequestShutdown()
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log, "JSGameLogicJob: Shutdown requested");

    {
        std::lock_guard lock(m_mutex);
        m_shutdownRequested.store(true, std::memory_order_relaxed);
        m_frameStartCV.notify_one();  // Wake worker if waiting
    }
}

//----------------------------------------------------------------------------------------------------
// IsShutdownComplete (Main Thread API)
//
// Checks if worker thread has completed shutdown.
//
// Returns:
//   true  - Worker thread exited, safe to delete job
//   false - Worker still running, wait before deletion
//
// Thread Safety: Safe to call from main thread
//----------------------------------------------------------------------------------------------------
bool JSGameLogicJob::IsShutdownComplete() const
{
    return m_shutdownComplete.load(std::memory_order_acquire);
}

//----------------------------------------------------------------------------------------------------
// ExecuteJavaScriptFrame (Worker Thread Implementation)
//
// Executes single JavaScript frame (update + render logic).
// Calls into Game::UpdateJS() and Game::RenderJS() methods.
//
// Thread Safety:
//   - Protected by v8::Locker (CRITICAL for multi-threaded V8 access)
//   - Writes to back entity buffer (safe, main thread reads front buffer)
//   - Submits render commands to queue (lock-free SPSC queue)
//
// Error Handling (Phase 1):
//   - No try-catch (errors propagate to worker thread termination)
//   - Phase 3: Add JavaScript exception handling and recovery
//----------------------------------------------------------------------------------------------------
void JSGameLogicJob::ExecuteJavaScriptFrame()
{
    // CRITICAL: Acquire V8 lock before ANY V8 API calls
    // Without this lock, multi-threaded V8 access will crash
    v8::Locker         locker(m_isolate);
    v8::Isolate::Scope isolateScope(m_isolate);

    // Phase 1: Simple delegation to Game::UpdateJS()
    // Game class handles JavaScript execution through ScriptSubsystem
    //
    // Future (Phase 2):
    //   - Direct JavaScript function calls for update/render
    //   - Entity state buffer updates
    //   - Render command submissions
    //
    // Future (Phase 3):
    //   - Try-catch for JavaScript exception handling
    //   - Error isolation (log error, signal frame complete, continue)
    //   - Timeout detection (main thread monitors IsFrameComplete())

    // Execute JavaScript update logic
    // This calls into JSEngine.update() through IJSGameLogicContext interface
    if (m_context)
    {
        // TODO Phase 1: Call m_context->UpdateJSWorkerThread() with worker thread context
        // For now, placeholder implementation (Phase 1 builds infrastructure only)

        // Example Phase 2 implementation:
        // float deltaTime = 0.0166f; // 60 FPS
        // m_context->UpdateJSWorkerThread(deltaTime, m_entityBuffer, m_commandQueue);

        // Placeholder: Log frame execution
        static uint64_t frameCount = 0;
        if (frameCount % 60 == 0)  // Log every 60 frames (≈1 second at 60 FPS)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Display,
                       Stringf("JSGameLogicJob: Executed frame #%llu on worker thread",
                           frameCount));
        }
        frameCount++;
    }
}

//----------------------------------------------------------------------------------------------------
// InitializeWorkerThreadV8 (Worker Thread Implementation)
//
// Initializes V8 thread-local data structures.
// Called once at worker thread startup.
//
// Thread Safety: Worker thread only, no locking needed
//----------------------------------------------------------------------------------------------------
void JSGameLogicJob::InitializeWorkerThreadV8()
{
    // Get V8 isolate from ScriptSubsystem (created on main thread)
    // In multi-threaded V8, the same isolate can be accessed from multiple threads
    // with proper v8::Locker protection
    m_isolate = g_scriptSubsystem->GetIsolate();

    if (!m_isolate)
    {
        ERROR_AND_DIE("JSGameLogicJob: Failed to get V8 isolate from ScriptSubsystem");
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
               "JSGameLogicJob: V8 isolate initialized for worker thread");
}

//----------------------------------------------------------------------------------------------------
// Implementation Notes
//
// V8 Thread Safety (CRITICAL):
//
//   v8::Locker Requirement:
//     - V8 isolates are single-threaded by default
//     - Multi-threaded access requires v8::Locker
//     - EVERY ExecuteJavaScriptFrame() call MUST acquire v8::Locker
//
//   Locking Pattern:
//     v8::Locker locker(isolate);              // Acquire V8 lock
//     v8::Isolate::Scope isolateScope(isolate);  // Enter isolate
//     // ... V8 API calls here (safe) ...
//
//   Failure Mode:
//     - Missing v8::Locker → Crash with "V8 isolate accessed from wrong thread"
//     - Detection: Thread Sanitizer (TSan) or V8 debug builds
//
// Frame Synchronization:
//
//   Main Thread Flow:
//     1. Check IsFrameComplete()
//     2. If complete: SwapBuffers(), TriggerNextFrame()
//     3. If not complete: Continue rendering with last state (frame skip tolerance)
//
//   Worker Thread Flow:
//     1. Wait for frame trigger (conditional variable)
//     2. Execute JavaScript frame
//     3. Signal frame completion
//     4. Repeat until shutdown
//
//   Benefits:
//     - Main thread never blocks (stable 60 FPS)
//     - Worker runs at variable speed (0-30ms)
//     - Frame skip tolerance (main uses last state if worker slow)
//
// Error Recovery Strategy (Phase 3):
//
//   JavaScript Exception:
//     - Catch in ExecuteJavaScriptFrame()
//     - Log error to console
//     - Signal frame complete (allow main thread to continue)
//     - Next frame: Retry execution (hot-reload may fix)
//
//   JavaScript Hang:
//     - Main thread: Detect timeout (IsFrameComplete() false for > 500ms)
//     - Action: Log error, continue rendering last state
//     - Recovery: Kill worker thread, restart with fresh isolate (Phase 3)
//
// Performance Profiling:
//
//   Metrics to Monitor:
//     - m_totalFrames: Frame execution count
//     - Frame duration: Time between TriggerNextFrame() and IsFrameComplete()
//     - Frame skip rate: Frames where IsFrameComplete() still false at next trigger
//
//   Optimization Targets (Phase 4):
//     - JavaScript frame duration: < 16.67ms for 60 FPS
//     - Frame skip rate: < 5% (acceptable for async architecture)
//     - Worker idle time: < 10% (efficient CPU utilization)
//
// Memory Safety:
//
//   Lifetimes:
//     - JSGameLogicJob: Created in App::Startup(), destroyed in App::Shutdown()
//     - m_game: Outlives JSGameLogicJob (guaranteed by App lifecycle)
//     - m_commandQueue: Outlives JSGameLogicJob (created before, destroyed after)
//     - m_entityBuffer: Outlives JSGameLogicJob (created before, destroyed after)
//
//   Dangling Pointer Prevention:
//     - Main thread: Calls RequestShutdown(), waits for IsShutdownComplete()
//     - Worker thread: Exits gracefully, no resource access after shutdown
//     - Destruction: Only after worker thread exited
//----------------------------------------------------------------------------------------------------
