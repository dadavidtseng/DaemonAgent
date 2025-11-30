//----------------------------------------------------------------------------------------------------
// App.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// IMPORTANT: Define NOMINMAX before any Windows headers to prevent min/max macro conflicts with V8
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "Game/Framework/App.hpp"
//----------------------------------------------------------------------------------------------------
#include "Game/Framework/GameCommon.hpp"
#include "Game/Framework/RenderResourceManager.hpp"
#include "Game/Framework/GameScriptInterface.hpp"
#include "Game/Gameplay/Game.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/CallbackQueue.hpp"
#include "Engine/Audio/AudioScriptInterface.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Engine/Script/ScriptSubsystem.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/ClockScriptInterface.hpp"
#include "Engine/Input/InputScriptInterface.hpp"
#include "Engine/Network/KADIScriptInterface.hpp"
#include "Engine/Renderer/DebugRenderSystemScriptInterface.hpp"
// Phase 1: Async Architecture Includes
#include "Engine/Entity/EntityStateBuffer.hpp"
#include "Engine/Renderer/CameraStateBuffer.hpp"
#include "Engine/Renderer/RenderCommandQueue.hpp"
#include "Game/Framework/JSGameLogicJob.hpp"

// Phase 2: High-Level Entity API Includes
// M4-T8: Direct API Usage (removed HighLevelEntityAPI facade)
#include "Engine/Entity/EntityAPI.hpp"
#include "Engine/Entity/EntityScriptInterface.hpp"
#include "Engine/Renderer/CameraAPI.hpp"

// M4-T8: Camera API Script Interface
#include "Engine/Renderer/CameraScriptInterface.hpp"

// Phase 2.4: CallbackQueue Script Interface (moved to Engine)
#include "Engine/Core/CallbackQueueScriptInterface.hpp"

// Phase 2: Geometry Creation Utilities
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/UI/ImGuiSubsystem.hpp"
#include "ThirdParty/json/json.hpp"


//----------------------------------------------------------------------------------------------------
App*  g_app  = nullptr;       // Created and owned by Main_Windows.cpp
Game* g_game = nullptr;       // Created and owned by the App

//----------------------------------------------------------------------------------------------------
App::App()
{
    GEngine::Get().Construct();
}

//----------------------------------------------------------------------------------------------------
App::~App()
{
    GEngine::Get().Destruct();
}

//----------------------------------------------------------------------------------------------------
void App::Startup()
{
    GEngine::Get().Startup();

    g_eventSystem->SubscribeEventCallbackFunction("OnCloseButtonClicked", OnCloseButtonClicked);
    g_eventSystem->SubscribeEventCallbackFunction("quit", OnCloseButtonClicked);

    // Phase 1: Initialize async architecture infrastructure BEFORE game initialization
    m_callbackQueue      = new CallbackQueue();
    m_renderCommandQueue = new RenderCommandQueue();
    m_entityStateBuffer  = new EntityStateBuffer();
    m_cameraStateBuffer  = new CameraStateBuffer();

    // Phase 5: Initialize render resource manager
    m_renderResourceManager = new RenderResourceManager();
    DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Startup - RenderResourceManager initialized (Phase 5)");

    // Phase 4.2: Enable per-key dirty tracking optimization for both buffers
    // This reduces SwapBuffers() cost from O(n) to O(d) where d = dirty entity count
    // Expected performance gain: 10-1000x speedup for sparse updates (typical gameplay)
    m_entityStateBuffer->EnableDirtyTracking(true);
    m_cameraStateBuffer->EnableDirtyTracking(true);
    DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Startup - Per-key dirty tracking enabled (Phase 4.2)");

    // M4-T8: Initialize EntityAPI and CameraAPI directly (removed HighLevelEntityAPI facade)
    m_entityAPI = new EntityAPI(m_renderCommandQueue, g_scriptSubsystem);
    m_cameraAPI = new CameraAPI(m_renderCommandQueue, g_scriptSubsystem, m_cameraStateBuffer);
    DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Startup - EntityAPI and CameraAPI initialized (M4-T8)");

    // Phase 3 FIX: Set camera-to-render transform to correct coordinate system rotation
    // The engine uses I-Forward/J-Left/K-Up (X-Forward/Y-Left/Z-Up)
    // But rendering expects a 90° CCW rotation around Z-axis to match screen orientation
    // This 90° CCW rotation swaps: X→Y, Y→-X (keeps Z unchanged)
    Mat44 cameraToRender;
    cameraToRender.m_values[Mat44::Ix] = 0.0f;  // New I-basis X component (was pointing X, now points Y)
    cameraToRender.m_values[Mat44::Iy] = 1.0f;  // New I-basis Y component
    cameraToRender.m_values[Mat44::Iz] = 0.0f;  // New I-basis Z component
    cameraToRender.m_values[Mat44::Iw] = 0.0f;

    cameraToRender.m_values[Mat44::Jx] = -1.0f;  // New J-basis X component (was pointing Y, now points -X)
    cameraToRender.m_values[Mat44::Jy] = 0.0f;  // New J-basis Y component
    cameraToRender.m_values[Mat44::Jz] = 0.0f;  // New J-basis Z component
    cameraToRender.m_values[Mat44::Jw] = 0.0f;

    cameraToRender.m_values[Mat44::Kx] = 0.0f;  // New K-basis X component (Z stays Z)
    cameraToRender.m_values[Mat44::Ky] = 0.0f;  // New K-basis Y component
    cameraToRender.m_values[Mat44::Kz] = 1.0f;  // New K-basis Z component
    cameraToRender.m_values[Mat44::Kw] = 0.0f;

    cameraToRender.m_values[Mat44::Tx] = 0.0f;  // No translation
    cameraToRender.m_values[Mat44::Ty] = 0.0f;
    cameraToRender.m_values[Mat44::Tz] = 0.0f;
    cameraToRender.m_values[Mat44::Tw] = 1.0f;

    DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Startup - Main camera initialized with 90° CCW Z-rotation camera-to-render transform");

    g_game = new Game();
    SetupScriptingBindings();
    g_game->PostInit();

    // Phase 1: Submit JavaScript worker thread job AFTER game and script initialization
    m_jsGameLogicJob = new JSGameLogicJob(g_game, m_renderCommandQueue, m_entityStateBuffer, m_callbackQueue);
    g_jobSystem->SubmitJob(m_jsGameLogicJob);

    DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Startup - Async architecture initialized (Phase 1)");
}

