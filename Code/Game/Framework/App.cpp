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
#include "Engine/Renderer/RendererScriptInterface.hpp"
#include "ThirdParty/json/json.hpp"

// Phase 1: Async Architecture Includes
#include "Engine/Renderer/RenderCommandQueue.hpp"
#include "Game/Framework/EntityStateBuffer.hpp"
#include "Game/Framework/JSGameLogicJob.hpp"

// Standard library for threading
#include <chrono>
#include <thread>



//----------------------------------------------------------------------------------------------------
App*  g_app  = nullptr;       // Created and owned by Main_Windows.cpp
Game* g_game = nullptr;       // Created and owned by the App


//----------------------------------------------------------------------------------------------------
STATIC bool App::m_isQuitting = false;

//----------------------------------------------------------------------------------------------------
App::App()
    : m_renderCommandQueue(nullptr)
    , m_entityStateBuffer(nullptr)
    , m_jsGameLogicJob(nullptr)
    , m_mainCamera(nullptr)
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
    // m_rendererScriptInterface.reset();  // ← Clear vertex arrays before g_renderer destructs
    // m_clockScriptInterface.reset();

    GAME_SAFE_RELEASE(g_game);

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
    g_game->UpdateJS();
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

    m_rendererScriptInterface = std::make_shared<RendererScriptInterface>(g_renderer);
    g_scriptSubsystem->RegisterScriptableObject("renderer", m_rendererScriptInterface);

    m_debugRenderSystemScriptInterface = std::make_shared<DebugRenderSystemScriptInterface>();
    g_scriptSubsystem->RegisterScriptableObject("debugRenderInterface", m_debugRenderSystemScriptInterface);

    m_clockScriptInterface = std::make_shared<ClockScriptInterface>();
    g_scriptSubsystem->RegisterScriptableObject("clock", m_clockScriptInterface);

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
    if (!m_renderCommandQueue)
    {
        return;
    }

    // Phase 1: Placeholder implementation
    // Consume all commands and log them (no actual processing yet)
    m_renderCommandQueue->ConsumeAll([](RenderCommand const& cmd) {
        // Phase 1: Log command type only (no processing)
        static uint64_t totalCommands = 0;
        if (totalCommands % 60 == 0)  // Log every 60 commands
        {
            DAEMON_LOG(LogRenderer, eLogVerbosity::Display,
                       Stringf("App::ProcessRenderCommands - Placeholder: Consumed command type %d (total: %llu)",
                               static_cast<int>(cmd.type),
                               totalCommands));
        }
        ++totalCommands;

        // Phase 2 will implement:
        // switch (cmd.type) {
        //     case RenderCommandType::CREATE_MESH:
        //         // Create mesh entity
        //         break;
        //     case RenderCommandType::UPDATE_ENTITY:
        //         // Update entity position/orientation/color
        //         break;
        //     // ... other command types
        // }
    });
}
