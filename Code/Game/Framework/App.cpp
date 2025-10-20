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
#include "Game/Framework/GameScriptInterface.hpp"
#include "Game/Gameplay/Game.hpp"
//----------------------------------------------------------------------------------------------------
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
#include "Engine/Renderer/CameraScriptInterface.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Engine/Script/ScriptSubsystem.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/ClockScriptInterface.hpp"
#include "Engine/Input/InputScriptInterface.hpp"
#include "Engine/Network/KADIScriptInterface.hpp"
#include "Engine/Renderer/DebugRenderSystemScriptInterface.hpp"
// Phase 2: RendererScriptInterface removed - replaced by EntityScriptInterface
// #include "Engine/Renderer/RendererScriptInterface.hpp"
#include "ThirdParty/json/json.hpp"

// Phase 1: Async Architecture Includes
#include "Engine/Renderer/RenderCommandQueue.hpp"
#include "Game/Framework/EntityStateBuffer.hpp"
#include "Game/Framework/JSGameLogicJob.hpp"

// Phase 2: High-Level Entity API Includes
#include "Game/Framework/HighLevelEntityAPI.hpp"
#include "Game/Framework/EntityScriptInterface.hpp"

// Phase 2: Geometry Creation Utilities
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Math/AABB3.hpp"

// Standard library for threading
#include <chrono>
#include <thread>



//----------------------------------------------------------------------------------------------------
App*  g_app  = nullptr;       // Created and owned by Main_Windows.cpp
Game* g_game = nullptr;       // Created and owned by the App

//----------------------------------------------------------------------------------------------------
// Phase 2: Static Vertex Buffer Storage
// Shared between CreateGeometryForMeshType() and RenderEntities()
static std::unordered_map<int, VertexList_PCU> g_vertexBuffers;
static int g_nextVertexBufferHandle = 1;

//----------------------------------------------------------------------------------------------------
STATIC bool App::m_isQuitting = false;

//----------------------------------------------------------------------------------------------------
App::App()
    : m_renderCommandQueue(nullptr)
    , m_entityStateBuffer(nullptr)
    , m_jsGameLogicJob(nullptr)
    , m_mainCamera(nullptr)
    , m_highLevelEntityAPI(nullptr)
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
    m_renderCommandQueue = new RenderCommandQueue();
    m_entityStateBuffer  = new EntityStateBuffer();

    // Phase 2: Initialize high-level entity API (requires RenderCommandQueue)
    m_highLevelEntityAPI = new HighLevelEntityAPI(m_renderCommandQueue, g_scriptSubsystem, g_renderer);
    DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Startup - HighLevelEntityAPI initialized (Phase 2)");

    // Phase 3: Initialize main camera for rendering entities
    m_mainCamera = new Camera();
    m_mainCamera->m_mode = Camera::eMode_Perspective;
    m_mainCamera->SetPerspectiveGraphicView(16.0f / 9.0f, 60.0f, 0.1f, 100.0f);  // aspect, fov, near, far
    m_mainCamera->SetNormalizedViewport(AABB2(Vec2::ZERO, Vec2::ONE));  // Full screen viewport (0,0) to (1,1)

    // Phase 3 FIX: Set camera-to-render transform to correct coordinate system rotation
    // The engine uses I-Forward/J-Left/K-Up (X-Forward/Y-Left/Z-Up)
    // But rendering expects a 90° CCW rotation around Z-axis to match screen orientation
    // This 90° CCW rotation swaps: X→Y, Y→-X (keeps Z unchanged)
    Mat44 cameraToRender;
    cameraToRender.m_values[Mat44::Ix] =  0.0f;  // New I-basis X component (was pointing X, now points Y)
    cameraToRender.m_values[Mat44::Iy] =  1.0f;  // New I-basis Y component
    cameraToRender.m_values[Mat44::Iz] =  0.0f;  // New I-basis Z component
    cameraToRender.m_values[Mat44::Iw] =  0.0f;

    cameraToRender.m_values[Mat44::Jx] = -1.0f;  // New J-basis X component (was pointing Y, now points -X)
    cameraToRender.m_values[Mat44::Jy] =  0.0f;  // New J-basis Y component
    cameraToRender.m_values[Mat44::Jz] =  0.0f;  // New J-basis Z component
    cameraToRender.m_values[Mat44::Jw] =  0.0f;

    cameraToRender.m_values[Mat44::Kx] =  0.0f;  // New K-basis X component (Z stays Z)
    cameraToRender.m_values[Mat44::Ky] =  0.0f;  // New K-basis Y component
    cameraToRender.m_values[Mat44::Kz] =  1.0f;  // New K-basis Z component
    cameraToRender.m_values[Mat44::Kw] =  0.0f;

    cameraToRender.m_values[Mat44::Tx] =  0.0f;  // No translation
    cameraToRender.m_values[Mat44::Ty] =  0.0f;
    cameraToRender.m_values[Mat44::Tz] =  0.0f;
    cameraToRender.m_values[Mat44::Tw] =  1.0f;

    m_mainCamera->SetCameraToRenderTransform(cameraToRender);
    DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Startup - Main camera initialized with 90° CCW Z-rotation camera-to-render transform");

    g_game = new Game();
    SetupScriptingBindings();
    g_game->PostInit();

    // Phase 1: Submit JavaScript worker thread job AFTER game and script initialization
    m_jsGameLogicJob = new JSGameLogicJob(g_game, m_renderCommandQueue, m_entityStateBuffer);
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

    // m_gameScriptInterface.reset();
    // m_inputScriptInterface.reset();
    // m_audioScriptInterface.reset();
    // m_cameraScriptInterface.reset();
    // Phase 2: RendererScriptInterface removed
    // m_rendererScriptInterface.reset();  // ← Clear vertex arrays before g_renderer destructs
    // m_clockScriptInterface.reset();

    GAME_SAFE_RELEASE(g_game);

    // Phase 2: Cleanup high-level entity API BEFORE async infrastructure
    if (m_highLevelEntityAPI)
    {
        delete m_highLevelEntityAPI;
        m_highLevelEntityAPI = nullptr;
        DAEMON_LOG(LogScript, eLogVerbosity::Display, "App::Shutdown - HighLevelEntityAPI destroyed (Phase 2)");
    }

    // Phase 1: Cleanup async infrastructure AFTER game destruction
    if (m_mainCamera)
    {
        delete m_mainCamera;
        m_mainCamera = nullptr;
    }

    if (m_entityStateBuffer)
    {
        delete m_entityStateBuffer;
        m_entityStateBuffer = nullptr;
    }

    if (m_renderCommandQueue)
    {
        delete m_renderCommandQueue;
        m_renderCommandQueue = nullptr;
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

    // KADI broker integration frame updates
    if (g_kadiSubsystem)
    {
        g_kadiSubsystem->BeginFrame();
    }
}