//----------------------------------------------------------------------------------------------------
// All Destroy and ShutDown process should be reverse order of the StartUp
//
void App::Shutdown()
{
    // Phase 1: Shutdown async architecture BEFORE game destruction
    // Order: Request shutdown → Wait for worker → Retrieve from JobSystem → Cleanup resources
    if (m_jsGameLogicJob)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Shutdown - Requesting worker thread shutdown...");
        m_jsGameLogicJob->RequestShutdown();

        // Wait for worker thread to complete (max 5 seconds timeout)
        int waitCount = 0;
        while (!m_jsGameLogicJob->IsShutdownComplete() && waitCount < 500)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++waitCount;
        }

        if (m_jsGameLogicJob->IsShutdownComplete())
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Shutdown - Worker thread exited successfully");
        }
        else
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Warning, "App::Shutdown - Worker thread shutdown timeout!");
        }

        // CRITICAL: Retrieve job from JobSystem before manual deletion
        // This prevents double-delete when JobSystem::Shutdown() tries to clean up
        // The job is in m_completedJobs queue, so retrieve it first
        Job* retrievedJob = g_jobSystem->RetrieveCompletedJob();
        while (retrievedJob != nullptr && retrievedJob != m_jsGameLogicJob)
        {
            // If there are other completed jobs, delete them first
            delete retrievedJob;
            retrievedJob = g_jobSystem->RetrieveCompletedJob();
        }

        // Now safe to delete our job (it's no longer tracked by JobSystem)
        delete m_jsGameLogicJob;
        m_jsGameLogicJob = nullptr;
    }

    // Phase 2: Clear V8::Persistent callbacks BEFORE V8 isolate destruction
    if (m_kadiScriptInterface)
    {
        m_kadiScriptInterface->ClearCallbacks();
    }

    GAME_SAFE_RELEASE(g_game);

    // M4-T8: Cleanup EntityAPI and CameraAPI BEFORE async infrastructure
    if (m_entityAPI)
    {
        delete m_entityAPI;
        m_entityAPI = nullptr;
        DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Shutdown - EntityAPI destroyed (M4-T8)");
    }

    if (m_cameraAPI)
    {
        delete m_cameraAPI;
        m_cameraAPI = nullptr;
        DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Shutdown - CameraAPI destroyed (M4-T8)");
    }

    // Phase 5: Cleanup RenderResourceManager BEFORE state buffers
    if (m_renderResourceManager)
    {
        delete m_renderResourceManager;
        m_renderResourceManager = nullptr;
        DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Shutdown - RenderResourceManager destroyed (Phase 5)");
    }

    if (m_entityStateBuffer)
    {
        delete m_entityStateBuffer;
        m_entityStateBuffer = nullptr;
    }

    if (m_cameraStateBuffer)
    {
        delete m_cameraStateBuffer;
        m_cameraStateBuffer = nullptr;
    }

    if (m_renderCommandQueue)
    {
        delete m_renderCommandQueue;
        m_renderCommandQueue = nullptr;
    }
	if (m_callbackQueue)
	{
		delete m_callbackQueue;
		m_callbackQueue = nullptr;
	}

    GEngine::Get().Shutdown();
}

//----------------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
//
void App::RunFrame()
{
    BeginFrame();   // Engine pre-frame stuff
    Update();       // Game updates / moves / spawns / hurts / kills stuff
    Render();       // Game draws current state of things
    EndFrame();     // Engine post-frame stuff
}

//----------------------------------------------------------------------------------------------------
void App::RunMainLoop()
{
    // Program main loop; keep running frames until it's time to quit
    while (!m_isQuitting)
    {
        // Sleep(16); // Temporary code to "slow down" our app to ~60Hz until we have proper frame timing in
        RunFrame();
    }
}

STATIC bool App::m_isQuitting = false;

//----------------------------------------------------------------------------------------------------
STATIC bool App::OnCloseButtonClicked(EventArgs& args)
{
    UNUSED(args)

    RequestQuit();

    return true;
}

//----------------------------------------------------------------------------------------------------
STATIC void App::RequestQuit()
{
    m_isQuitting = true;
}

//----------------------------------------------------------------------------------------------------
void App::BeginFrame() const
{
    g_eventSystem->BeginFrame();
    g_window->BeginFrame();
    g_renderer->BeginFrame();
    DebugRenderBeginFrame();
    g_devConsole->BeginFrame();
    g_input->BeginFrame();
    g_audio->BeginFrame();
    g_kadiSubsystem->BeginFrame();
}

//----------------------------------------------------------------------------------------------------
void App::Update()
{
    Clock::TickSystemClock();
    UpdateCursorMode();

    g_imgui->Update();

    // Process pending hot-reload events on main thread (V8-safe)
    g_scriptSubsystem->Update();

    // Phase 1: Async Frame Synchronization
    // Check if worker thread completed previous JavaScript frame
    if (m_jsGameLogicJob && m_jsGameLogicJob->IsFrameComplete())
    {
        // Swap entity state buffers (copy back buffer → front buffer)
        if (m_entityStateBuffer != nullptr) m_entityStateBuffer->SwapBuffers();

        // Swap camera state buffers (copy back buffer → front buffer)
        if (m_cameraStateBuffer != nullptr) m_cameraStateBuffer->SwapBuffers();

        // Trigger next JavaScript frame on worker thread
        m_jsGameLogicJob->TriggerNextFrame();
    }
    else if (m_jsGameLogicJob)
    {
        // Frame skip: Worker still executing, continue with last state
        // This maintains stable 60 FPS rendering regardless of JavaScript performance
        static uint64_t frameSkipCount = 0;
        if (frameSkipCount % 60 == 0)  // Log every 60 frame skips
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                       Stringf("App::Update - JavaScript frame skip (worker still executing) - Total skips: %llu",
                           frameSkipCount));
        }
        ++frameSkipCount;
    }

    // Phase 1: Process render commands from queue (placeholder implementation)
    // Phase 2 will implement actual command processing
    ProcessRenderCommands();
    

    // Phase 2.3: Async JavaScript execution on worker thread
    // Main thread triggers worker, continues rendering independently (stable 60 FPS)
    if (m_jsGameLogicJob)
    {
        if (m_jsGameLogicJob->IsFrameComplete())
        {
            // Worker finished previous frame - safe to swap buffers and trigger next frame
            if (m_entityStateBuffer)
            {
                m_entityStateBuffer->SwapBuffers();  // Swap entity state (main reads front, worker writes back)
            }
            
            // Trigger next JavaScript frame on worker thread (non-blocking)
            m_jsGameLogicJob->TriggerNextFrame();
        }
        else
        {
            // Worker still executing previous frame - frame skip tolerance
            // Continue rendering with last known state (maintains stable 60 FPS)
            static uint64_t skipCount = 0;
            if ((skipCount % 60) == 0)  // Log every 60 skips (~1 second at 60 FPS)
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                           Stringf("App::Update - Worker frame skip detected (total: %llu)", skipCount));
            }
            skipCount++;
        }
    }

    // Phase 2.4: Callbacks now processed on JavaScript worker thread via CallbackQueue.dequeueAll()
    // C++ enqueues callbacks to CallbackQueue, JavaScript dequeues and executes them
    // This achieves true async bidirectional communication between C++ and JavaScript
    if (m_entityAPI)
    {
        m_entityAPI->ExecutePendingCallbacks(m_callbackQueue);
    }
    if (m_cameraAPI)
    {
        m_cameraAPI->ExecutePendingCallbacks(m_callbackQueue);
    }
}