//----------------------------------------------------------------------------------------------------
void App::Update()
{
    Clock::TickSystemClock();
    UpdateCursorMode();

    // Process pending hot-reload events on main thread (V8-safe)
    if (g_scriptSubsystem)
    {
        g_scriptSubsystem->Update();
    }

    // Phase 1: Async Frame Synchronization
    // Check if worker thread completed previous JavaScript frame
    if (m_jsGameLogicJob && m_jsGameLogicJob->IsFrameComplete())
    {
        // Swap entity state buffers (copy back buffer → front buffer)
        if (m_entityStateBuffer)
        {
            m_entityStateBuffer->SwapBuffers();
        }

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

    // Legacy synchronous JavaScript update (TODO: Remove after Phase 1 validation)
    // Kept temporarily to avoid breaking existing JavaScript functionality
    // NOTE: This runs synchronously on main thread but accesses V8 via ScriptSubsystem's thread-safe interface
    g_game->UpdateJS();

    // Phase 2: Execute pending JavaScript callbacks AFTER JavaScript frame completes
    // CRITICAL: Must be called AFTER UpdateJS() so V8 context exists from the JavaScript execution
    // The callbacks will execute using the V8 isolate that was just active during UpdateJS()
    if (m_highLevelEntityAPI)
    {
        m_highLevelEntityAPI->ExecutePendingCallbacks();
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
    g_game->RenderJS();

    // Phase 2: Render all entities from EntityStateBuffer
    RenderEntities();

    AABB2 const box = AABB2(Vec2::ZERO, Vec2(1600.f, 30.f));

    g_devConsole->Render(box);
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

    // KADI broker integration frame updates
    if (g_kadiSubsystem)
    {
        g_kadiSubsystem->EndFrame();
    }
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
    bool const doesWindowHasFocus   = GetActiveWindow() == g_window->GetWindowHandle();
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
    std::string projectRoot = "C:/p4/Personal/SD/ProtogameJS3D/";

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

    m_cameraScriptInterface = std::make_shared<CameraScriptInterface>();
    g_scriptSubsystem->RegisterScriptableObject("cameraInterface", m_cameraScriptInterface);

    // Phase 2: RendererScriptInterface removed - replaced by EntityScriptInterface
    // m_rendererScriptInterface = std::make_shared<RendererScriptInterface>(g_renderer);
    // g_scriptSubsystem->RegisterScriptableObject("renderer", m_rendererScriptInterface);

    m_debugRenderSystemScriptInterface = std::make_shared<DebugRenderSystemScriptInterface>();
    g_scriptSubsystem->RegisterScriptableObject("debugRenderInterface", m_debugRenderSystemScriptInterface);

    m_clockScriptInterface = std::make_shared<ClockScriptInterface>();
    g_scriptSubsystem->RegisterScriptableObject("clock", m_clockScriptInterface);

    // Phase 2: Register high-level entity API for JavaScript
    if (m_highLevelEntityAPI)
    {
        m_entityScriptInterface = std::make_shared<EntityScriptInterface>(m_highLevelEntityAPI);
        g_scriptSubsystem->RegisterScriptableObject("entity", m_entityScriptInterface);
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(App::SetupScriptingBindings) Entity script interface registered (Phase 2)"));
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
    if (!m_renderCommandQueue || !m_entityStateBuffer || !m_highLevelEntityAPI)
    {
        return;
    }

    static int s_commandCount = 0;

    // Process all commands from queue
    m_renderCommandQueue->ConsumeAll([this](RenderCommand const& cmd) {
        s_commandCount++;
        DebuggerPrintf("[TRACE] ProcessRenderCommands - Processing command #%d, type=%d, entityId=%llu\n",
                       s_commandCount, static_cast<int>(cmd.type), cmd.entityId);

        switch (cmd.type)
        {
            case RenderCommandType::CREATE_MESH:
            {
                auto const& meshData = std::get<MeshCreationData>(cmd.data);
                DebuggerPrintf("[TRACE] ProcessRenderCommands - CREATE_MESH: meshType=%s, pos=(%.1f,%.1f,%.1f), radius=%.1f\n",
                               meshData.meshType.c_str(), meshData.position.x, meshData.position.y, meshData.position.z, meshData.radius);
                int vbHandle = CreateGeometryForMeshType(meshData.meshType, meshData.radius, meshData.color);

                if (vbHandle != 0)
                {
                    EntityState state;
                    state.position = meshData.position;
                    state.orientation = EulerAngles::ZERO;
                    state.color = meshData.color;
                    state.radius = meshData.radius;
                    state.meshType = meshData.meshType;
                    state.isActive = true;
                    state.vertexBufferHandle = vbHandle;
                    state.cameraType = "world";  // Phase 2: All mesh entities use world camera by default

                    auto* backBuffer = m_entityStateBuffer->GetBackBuffer();
                    (*backBuffer)[cmd.entityId] = state;

                    DebuggerPrintf("[TRACE] ProcessRenderCommands - Entity %llu added to back buffer (vbHandle=%d, vertCount=%zu)\n",
                                   cmd.entityId, vbHandle, backBuffer->size());
                }
                else
                {
                    DebuggerPrintf("[TRACE] ProcessRenderCommands - ERROR: CreateGeometryForMeshType returned 0 (failed)!\n");
                }
                break;
            }

            case RenderCommandType::UPDATE_ENTITY:
            {
                auto const& updateData = std::get<EntityUpdateData>(cmd.data);
                auto* backBuffer = m_entityStateBuffer->GetBackBuffer();
                auto it = backBuffer->find(cmd.entityId);

                if (it != backBuffer->end())
                {
                    if (updateData.position.has_value()) it->second.position = updateData.position.value();
                    if (updateData.orientation.has_value()) it->second.orientation = updateData.orientation.value();
                    if (updateData.color.has_value()) it->second.color = updateData.color.value();
                }
                break;
            }

            case RenderCommandType::DESTROY_ENTITY:
            {
                auto* backBuffer = m_entityStateBuffer->GetBackBuffer();
                auto it = backBuffer->find(cmd.entityId);
                if (it != backBuffer->end()) it->second.isActive = false;
                break;
            }

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
    if (!m_entityStateBuffer || !m_mainCamera)
    {
        return;
    }

    // Get front buffer for reading (thread-safe, no locking)
    EntityStateMap const* frontBuffer = m_entityStateBuffer->GetFrontBuffer();
    if (!frontBuffer)
    {
        return;
    }

    static int s_frameCount = 0;
    s_frameCount++;

    // Log entity count periodically (every 60 frames = ~1 second)
    if (s_frameCount % 60 == 0)
    {
        DebuggerPrintf("[TRACE] RenderEntities - Frame %d: Front buffer has %zu entities\n",
                       s_frameCount, frontBuffer->size());
    }

    //========================================
    // BLOCK 1: World Camera (3D Entities)
    //========================================
    // Phase 2: Entity-based camera selection - render entities tagged with "world" camera type

    Camera* worldCamera = m_cameraScriptInterface->GetCameraByRole("world");
    if (!worldCamera)
    {
        // Fallback to active world camera (backward compatibility)
        worldCamera = m_cameraScriptInterface->GetActiveWorldCameraPtr();
    }
    if (!worldCamera)
    {
        // Final fallback to m_mainCamera
        worldCamera = m_mainCamera;
        if (s_frameCount % 300 == 0) // Log warning every 5 seconds
        {
            DebuggerPrintf("[WARNING] RenderEntities - No world camera set, using fallback m_mainCamera\n");
        }
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

        // Skip entities without valid vertex buffer handles
        if (state.vertexBufferHandle == 0) continue;

        // Find vertex data in global vertex buffer storage
        auto it = g_vertexBuffers.find(state.vertexBufferHandle);
        if (it == g_vertexBuffers.end()) continue;

        VertexList_PCU const& verts = it->second;
        if (verts.empty()) continue;

        // Set model transformation matrix
        // Coordinate System: X-forward, Y-left, Z-up
        Mat44 modelMatrix = Mat44::MakeTranslation3D(state.position);
        modelMatrix.Append(Mat44::MakeZRotationDegrees(state.orientation.m_yawDegrees));
        modelMatrix.Append(Mat44::MakeYRotationDegrees(state.orientation.m_pitchDegrees));
        modelMatrix.Append(Mat44::MakeXRotationDegrees(state.orientation.m_rollDegrees));

        g_renderer->SetModelConstants(modelMatrix, state.color);
        g_renderer->BindTexture(nullptr);  // Phase 2.5: Will be replaced with per-entity texture binding

        // Draw entity geometry
        g_renderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
        worldRenderedCount++;
    }

    g_renderer->EndCamera(*worldCamera);

    //========================================
    // BLOCK 2: Screen Camera (2D UI/Attract Mode)
    //========================================
    // Phase 2: Render entities tagged with "screen" camera type (2D UI, attract mode)

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

    // Log world render count periodically
    if (s_frameCount % 60 == 0 && worldRenderedCount > 0)
    {
        DebuggerPrintf("[TRACE] RenderEntities - Rendered %d world entities this frame\n", worldRenderedCount);
    }
}

//----------------------------------------------------------------------------------------------------
// CreateGeometryForMeshType (Phase 2 Helper Method)
//
// Creates geometry for a given mesh type and returns vertex buffer handle.
// Returns 0 on failure.
//----------------------------------------------------------------------------------------------------
int App::CreateGeometryForMeshType(std::string const& meshType, float radius, Rgba8 const& color)
{
    // Create vertex list
    VertexList_PCU verts;

    if (meshType == "cube")
    {
        // Create cube geometry
        AABB3 cubeBox(Vec3(-radius, -radius, -radius), Vec3(radius, radius, radius));
        AddVertsForAABB3D(verts, cubeBox, color);
    }
    else if (meshType == "sphere")
    {
        // Create sphere geometry
        AddVertsForSphere3D(verts, Vec3::ZERO, radius, color, AABB2::ZERO_TO_ONE, 32, 16);
    }
    else if (meshType == "grid")
    {
        // Create grid geometry (horizontal floor in XZ plane)
        // Grid lies flat on the ground, visible when camera looks down
        float gridSize = radius * 2.0f;

        // Create quad in XZ plane (horizontal floor), Y=0
        Vec3 bottomLeft(-gridSize / 2.0f, 0.0f, -gridSize / 2.0f);  // Back-left corner
        Vec3 bottomRight(gridSize / 2.0f, 0.0f, -gridSize / 2.0f);  // Back-right corner
        Vec3 topLeft(-gridSize / 2.0f, 0.0f, gridSize / 2.0f);      // Front-left corner
        Vec3 topRight(gridSize / 2.0f, 0.0f, gridSize / 2.0f);      // Front-right corner

        AddVertsForQuad3D(verts, bottomLeft, bottomRight, topLeft, topRight, color);
    }
    else if (meshType == "plane")
    {
        // Create plane geometry (simple quad)
        float halfSize = radius;
        Vec3 bottomLeft(-halfSize, -halfSize, 0.0f);
        Vec3 bottomRight(halfSize, -halfSize, 0.0f);
        Vec3 topLeft(-halfSize, halfSize, 0.0f);
        Vec3 topRight(halfSize, halfSize, 0.0f);
        AddVertsForQuad3D(verts, bottomLeft, bottomRight, topLeft, topRight, color);
    }
    else
    {
        return 0;  // Unknown mesh type
    }

    // Check if geometry was created
    if (verts.empty())
    {
        return 0;
    }

    // Allocate handle and store vertex data in global storage
    int handle = g_nextVertexBufferHandle++;
    g_vertexBuffers[handle] = verts;

    return handle;
}