//----------------------------------------------------------------------------------------------------
// Some simple OpenGL example drawing code.
// This is the graphical equivalent of printing "Hello, world."
//
// Ultimately this function (App::Render) will only call methods on Renderer (like Renderer::DrawVertexArray)
//	to draw things, never calling OpenGL (nor DirectX) functions directly.
//
void App::Render() const
{
    Rgba8 const clearColor = Rgba8::GREY;

    g_renderer->ClearScreen(clearColor, Rgba8::BLACK);

    // Phase 2: Render all entities from EntityStateBuffer
    // Only render entities in GAME mode, not in ATTRACT mode
    if (g_game && !g_game->IsAttractMode())
    {
        RenderEntities();
    }

    //========================================
    // Debug Rendering (World + Screen)
    //========================================
    // Get world camera (active camera - typically the player camera)
    Camera const* worldCamera = nullptr;
    if (m_cameraStateBuffer)
    {
        EntityID activeCameraId = m_cameraStateBuffer->GetActiveCameraID();
        if (activeCameraId != 0)
        {
            worldCamera = m_cameraStateBuffer->GetCameraById(activeCameraId);
        }
    }

    // Render 3D debug visualization (world space)
    // Only render world debug objects in GAME mode, not in ATTRACT mode
    if (worldCamera && g_game && !g_game->IsAttractMode())
    {
        DebugRenderWorld(*worldCamera);
    }

    // Get screen camera (orthographic 2D camera for UI text)
    // Note: Screen camera is created by JSGame.js but not set as active
    // We need to find it by iterating through cameras or store its ID separately
    Camera const* screenCamera = nullptr;
    if (m_cameraStateBuffer)
    {
        // TEMPORARY: Find screen camera by checking all cameras for orthographic mode
        // TODO: Store screen camera ID separately for direct lookup
        CameraStateMap const* frontBuffer = m_cameraStateBuffer->GetFrontBuffer();
        if (frontBuffer)
        {
            for (auto const& [cameraId, cameraState] : *frontBuffer)
            {
                if (cameraState.type == "screen")
                {
                    screenCamera = m_cameraStateBuffer->GetCameraById(cameraId);
                    break;  // Found the screen camera
                }
            }
        }
    }

    // Render 2D debug visualization (screen space text/UI)
    if (screenCamera)
    {
        DebugRenderScreen(*screenCamera);
    }

    AABB2 const box = AABB2(Vec2::ZERO, Vec2(1600.f, 30.f));

    g_devConsole->Render(box);
    g_imgui->Render();
}

//----------------------------------------------------------------------------------------------------
void App::EndFrame() const
{
    g_eventSystem->EndFrame();
    g_window->EndFrame();
    g_renderer->EndFrame();
    DebugRenderEndFrame();
    g_devConsole->EndFrame();
    g_input->EndFrame();
    g_audio->EndFrame();
    g_kadiSubsystem->EndFrame();
}

//----------------------------------------------------------------------------------------------------
STATIC std::any App::OnPrint(std::vector<std::any> const& args)
{
    if (!args.empty())
    {
        try
        {
            std::string message = std::any_cast<std::string>(args[0]);
            DebuggerPrintf("JS: %s\n", message.c_str());

            if (g_devConsole)
            {
                g_devConsole->AddLine(DevConsole::INFO_MINOR, "JS: " + message);
            }
        }
        catch (std::bad_any_cast const&)
        {
            DebuggerPrintf("JS: [無法轉換的物件]\n");
        }
    }
    return std::any{};
}

std::any App::OnDebug(std::vector<std::any> const& args)
{
    if (!args.empty())
    {
        try
        {
            std::string message = std::any_cast<std::string>(args[0]);
            DebuggerPrintf("JS DEBUG: %s\n", message.c_str());
        }
        catch (std::bad_any_cast const&)
        {
            DebuggerPrintf("JS DEBUG: [無法轉換的物件]\n");
        }
    }
    return std::any{};
}

std::any App::OnGarbageCollection(std::vector<std::any> const& args)
{
    UNUSED(args)
    if (g_scriptSubsystem)
    {
        g_scriptSubsystem->ForceGarbageCollection();
        DebuggerPrintf("JS: 垃圾回收已執行\n");
    }
    return std::any{};
}

//----------------------------------------------------------------------------------------------------
void App::UpdateCursorMode()
{
    bool const doesWindowHasFocus = GetActiveWindow() == g_window->GetWindowHandle();
    // bool const shouldUsePointerMode = !doesWindowHasFocus || g_devConsole->IsOpen() || g_game->IsAttractMode();
    bool const shouldUsePointerMode = !doesWindowHasFocus || g_devConsole->IsOpen();

    if (shouldUsePointerMode == true)
    {
        g_input->SetCursorMode(eCursorMode::POINTER);
    }
    else
    {
        g_input->SetCursorMode(eCursorMode::FPS);
    }
}

void App::SetupScriptingBindings()
{
    if (g_scriptSubsystem == nullptr)ERROR_AND_DIE(StringFormat("(App::SetupScriptingBindings)(g_scriptSubsystem is nullptr!"))
    if (!g_scriptSubsystem->IsInitialized())ERROR_AND_DIE(StringFormat("(App::SetupScriptingBindings)(g_scriptSubsystem is not initialized!"))
    if (g_game == nullptr)ERROR_AND_DIE(StringFormat("(App::SetupScriptingBindings)(g_game is nullptr"))

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(App::SetupScriptingBindings)(start)"));

    // Initialize hot-reload system (now integrated into ScriptSubsystem)
    std::string projectRoot = "../";

    if (g_scriptSubsystem->InitializeHotReload(projectRoot))
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(App::SetupScriptingBindings) Hot-reload system initialized successfully"));
    }
    else
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("(App::SetupScriptingBindings) Hot-reload system initialization failed"));
    }

    m_gameScriptInterface = std::make_shared<GameScriptInterface>(g_game);
    g_scriptSubsystem->RegisterScriptableObject("game", m_gameScriptInterface);

    m_inputScriptInterface = std::make_shared<InputScriptInterface>(g_input);
    g_scriptSubsystem->RegisterScriptableObject("input", m_inputScriptInterface);

    m_audioScriptInterface = std::make_shared<AudioScriptInterface>(g_audio);
    g_scriptSubsystem->RegisterScriptableObject("audio", m_audioScriptInterface);

    m_debugRenderSystemScriptInterface = std::make_shared<DebugRenderSystemScriptInterface>();
    g_scriptSubsystem->RegisterScriptableObject("debugRenderInterface", m_debugRenderSystemScriptInterface);

    m_clockScriptInterface = std::make_shared<ClockScriptInterface>();
    g_scriptSubsystem->RegisterScriptableObject("clock", m_clockScriptInterface);

    // M4-T8: Register entity and camera script interfaces (API splitting)
    if (m_entityAPI && m_cameraAPI)
    {
        // M4-T8: Register EntityScriptInterface as "entity" global (direct API usage)
        m_entityScriptInterface = std::make_shared<EntityScriptInterface>(m_entityAPI);
        g_scriptSubsystem->RegisterScriptableObject("entity", m_entityScriptInterface);

        // M4-T8: Register CameraScriptInterface as "camera" global (direct API usage)
        m_cameraScriptInterface = std::make_shared<CameraScriptInterface>(m_cameraAPI);
        g_scriptSubsystem->RegisterScriptableObject("camera", m_cameraScriptInterface);

        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(App::SetupScriptingBindings) Entity and Camera script interfaces registered (M4-T8)"));
    }

    // Register KADI broker integration for distributed agent communication
    if (g_kadiSubsystem)
    {
        m_kadiScriptInterface = std::make_shared<KADIScriptInterface>(g_kadiSubsystem);

        // Phase 2: Pass V8 isolate to KADI interface for callback invocation
        m_kadiScriptInterface->SetV8Isolate(g_scriptSubsystem->GetIsolate());

        g_scriptSubsystem->RegisterScriptableObject("kadi", m_kadiScriptInterface);
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(App::SetupScriptingBindings) KADI script interface registered with V8 isolate"));
    }
    else
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("(App::SetupScriptingBindings) KADI subsystem not available, skipping registration"));
    }

    // Phase 2.4: Register CallbackQueue script interface for JavaScript callback dequeuing
    if (m_callbackQueue)
    {
        m_callbackQueueScriptInterface = std::make_shared<CallbackQueueScriptInterface>(m_callbackQueue);
        g_scriptSubsystem->RegisterScriptableObject("callbackQueue", m_callbackQueueScriptInterface);
        DAEMON_LOG(LogScript, eLogVerbosity::Log, "App::SetupScriptingBindings - CallbackQueue script interface registered (Phase 2.4)");
    }

    g_scriptSubsystem->RegisterGlobalFunction("print", OnPrint);
    g_scriptSubsystem->RegisterGlobalFunction("debug", OnDebug);
    g_scriptSubsystem->RegisterGlobalFunction("gc", OnGarbageCollection);

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(App::SetupScriptingBindings)(end)"));
}

//----------------------------------------------------------------------------------------------------
// ProcessRenderCommands (Phase 1: Placeholder Implementation)
//
// Consumes render commands from lock-free queue and processes them.
//
// Phase 1: Placeholder - only logs commands, no actual processing
// Phase 2: Implement actual command processing (CREATE_MESH, UPDATE_ENTITY, etc.)
//----------------------------------------------------------------------------------------------------
void App::ProcessRenderCommands()
{
    if (!m_renderCommandQueue || !m_entityStateBuffer || !m_cameraStateBuffer || !m_entityAPI || !m_cameraAPI)
    {
        return;
    }

    static int s_commandCount = 0;

    // Process all commands from queue
    m_renderCommandQueue->ConsumeAll([this](RenderCommand const& cmd)
    {
        s_commandCount++;
        // DebuggerPrintf("[TRACE] ProcessRenderCommands - Processing command #%d, type=%d, entityId=%llu\n",
        //                s_commandCount, static_cast<int>(cmd.type), cmd.entityId);

        switch (cmd.type)
        {
        case RenderCommandType::CREATE_MESH:
            {
                MeshCreationData const& meshData = std::get<MeshCreationData>(cmd.data);
                // DebuggerPrintf("[TRACE] ProcessRenderCommands - CREATE_MESH: meshType=%s, pos=(%.1f,%.1f,%.1f), radius=%.1f\n",
                //                meshData.meshType.c_str(), meshData.position.x, meshData.position.y, meshData.position.z, meshData.radius);

                // Phase 5: Use RenderResourceManager instead of CreateGeometryForMeshType
                int vbHandle = m_renderResourceManager->RegisterEntity(cmd.entityId, meshData.meshType, meshData.radius, meshData.color);

                if (vbHandle != 0)
                {
                    EntityState state;
                    state.position           = meshData.position;
                    state.orientation        = EulerAngles::ZERO;
                    state.color              = meshData.color;
                    state.radius             = meshData.radius;
                    state.meshType           = meshData.meshType;
                    state.isActive           = true;
                    // Phase 5: Removed vertexBufferHandle from EntityState (moved to RenderResourceManager)
                    state.cameraType         = "world";  // Phase 2: All mesh entities use world camera by default

                    auto* backBuffer            = m_entityStateBuffer->GetBackBuffer();
                    (*backBuffer)[cmd.entityId] = state;

                    // Phase 4.2: Mark entity as dirty for per-key copy optimization
                    m_entityStateBuffer->MarkDirty(cmd.entityId);

                    // DebuggerPrintf("[TRACE] ProcessRenderCommands - Entity %llu added to back buffer (vbHandle=%d, vertCount=%zu)\n",
                    //                cmd.entityId, vbHandle, backBuffer->size());
                }
                else
                {
                    // DebuggerPrintf("[TRACE] ProcessRenderCommands - ERROR: CreateGeometryForMeshType returned 0 (failed)!\n");
                }
                break;
            }

        case RenderCommandType::UPDATE_ENTITY:
            {
                EntityUpdateData const& updateData = std::get<EntityUpdateData>(cmd.data);
                auto*                   backBuffer = m_entityStateBuffer->GetBackBuffer();
                auto                    it         = backBuffer->find(cmd.entityId);

                if (it != backBuffer->end())
                {
                    if (updateData.position.has_value()) it->second.position = updateData.position.value();
                    if (updateData.orientation.has_value()) it->second.orientation = updateData.orientation.value();
                    if (updateData.color.has_value()) it->second.color = updateData.color.value();

                    // Phase 4.2: Mark entity as dirty for per-key copy optimization
                    m_entityStateBuffer->MarkDirty(cmd.entityId);
                }
                break;
            }

        case RenderCommandType::DESTROY_ENTITY:
            {
                std::unordered_map<unsigned long long, EntityState>* backBuffer = m_entityStateBuffer->GetBackBuffer();
                auto                                                 it         = backBuffer->find(cmd.entityId);
                if (it != backBuffer->end()) it->second.isActive = false;
                break;
            }

        case RenderCommandType::CREATE_CAMERA:
            {
                auto const& cameraData = std::get<CameraCreationData>(cmd.data);
                // DebuggerPrintf("[TRACE] ProcessRenderCommands - CREATE_CAMERA: cameraId=%llu, pos=(%.1f,%.1f,%.1f), type=%s\n",
                //                cmd.entityId, cameraData.position.x, cameraData.position.y, cameraData.position.z, cameraData.type.c_str());

                CameraState state;
                state.position    = cameraData.position;
                state.orientation = cameraData.orientation;
                state.type        = cameraData.type;
                state.isActive    = true;

                // Auto-configure camera mode based on type
                if (cameraData.type == "world")
                {
                    state.mode              = Camera::eMode_Perspective;
                    state.perspectiveFOV    = 60.0f;
                    state.perspectiveAspect = 16.0f / 9.0f;
                    state.perspectiveNear   = 0.1f;
                    state.perspectiveFar    = 100.0f;
                }
                else if (cameraData.type == "screen")
                {
                    // Get viewport dimensions for screen camera orthographic bounds
                    // Use viewport dimensions (not client dimensions) to match actual rendering area
                    Vec2 viewportDimensions = Vec2(1600.f, 800.f);  // Default fallback
                    if (Window::s_mainWindow)
                    {
                        viewportDimensions = Window::s_mainWindow->GetViewportDimensions();
                    }

                    state.mode        = Camera::eMode_Orthographic;
                    state.orthoLeft   = 0.0f;
                    state.orthoBottom = 0.0f;
                    state.orthoRight  = viewportDimensions.x;  // Match viewport width
                    state.orthoTop    = viewportDimensions.y;    // Match viewport height
                    state.orthoNear   = 0.0f;
                    state.orthoFar    = 1.0f;
                    state.viewport    = AABB2(Vec2::ZERO, Vec2::ONE);  // Full screen viewport for UI overlay

                    // DIAGNOSTIC: Log screen camera orthographic bounds
                    DAEMON_LOG(LogScript, eLogVerbosity::Display,
                               StringFormat("[DIAGNOSTIC] CREATE_CAMERA screen: ortho bounds = ({:.2f}, {:.2f}) to ({:.2f}, {:.2f}), viewport dims = ({:.2f}, {:.2f})",
                                   state.orthoLeft, state.orthoBottom, state.orthoRight, state.orthoTop,
                                   viewportDimensions.x, viewportDimensions.y));
                }

                auto* backBuffer            = m_cameraStateBuffer->GetBackBuffer();
                (*backBuffer)[cmd.entityId] = state;

                // Phase 4.2: Mark camera as dirty for per-key copy optimization
                m_cameraStateBuffer->MarkDirty(cmd.entityId);

                // Phase 2.3 FIX: Notify CameraAPI that camera creation is complete
                // This marks the callback as ready so it can be enqueued and executed
                if (m_cameraAPI && cameraData.callbackId != 0)
                {
                    m_cameraAPI->NotifyCallbackReady(cameraData.callbackId, cmd.entityId);
                }

                // DebuggerPrintf("[TRACE] ProcessRenderCommands - Camera %llu added to back buffer\n", cmd.entityId);
                break;
            }

        case RenderCommandType::UPDATE_CAMERA:
            {
                auto const& updateData = std::get<CameraUpdateData>(cmd.data);
                auto*       backBuffer = m_cameraStateBuffer->GetBackBuffer();
                auto        it         = backBuffer->find(cmd.entityId);

                if (it != backBuffer->end())
                {
                    // OPTION A FIX: Always apply both position and orientation
                    // HighLevelEntityAPI now reads the current state and sends complete updates
                    // So we can safely apply both fields without zeroing anything out

                    it->second.position    = updateData.position;
                    it->second.orientation = updateData.orientation;

                    // Phase 4.2: Mark camera as dirty for per-key copy optimization
                    m_cameraStateBuffer->MarkDirty(cmd.entityId);

                    // DebuggerPrintf("[TRACE] ProcessRenderCommands - UPDATE_CAMERA: cameraId=%llu updated\n", cmd.entityId);
                }
                else
                {
                    // Camera not found in back buffer!
                    DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                               StringFormat("ProcessRenderCommands UPDATE_CAMERA: Camera {} NOT FOUND in back buffer!", cmd.entityId));
                }
                break;
            }

        case RenderCommandType::SET_ACTIVE_CAMERA:
            {
                m_cameraStateBuffer->SetActiveCameraID(cmd.entityId);
                // DebuggerPrintf("[TRACE] ProcessRenderCommands - SET_ACTIVE_CAMERA: cameraId=%llu\n", cmd.entityId);
                break;
            }

        case RenderCommandType::UPDATE_CAMERA_TYPE:
            {
                auto const& typeData   = std::get<CameraTypeUpdateData>(cmd.data);
                auto*       backBuffer = m_cameraStateBuffer->GetBackBuffer();
                auto        it         = backBuffer->find(cmd.entityId);

                if (it != backBuffer->end())
                {
                    it->second.type = typeData.type;

                    // Reconfigure camera based on new type
                    if (typeData.type == "world")
                    {
                        it->second.mode              = Camera::eMode_Perspective;
                        it->second.perspectiveFOV    = 60.0f;
                        it->second.perspectiveAspect = 16.0f / 9.0f;
                        it->second.perspectiveNear   = 0.1f;
                        it->second.perspectiveFar    = 100.0f;
                    }
                    else if (typeData.type == "screen")
                    {
                        // Get actual client dimensions for screen camera orthographic bounds
                        Vec2 clientDimensions = Vec2(1600.f, 800.f);  // Default fallback
                        if (Window::s_mainWindow)
                        {
                            clientDimensions = Window::s_mainWindow->GetClientDimensions();
                        }

                        it->second.mode        = Camera::eMode_Orthographic;
                        it->second.orthoLeft   = 0.0f;
                        it->second.orthoBottom = 0.0f;
                        it->second.orthoRight  = clientDimensions.x;  // Match window width
                        it->second.orthoTop    = clientDimensions.y;    // Match window height
                        it->second.orthoNear   = 0.0f;
                        it->second.orthoFar    = 1.0f;
                        it->second.viewport    = AABB2(Vec2::ZERO, Vec2::ONE);  // Full screen viewport for UI overlay
                    }
                    // DebuggerPrintf("[TRACE] ProcessRenderCommands - UPDATE_CAMERA_TYPE: cameraId=%llu, type=%s\n",
                    //                cmd.entityId, typeData.type.c_str());
                }
                break;
            }

        case RenderCommandType::DESTROY_CAMERA:
            {
                auto* backBuffer = m_cameraStateBuffer->GetBackBuffer();
                auto  it         = backBuffer->find(cmd.entityId);
                if (it != backBuffer->end())
                {
                    it->second.isActive = false;
                    // DebuggerPrintf("[TRACE] ProcessRenderCommands - DESTROY_CAMERA: cameraId=%llu\n", cmd.entityId);
                }
                break;
            }

        case RenderCommandType::CREATE_LIGHT:
            break;
        case RenderCommandType::UPDATE_LIGHT:
            break;
        default:
            break;
        }
    });
}

//----------------------------------------------------------------------------------------------------
// RenderEntities (Phase 2 Entity Rendering)
//
// Renders all active entities from EntityStateBuffer.
// Reads front buffer (thread-safe, no locking) and draws each entity using stored vertex buffers.
//----------------------------------------------------------------------------------------------------
void App::RenderEntities() const
{
    // if (!m_entityStateBuffer || !m_mainCamera)
    // {
    //     return;
    // }

    // Get front buffer for reading (thread-safe, no locking)
    EntityStateMap const* frontBuffer = m_entityStateBuffer->GetFrontBuffer();
    if (!frontBuffer)
    {
        return;
    }

    static int s_frameCount = 0;
    s_frameCount++;


    //========================================
    // BLOCK 1: World Camera (3D Entities)
    //========================================
    // Phase 2b: NEW Camera System - Use CameraStateBuffer for active camera lookup

    Camera const* worldCamera = nullptr;
    if (m_cameraStateBuffer)
    {
        // Get active camera from NEW camera system
        EntityID activeCameraId = m_cameraStateBuffer->GetActiveCameraID();
        if (activeCameraId != 0)
        {
            worldCamera = m_cameraStateBuffer->GetCameraById(activeCameraId);
        }
    }


    // Phase 2.3 Fix: Check if worldCamera is valid before rendering
    // Camera creation is async, so it may not be ready on first few frames
    if (!worldCamera)
    {
        // Skip world entity rendering until camera is ready
        // This is normal during startup - camera creation happens asynchronously
        return;
    }
    
    g_renderer->BeginCamera(*worldCamera);

    int worldRenderedCount = 0;

    // Render all entities with cameraType == "world" (3D entities)
    for (auto const& [entityId, state] : *frontBuffer)
    {
        // Skip inactive entities
        if (!state.isActive) continue;

        // Skip entities not tagged for world camera
        if (state.cameraType != "world") continue;

        // Phase 5: Query vertex data from RenderResourceManager instead of EntityState
        VertexList_PCU const* verts = m_renderResourceManager->GetVerticesForEntity(entityId);
        if (!verts || verts->empty()) continue;

        // Set model transformation matrix
        // Coordinate System: X-forward, Y-left, Z-up
        // FIXED: Use same pattern as Entity::GetModelToWorldTransform()
        // SetTranslation + Append(orientation matrix) instead of separate axis rotations
        Mat44 modelMatrix;
        modelMatrix.SetTranslation3D(state.position);
        modelMatrix.Append(state.orientation.GetAsMatrix_IFwd_JLeft_KUp());

        g_renderer->SetModelConstants(modelMatrix, state.color);
        g_renderer->BindTexture(nullptr);  // Phase 2.5: Will be replaced with per-entity texture binding

        // Draw entity geometry
        g_renderer->DrawVertexArray(static_cast<int>(verts->size()), verts->data());
        worldRenderedCount++;
    }

    g_renderer->EndCamera(*worldCamera);

    //========================================
    // BLOCK 2: Screen Camera (2D UI/Attract Mode)
    //========================================
    // Phase 2b: Screen camera rendering temporarily disabled during NEW camera system migration
    // TODO: Implement multi-camera support or separate screen camera ID in CameraStateBuffer

    /*
    // OLD camera system code - commented out during Phase 2b migration
    Camera* screenCamera = m_cameraScriptInterface->GetCameraByRole("screen");
    if (screenCamera)
    {
        g_renderer->BeginCamera(*screenCamera);

        int screenRenderedCount = 0;

        // Render all entities with cameraType == "screen" (2D UI entities)
        for (auto const& [entityId, state] : *frontBuffer)
        {
            // Skip inactive entities
            if (!state.isActive) continue;

            // Skip entities not tagged for screen camera
            if (state.cameraType != "screen") continue;

            // Skip entities without valid vertex buffer handles
            if (state.vertexBufferHandle == 0) continue;

            // Find vertex data in global vertex buffer storage
            auto it = g_vertexBuffers.find(state.vertexBufferHandle);
            if (it == g_vertexBuffers.end()) continue;

            VertexList_PCU const& verts = it->second;
            if (verts.empty()) continue;

            // Set model transformation matrix (2D UI typically has simpler transforms)
            Mat44 modelMatrix = Mat44::MakeTranslation3D(state.position);
            modelMatrix.Append(Mat44::MakeZRotationDegrees(state.orientation.m_yawDegrees));

            g_renderer->SetModelConstants(modelMatrix, state.color);
            g_renderer->BindTexture(nullptr);  // Phase 2.5: Will be replaced with per-entity texture binding

            // Draw UI geometry
            g_renderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
            screenRenderedCount++;
        }

        g_renderer->EndCamera(*screenCamera);

        // Log screen render count periodically
        if (s_frameCount % 60 == 0 && screenRenderedCount > 0)
        {
            DebuggerPrintf("[TRACE] RenderEntities - Rendered %d screen entities this frame\n", screenRenderedCount);
        }
    }
    */
}
