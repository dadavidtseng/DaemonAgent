//----------------------------------------------------------------------------------------------------
// App.cpp
//----------------------------------------------------------------------------------------------------

// IMPORTANT: Define NOMINMAX before any Windows headers to prevent min/max macro conflicts with V8
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "Game/Framework/App.hpp"
//----------------------------------------------------------------------------------------------------
#include "Game/Framework/GameCommon.hpp"
#include "Game/Framework/GameScriptInterface.hpp"
#include "Game/Framework/JSGameLogicJob.hpp"
#include "Game/Framework/RenderResourceManager.hpp"
#include "Game/Gameplay/Game.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Audio/AudioAPI.hpp"
#include "Engine/Audio/AudioCommand.hpp"
#include "Engine/Audio/AudioCommandQueue.hpp"
#include "Engine/Audio/AudioScriptInterface.hpp"
#include "Engine/Audio/AudioStateBuffer.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/CallbackQueue.hpp"
#include "Engine/Core/CallbackQueueScriptInterface.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/ClockScriptInterface.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/GenericCommandExecutor.hpp"
#include "Engine/Core/GenericCommandQueue.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Entity/EntityAPI.hpp"
#include "Engine/Entity/EntityScriptInterface.hpp"
#include "Engine/Entity/EntityStateBuffer.hpp"
#include "Engine/Input/InputScriptInterface.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Network/KADIScriptInterface.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/CameraAPI.hpp"
#include "Engine/Renderer/CameraScriptInterface.hpp"
#include "Engine/Renderer/CameraStateBuffer.hpp"
#include "Engine/Renderer/DebugRenderAPI.hpp"
#include "Engine/Renderer/DebugRenderStateBuffer.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/DebugRenderSystemScriptInterface.hpp"
#include "Engine/Renderer/RenderCommandQueue.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Engine/Script/ScriptSubsystem.hpp"
#include "Engine/Script/GenericCommandScriptInterface.hpp"
#include "Engine/UI/ImGuiSubsystem.hpp"
#include "ThirdParty/json/json.hpp"
#include <fstream>


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

    // Initialize async architecture infrastructure
    m_callbackQueue      = new CallbackQueue();
    m_renderCommandQueue = new RenderCommandQueue();
    m_audioCommandQueue  = new AudioCommandQueue();
    // Load GenericCommand configuration from JSON (optional — uses defaults if file missing)
    size_t   gcQueueCapacity   = 500;    // GenericCommandQueue::DEFAULT_CAPACITY
    uint32_t gcRateLimitPerAgent = 100;  // Default: 100 commands/sec per agent
    bool     gcAuditLogging    = false;
    try
    {
        std::ifstream configFile("Data/Config/GenericCommand.json");
        if (configFile.is_open())
        {
            nlohmann::json jsonConfig;
            configFile >> jsonConfig;

            gcQueueCapacity    = jsonConfig.value("queueCapacity", 500);
            gcRateLimitPerAgent = jsonConfig.value("rateLimitPerAgent", 100u);
            gcAuditLogging     = jsonConfig.value("enableAuditLogging", false);

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand config loaded: capacity=%zu, rateLimit=%u/s, audit=%s",
                           gcQueueCapacity, gcRateLimitPerAgent, gcAuditLogging ? "ON" : "OFF"));
        }
        else
        {
            DAEMON_LOG(LogApp, eLogVerbosity::Log, "GenericCommand.json not found, using defaults");
        }
    }
    catch (nlohmann::json::exception const& e)
    {
        DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                   Stringf("GenericCommand config parse error: %s - using defaults", e.what()));
    }

    m_genericCommandQueue    = new GenericCommandQueue(gcQueueCapacity);
    m_genericCommandExecutor = new GenericCommandExecutor();
    m_genericCommandExecutor->SetRateLimitPerAgent(gcRateLimitPerAgent);
    m_genericCommandExecutor->SetAuditLoggingEnabled(gcAuditLogging);

    // === Smoke-test handler: "ping" → returns {pong: "hello"} ===
    m_genericCommandExecutor->RegisterHandler("ping", [](std::any const& /*payload*/) -> HandlerResult
    {
        DAEMON_LOG(LogApp, eLogVerbosity::Log, "GenericCommand [ping] handler: received ping, returning pong");
        return HandlerResult::Success({{"pong", std::any(String("hello"))}});
    });

    // Initialize state buffers with dirty tracking for O(d) swap optimization
    m_entityStateBuffer = new EntityStateBuffer();
    m_entityStateBuffer->EnableDirtyTracking(true);

    m_cameraStateBuffer = new CameraStateBuffer();
    m_cameraStateBuffer->EnableDirtyTracking(true);

    m_debugRenderStateBuffer = new DebugRenderStateBuffer();
    m_debugRenderStateBuffer->EnableDirtyTracking(true);

    m_audioStateBuffer = new AudioStateBuffer();
    m_audioStateBuffer->EnableDirtyTracking(true);

    // Initialize render resource manager
    m_renderResourceManager = new RenderResourceManager();

    // Initialize APIs
    m_entityAPI      = new EntityAPI(m_renderCommandQueue, g_scriptSubsystem);
    m_cameraAPI      = new CameraAPI(m_renderCommandQueue, g_scriptSubsystem, m_cameraStateBuffer);
    m_debugRenderAPI = new DebugRenderAPI(m_renderCommandQueue, g_scriptSubsystem, m_debugRenderStateBuffer, m_callbackQueue);
    m_audioAPI       = new AudioAPI(m_audioCommandQueue, g_scriptSubsystem, m_callbackQueue);

    // === GenericCommand handler: "create_mesh" (Task 8.3 — EntityScriptInterface migration) ===
    // Replaces EntityScriptInterface::ExecuteCreateMesh with GenericCommand pipeline.
    // Handler runs on main thread; entity ID is generated immediately, render command queued.
    // Flow: JS submit("create_mesh", payload) → handler → HandlerResult::Success({resultId}) → JS callback
    m_genericCommandExecutor->RegisterHandler("create_mesh",
        [this](std::any const& payload) -> HandlerResult
        {
            // Parse JSON payload from JavaScript
            String payloadStr;
            try
            {
                payloadStr = std::any_cast<String>(payload);
            }
            catch (std::bad_any_cast const&)
            {
                return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string");
            }

            nlohmann::json json;
            try
            {
                json = nlohmann::json::parse(payloadStr);
            }
            catch (nlohmann::json::exception const& e)
            {
                return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what()));
            }

            // Extract fields with defaults matching EntityAPI.js validation
            String meshType = json.value("meshType", "cube");

            auto posArr = json.value("position", std::vector<double>{0.0, 0.0, 0.0});
            Vec3 position(
                static_cast<float>(posArr.size() > 0 ? posArr[0] : 0.0),
                static_cast<float>(posArr.size() > 1 ? posArr[1] : 0.0),
                static_cast<float>(posArr.size() > 2 ? posArr[2] : 0.0)
            );

            float scale = json.value("scale", 1.0f);

            auto colorArr = json.value("color", std::vector<int>{255, 255, 255, 255});
            Rgba8 color(
                static_cast<unsigned char>(colorArr.size() > 0 ? colorArr[0] : 255),
                static_cast<unsigned char>(colorArr.size() > 1 ? colorArr[1] : 255),
                static_cast<unsigned char>(colorArr.size() > 2 ? colorArr[2] : 255),
                static_cast<unsigned char>(colorArr.size() > 3 ? colorArr[3] : 255)
            );

            // Generate entity ID (same as EntityAPI::CreateMesh)
            EntityID entityId = m_entityAPI->GenerateEntityID();

            // Build render command (same as EntityAPI::CreateMesh)
            MeshCreationData meshData;
            meshData.meshType = meshType;
            meshData.position = position;
            meshData.radius   = scale;
            meshData.color    = color;

            RenderCommand command(RenderCommandType::CREATE_MESH, entityId, meshData);
            bool submitted = m_renderCommandQueue->Submit(command);

            if (!submitted)
            {
                DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                           Stringf("GenericCommand [create_mesh]: RenderCommandQueue full, entity %llu dropped", entityId));
                return HandlerResult::Error("ERR_QUEUE_FULL: render command queue is full");
            }

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [create_mesh]: entityId=%llu, mesh=%s, pos=(%.1f,%.1f,%.1f), scale=%.1f",
                           entityId, meshType.c_str(), position.x, position.y, position.z, scale));

            // Return entityId via resultId field — GenericCommand pipeline delivers to JS callback
            return HandlerResult::Success({{"resultId", std::any(static_cast<uint64_t>(entityId))}});
        });

    DAEMON_LOG(LogApp, eLogVerbosity::Display, "App::Startup - Async architecture initialized");

    g_game = new Game();
    SetupScriptingBindings();
    g_game->PostInit();

    // Submit JavaScript worker thread job after game and script initialization
    m_jsGameLogicJob = new JSGameLogicJob(g_game, m_renderCommandQueue, m_entityStateBuffer, m_callbackQueue);
    g_jobSystem->SubmitJob(m_jsGameLogicJob);
}

//----------------------------------------------------------------------------------------------------
// Shutdown - reverse order of Startup
//----------------------------------------------------------------------------------------------------
void App::Shutdown()
{
    // Shutdown async job first
    if (m_jsGameLogicJob)
    {
        DAEMON_LOG(LogApp, eLogVerbosity::Display, "App::Shutdown - Requesting worker thread shutdown...");
        m_jsGameLogicJob->RequestShutdown();

        // Wait for worker thread to complete (max 5 seconds timeout)
        constexpr int kMaxWaitIterations = 500;
        constexpr int kWaitMilliseconds  = 10;
        int waitCount = 0;
        while (!m_jsGameLogicJob->IsShutdownComplete() && waitCount < kMaxWaitIterations)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(kWaitMilliseconds));
            ++waitCount;
        }

        if (m_jsGameLogicJob->IsShutdownComplete())
        {
            DAEMON_LOG(LogApp, eLogVerbosity::Display, "App::Shutdown - Worker thread exited successfully");
        }
        else
        {
            DAEMON_LOG(LogApp, eLogVerbosity::Warning, "App::Shutdown - Worker thread shutdown timeout!");
        }

        // Retrieve job from JobSystem before deletion to prevent double-delete
        Job* retrievedJob = g_jobSystem->RetrieveCompletedJob();
        while (retrievedJob != nullptr && retrievedJob != m_jsGameLogicJob)
        {
            delete retrievedJob;
            retrievedJob = g_jobSystem->RetrieveCompletedJob();
        }

        delete m_jsGameLogicJob;
        m_jsGameLogicJob = nullptr;
    }

    // Clear V8::Persistent callbacks before V8 isolate destruction
    if (m_kadiScriptInterface)
    {
        m_kadiScriptInterface->ClearCallbacks();
    }

    GAME_SAFE_RELEASE(g_game);

    // Cleanup APIs (before state buffers)
    delete m_entityAPI;
    m_entityAPI = nullptr;

    delete m_cameraAPI;
    m_cameraAPI = nullptr;

    delete m_debugRenderAPI;
    m_debugRenderAPI = nullptr;

    delete m_audioAPI;
    m_audioAPI = nullptr;

    delete m_renderResourceManager;
    m_renderResourceManager = nullptr;

    // Cleanup state buffers
    delete m_entityStateBuffer;
    m_entityStateBuffer = nullptr;

    delete m_cameraStateBuffer;
    m_cameraStateBuffer = nullptr;

    delete m_debugRenderStateBuffer;
    m_debugRenderStateBuffer = nullptr;

    delete m_audioStateBuffer;
    m_audioStateBuffer = nullptr;

    // Cleanup command queues
    delete m_renderCommandQueue;
    m_renderCommandQueue = nullptr;

    delete m_audioCommandQueue;
    m_audioCommandQueue = nullptr;

    delete m_genericCommandExecutor;
    m_genericCommandExecutor = nullptr;

    delete m_genericCommandQueue;
    m_genericCommandQueue = nullptr;

    delete m_callbackQueue;
    m_callbackQueue = nullptr;

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
    g_scriptSubsystem->Update();

    ProcessRenderCommands();
    ProcessAudioCommands();
    ProcessGenericCommands();

    // Async Frame Synchronization: Check if worker thread completed previous JavaScript frame
    if (m_jsGameLogicJob && m_jsGameLogicJob->IsFrameComplete())
    {
        // Swap state buffers (copy back buffer to front buffer)
        if (m_entityStateBuffer) m_entityStateBuffer->SwapBuffers();
        if (m_cameraStateBuffer) m_cameraStateBuffer->SwapBuffers();
        if (m_audioStateBuffer) m_audioStateBuffer->SwapBuffers();

        if (m_debugRenderStateBuffer)
        {
            m_debugRenderStateBuffer->SwapBuffers();
            // Update debug primitive expiration after swap so primitives render at least once
            float const systemDeltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());
            UpdateDebugPrimitiveExpiration(systemDeltaSeconds);
        }

        m_jsGameLogicJob->TriggerNextFrame();
    }
    else if (m_jsGameLogicJob)
    {
        // Frame skip: Worker still executing, continue with last state (maintains stable 60 FPS)
        static uint64_t frameSkipCount = 0;
        if (frameSkipCount % 60 == 0)
        {
            DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                       Stringf("App::Update - JavaScript frame skip (worker still executing) - Total skips: %llu",
                               frameSkipCount));
        }
        ++frameSkipCount;
    }

    // Execute pending callbacks from APIs
    if (m_entityAPI) m_entityAPI->ExecutePendingCallbacks(m_callbackQueue);
    if (m_cameraAPI) m_cameraAPI->ExecutePendingCallbacks(m_callbackQueue);
    if (m_audioAPI) m_audioAPI->ExecutePendingCallbacks(m_callbackQueue);
    if (m_debugRenderAPI) m_debugRenderAPI->ExecutePendingCallbacks(m_callbackQueue);
    if (m_genericCommandExecutor) m_genericCommandExecutor->ExecutePendingCallbacks(m_callbackQueue);
}

//----------------------------------------------------------------------------------------------------
void App::Render() const
{
    g_renderer->ClearScreen(Rgba8::GREY, Rgba8::BLACK);

    // Render entities only in GAME mode
    if (g_game && !g_game->IsAttractMode())
    {
        RenderEntities();
    }

    // Populate debug primitive queue from state buffer (renders in all modes for UI support)
    if (m_debugRenderStateBuffer)
    {
        RenderDebugPrimitives();
    }

    // Get world camera for 3D debug rendering
    Camera const* worldCamera = nullptr;
    if (m_cameraStateBuffer)
    {
        EntityID activeCameraId = m_cameraStateBuffer->GetActiveCameraID();
        if (activeCameraId != 0)
        {
            worldCamera = m_cameraStateBuffer->GetCameraById(activeCameraId);
        }
    }

    // Render 3D debug visualization (world space) - only in GAME mode
    if (worldCamera && g_game && !g_game->IsAttractMode())
    {
        DebugRenderWorld(*worldCamera);
    }

    // Find and use screen camera for 2D debug rendering
    Camera const* screenCamera = nullptr;
    if (m_cameraStateBuffer)
    {
        CameraStateMap const* frontBuffer = m_cameraStateBuffer->GetFrontBuffer();
        if (frontBuffer)
        {
            for (auto const& [cameraId, cameraState] : *frontBuffer)
            {
                if (cameraState.type == "screen")
                {
                    screenCamera = m_cameraStateBuffer->GetCameraById(cameraId);
                    break;
                }
            }
        }
    }

    if (screenCamera)
    {
        DebugRenderScreen(*screenCamera);
    }

    AABB2 const consoleBox = AABB2(Vec2::ZERO, Vec2(1600.f, 30.f));
    g_devConsole->Render(consoleBox);
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
    bool const windowHasFocus      = GetActiveWindow() == g_window->GetWindowHandle();
    bool const shouldUsePointerMode = !windowHasFocus || g_devConsole->IsOpen();

    g_input->SetCursorMode(shouldUsePointerMode ? eCursorMode::POINTER : eCursorMode::FPS);
}

//----------------------------------------------------------------------------------------------------
void App::SetupScriptingBindings()
{
    if (g_scriptSubsystem == nullptr)
        ERROR_AND_DIE("App::SetupScriptingBindings - g_scriptSubsystem is nullptr!")
    if (!g_scriptSubsystem->IsInitialized())
        ERROR_AND_DIE("App::SetupScriptingBindings - g_scriptSubsystem is not initialized!")
    if (g_game == nullptr)
        ERROR_AND_DIE("App::SetupScriptingBindings - g_game is nullptr")

    DAEMON_LOG(LogApp, eLogVerbosity::Log, "App::SetupScriptingBindings - start");

    // Initialize hot-reload system
    if (g_scriptSubsystem->InitializeHotReload("../"))
    {
        DAEMON_LOG(LogApp, eLogVerbosity::Log, "App::SetupScriptingBindings - Hot-reload system initialized successfully");
    }
    else
    {
        DAEMON_LOG(LogApp, eLogVerbosity::Warning, "App::SetupScriptingBindings - Hot-reload system initialization failed");
    }

    // Register core script interfaces
    m_gameScriptInterface = std::make_shared<GameScriptInterface>(g_game);
    g_scriptSubsystem->RegisterScriptableObject("game", m_gameScriptInterface);

    m_inputScriptInterface = std::make_shared<InputScriptInterface>(g_input);
    g_scriptSubsystem->RegisterScriptableObject("input", m_inputScriptInterface);

    m_audioScriptInterface = std::make_shared<AudioScriptInterface>(g_audio);
    g_scriptSubsystem->RegisterScriptableObject("audio", m_audioScriptInterface);

    m_clockScriptInterface = std::make_shared<ClockScriptInterface>();
    g_scriptSubsystem->RegisterScriptableObject("clock", m_clockScriptInterface);

#ifdef ENGINE_SCRIPTING_ENABLED
    // Configure AudioCommandQueue for async JavaScript audio operations
    if (m_audioCommandQueue && m_callbackQueue && g_audio)
    {
        g_audio->SetCommandQueue(m_audioCommandQueue, m_callbackQueue);
        m_audioScriptInterface->SetCommandQueue(m_audioCommandQueue, m_callbackQueue);
    }
#endif

    // Register debug render script interface
    if (m_debugRenderAPI)
    {
        m_debugRenderSystemScriptInterface = std::make_shared<DebugRenderSystemScriptInterface>(m_debugRenderAPI);
        g_scriptSubsystem->RegisterScriptableObject("debugRenderInterface", m_debugRenderSystemScriptInterface);
    }

    // Register entity and camera script interfaces
    if (m_entityAPI && m_cameraAPI)
    {
        m_entityScriptInterface = std::make_shared<EntityScriptInterface>(m_entityAPI);
        g_scriptSubsystem->RegisterScriptableObject("entity", m_entityScriptInterface);

        m_cameraScriptInterface = std::make_shared<CameraScriptInterface>(m_cameraAPI);
        g_scriptSubsystem->RegisterScriptableObject("camera", m_cameraScriptInterface);
    }

    // Register KADI broker integration
    if (g_kadiSubsystem)
    {
        m_kadiScriptInterface = std::make_shared<KADIScriptInterface>(g_kadiSubsystem);
        m_kadiScriptInterface->SetV8Isolate(g_scriptSubsystem->GetIsolate());
        g_scriptSubsystem->RegisterScriptableObject("kadi", m_kadiScriptInterface);
    }
    else
    {
        DAEMON_LOG(LogApp, eLogVerbosity::Warning, "App::SetupScriptingBindings - KADI subsystem not available");
    }

    // Register callback queue script interface
    if (m_callbackQueue)
    {
        m_callbackQueueScriptInterface = std::make_shared<CallbackQueueScriptInterface>(m_callbackQueue);
        g_scriptSubsystem->RegisterScriptableObject("callbackQueue", m_callbackQueueScriptInterface);
    }

    // Register generic command script interface
    if (m_genericCommandQueue && m_genericCommandExecutor)
    {
        m_genericCommandScriptInterface = std::make_shared<GenericCommandScriptInterface>(m_genericCommandQueue, m_genericCommandExecutor);
        g_scriptSubsystem->RegisterScriptableObject("commandQueue", m_genericCommandScriptInterface);
    }

    // Register global functions
    g_scriptSubsystem->RegisterGlobalFunction("print", OnPrint);
    g_scriptSubsystem->RegisterGlobalFunction("debug", OnDebug);
    g_scriptSubsystem->RegisterGlobalFunction("gc", OnGarbageCollection);

    DAEMON_LOG(LogApp, eLogVerbosity::Log, "App::SetupScriptingBindings - end");
}

//----------------------------------------------------------------------------------------------------
void App::ProcessRenderCommands()
{
    if (!m_renderCommandQueue || !m_entityStateBuffer || !m_cameraStateBuffer || !m_entityAPI || !m_cameraAPI)
    {
        return;
    }

    m_renderCommandQueue->ConsumeAll([this](RenderCommand const& cmd)
    {
        switch (cmd.type)
        {
        case RenderCommandType::CREATE_MESH:
            {
                MeshCreationData const& meshData = std::get<MeshCreationData>(cmd.data);
                int vbHandle = m_renderResourceManager->RegisterEntity(cmd.entityId, meshData.meshType, meshData.radius, meshData.color);

                if (vbHandle != 0)
                {
                    EntityState state;
                    state.position    = meshData.position;
                    state.orientation = EulerAngles::ZERO;
                    state.color       = meshData.color;
                    state.radius      = meshData.radius;
                    state.meshType    = meshData.meshType;
                    state.isActive    = true;
                    state.cameraType  = "world";

                    auto* backBuffer = m_entityStateBuffer->GetBackBuffer();
                    (*backBuffer)[cmd.entityId] = state;
                    m_entityStateBuffer->MarkDirty(cmd.entityId);
                }
                break;
            }

        case RenderCommandType::UPDATE_ENTITY:
            {
                EntityUpdateData const& updateData = std::get<EntityUpdateData>(cmd.data);
                auto* backBuffer = m_entityStateBuffer->GetBackBuffer();
                auto  it = backBuffer->find(cmd.entityId);

                if (it != backBuffer->end())
                {
                    if (updateData.position.has_value()) it->second.position = updateData.position.value();
                    if (updateData.orientation.has_value()) it->second.orientation = updateData.orientation.value();
                    if (updateData.color.has_value()) it->second.color = updateData.color.value();
                    m_entityStateBuffer->MarkDirty(cmd.entityId);
                }
                break;
            }

        case RenderCommandType::DESTROY_ENTITY:
            {
                auto* backBuffer = m_entityStateBuffer->GetBackBuffer();
                auto  it = backBuffer->find(cmd.entityId);
                if (it != backBuffer->end()) it->second.isActive = false;
                break;
            }

        case RenderCommandType::CREATE_CAMERA:
            {
                auto const& cameraData = std::get<CameraCreationData>(cmd.data);

                CameraState state;
                state.position    = cameraData.position;
                state.orientation = cameraData.orientation;
                state.type        = cameraData.type;
                state.isActive    = true;

                // Configure camera based on type
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
                    Vec2 viewportDimensions = Vec2(1600.f, 800.f);
                    if (Window::s_mainWindow)
                    {
                        viewportDimensions = Window::s_mainWindow->GetViewportDimensions();
                    }

                    state.mode        = Camera::eMode_Orthographic;
                    state.orthoLeft   = 0.0f;
                    state.orthoBottom = 0.0f;
                    state.orthoRight  = viewportDimensions.x;
                    state.orthoTop    = viewportDimensions.y;
                    state.orthoNear   = 0.0f;
                    state.orthoFar    = 1.0f;
                    state.viewport    = AABB2(Vec2::ZERO, Vec2::ONE);
                }

                auto* backBuffer = m_cameraStateBuffer->GetBackBuffer();
                (*backBuffer)[cmd.entityId] = state;
                m_cameraStateBuffer->MarkDirty(cmd.entityId);

                if (m_cameraAPI && cameraData.callbackId != 0)
                {
                    m_cameraAPI->NotifyCallbackReady(cameraData.callbackId, cmd.entityId);
                }
                break;
            }

        case RenderCommandType::UPDATE_CAMERA:
            {
                auto const& updateData = std::get<CameraUpdateData>(cmd.data);
                auto* backBuffer = m_cameraStateBuffer->GetBackBuffer();
                auto  it = backBuffer->find(cmd.entityId);

                if (it != backBuffer->end())
                {
                    it->second.position    = updateData.position;
                    it->second.orientation = updateData.orientation;
                    m_cameraStateBuffer->MarkDirty(cmd.entityId);
                }
                else
                {
                    DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                               StringFormat("ProcessRenderCommands UPDATE_CAMERA: Camera {} NOT FOUND", cmd.entityId));
                }
                break;
            }

        case RenderCommandType::SET_ACTIVE_CAMERA:
            m_cameraStateBuffer->SetActiveCameraID(cmd.entityId);
            break;

        case RenderCommandType::UPDATE_CAMERA_TYPE:
            {
                auto const& typeData = std::get<CameraTypeUpdateData>(cmd.data);
                auto* backBuffer = m_cameraStateBuffer->GetBackBuffer();
                auto  it = backBuffer->find(cmd.entityId);

                if (it != backBuffer->end())
                {
                    it->second.type = typeData.type;

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
                        Vec2 clientDimensions = Vec2(1600.f, 800.f);
                        if (Window::s_mainWindow)
                        {
                            clientDimensions = Window::s_mainWindow->GetClientDimensions();
                        }

                        it->second.mode        = Camera::eMode_Orthographic;
                        it->second.orthoLeft   = 0.0f;
                        it->second.orthoBottom = 0.0f;
                        it->second.orthoRight  = clientDimensions.x;
                        it->second.orthoTop    = clientDimensions.y;
                        it->second.orthoNear   = 0.0f;
                        it->second.orthoFar    = 1.0f;
                        it->second.viewport    = AABB2(Vec2::ZERO, Vec2::ONE);
                    }
                }
                break;
            }

        case RenderCommandType::DESTROY_CAMERA:
            {
                auto* backBuffer = m_cameraStateBuffer->GetBackBuffer();
                auto  it = backBuffer->find(cmd.entityId);
                if (it != backBuffer->end()) it->second.isActive = false;
                break;
            }

        case RenderCommandType::DEBUG_ADD_LINE:
            {
                auto const& lineData = std::get<DebugLineData>(cmd.data);
                DebugPrimitive primitive;
                primitive.primitiveId   = cmd.entityId;
                primitive.type          = DebugPrimitiveType::LINE;
                primitive.startPos      = lineData.start;
                primitive.endPos        = lineData.end;
                primitive.startColor    = lineData.startColor;
                primitive.endColor      = lineData.endColor;
                primitive.radius        = lineData.radius;
                primitive.duration      = lineData.duration;
                primitive.timeRemaining = lineData.duration;
                primitive.isActive      = true;

                (*m_debugRenderStateBuffer->GetBackBuffer())[cmd.entityId] = primitive;
                m_debugRenderStateBuffer->MarkDirty(cmd.entityId);
                break;
            }

        case RenderCommandType::DEBUG_ADD_POINT:
            {
                auto const& pointData = std::get<DebugPointData>(cmd.data);
                DebugPrimitive primitive;
                primitive.primitiveId   = cmd.entityId;
                primitive.type          = DebugPrimitiveType::POINT;
                primitive.startPos      = pointData.position;
                primitive.startColor    = pointData.color;
                primitive.radius        = pointData.radius;
                primitive.duration      = pointData.duration;
                primitive.timeRemaining = pointData.duration;
                primitive.isBillboard   = pointData.isBillboard;
                primitive.isActive      = true;

                (*m_debugRenderStateBuffer->GetBackBuffer())[cmd.entityId] = primitive;
                m_debugRenderStateBuffer->MarkDirty(cmd.entityId);
                break;
            }

        case RenderCommandType::DEBUG_ADD_SPHERE:
            {
                auto const& sphereData = std::get<DebugSphereData>(cmd.data);
                DebugPrimitive primitive;
                primitive.primitiveId   = cmd.entityId;
                primitive.type          = DebugPrimitiveType::SPHERE;
                primitive.startPos      = sphereData.center;
                primitive.startColor    = sphereData.color;
                primitive.radius        = sphereData.radius;
                primitive.duration      = sphereData.duration;
                primitive.timeRemaining = sphereData.duration;
                primitive.isSolid       = sphereData.isSolid;
                primitive.isActive      = true;

                (*m_debugRenderStateBuffer->GetBackBuffer())[cmd.entityId] = primitive;
                m_debugRenderStateBuffer->MarkDirty(cmd.entityId);
                break;
            }

        case RenderCommandType::DEBUG_ADD_AABB:
            {
                auto const& aabbData = std::get<DebugAABBData>(cmd.data);
                DebugPrimitive primitive;
                primitive.primitiveId   = cmd.entityId;
                primitive.type          = DebugPrimitiveType::AABB;
                primitive.startPos      = aabbData.minBounds;
                primitive.endPos        = aabbData.maxBounds;
                primitive.startColor    = aabbData.color;
                primitive.duration      = aabbData.duration;
                primitive.timeRemaining = aabbData.duration;
                primitive.isActive      = true;

                (*m_debugRenderStateBuffer->GetBackBuffer())[cmd.entityId] = primitive;
                m_debugRenderStateBuffer->MarkDirty(cmd.entityId);
                break;
            }

        case RenderCommandType::DEBUG_ADD_BASIS:
            {
                auto const& basisData = std::get<DebugBasisData>(cmd.data);
                DebugPrimitive primitive;
                primitive.primitiveId   = cmd.entityId;
                primitive.type          = DebugPrimitiveType::BASIS;
                primitive.startPos      = basisData.position;
                primitive.basisI        = basisData.iBasis;
                primitive.basisJ        = basisData.jBasis;
                primitive.basisK        = basisData.kBasis;
                primitive.duration      = basisData.duration;
                primitive.timeRemaining = basisData.duration;
                primitive.radius        = basisData.axisLength;
                primitive.isActive      = true;

                (*m_debugRenderStateBuffer->GetBackBuffer())[cmd.entityId] = primitive;
                m_debugRenderStateBuffer->MarkDirty(cmd.entityId);
                break;
            }

        case RenderCommandType::DEBUG_ADD_WORLD_TEXT:
            {
                auto const& textData = std::get<DebugWorldTextData>(cmd.data);
                DebugPrimitive primitive;
                primitive.primitiveId   = cmd.entityId;
                primitive.type          = DebugPrimitiveType::TEXT_3D;
                primitive.text          = textData.text;
                primitive.textTransform = textData.transform;
                primitive.fontSize      = textData.fontSize;
                primitive.textAlignment = textData.alignment;
                primitive.startColor    = textData.color;
                primitive.duration      = textData.duration;
                primitive.timeRemaining = textData.duration;
                primitive.isActive      = true;

                (*m_debugRenderStateBuffer->GetBackBuffer())[cmd.entityId] = primitive;
                m_debugRenderStateBuffer->MarkDirty(cmd.entityId);
                break;
            }

        case RenderCommandType::DEBUG_ADD_SCREEN_TEXT:
            {
                auto const& textData = std::get<DebugScreenTextData>(cmd.data);
                DebugPrimitive primitive;
                primitive.primitiveId   = cmd.entityId;
                primitive.type          = DebugPrimitiveType::TEXT_2D;
                primitive.text          = textData.text;
                primitive.startPos      = Vec3(textData.position.x, textData.position.y, 0.0f);
                primitive.fontSize      = textData.fontSize;
                primitive.textAlignment = textData.alignment;
                primitive.startColor    = textData.color;
                primitive.duration      = textData.duration;
                primitive.timeRemaining = textData.duration;
                primitive.isActive      = true;

                (*m_debugRenderStateBuffer->GetBackBuffer())[cmd.entityId] = primitive;
                m_debugRenderStateBuffer->MarkDirty(cmd.entityId);
                break;
            }

        case RenderCommandType::DEBUG_UPDATE_COLOR:
            {
                auto const& colorData = std::get<DebugColorUpdateData>(cmd.data);
                auto* backBuffer = m_debugRenderStateBuffer->GetBackBuffer();
                auto  it = backBuffer->find(cmd.entityId);

                if (it != backBuffer->end())
                {
                    it->second.startColor = colorData.newColor;
                    it->second.endColor   = colorData.newColor;
                    m_debugRenderStateBuffer->MarkDirty(cmd.entityId);
                }
                break;
            }

        case RenderCommandType::DEBUG_REMOVE:
            {
                auto* backBuffer = m_debugRenderStateBuffer->GetBackBuffer();
                auto  it = backBuffer->find(cmd.entityId);

                if (it != backBuffer->end())
                {
                    it->second.isActive = false;
                    m_debugRenderStateBuffer->MarkDirty(cmd.entityId);
                }
                break;
            }

        case RenderCommandType::DEBUG_CLEAR_ALL:
            m_debugRenderStateBuffer->GetBackBuffer()->clear();
            break;

        case RenderCommandType::CREATE_LIGHT:
        case RenderCommandType::UPDATE_LIGHT:
        default:
            break;
        }
    });
}

//----------------------------------------------------------------------------------------------------
void App::ProcessAudioCommands()
{
    if (!m_audioCommandQueue || !m_audioStateBuffer || !m_audioAPI)
    {
        return;
    }

    m_audioCommandQueue->ConsumeAll([this](AudioCommand const& cmd)
    {
        switch (cmd.type)
        {
        case AudioCommandType::LOAD_SOUND:
            {
                SoundLoadData const& loadData = std::get<SoundLoadData>(cmd.data);
                SoundID soundId = g_audio->CreateOrGetSound(loadData.soundPath, eAudioSystemSoundDimension::Sound3D);

                if (soundId != MISSING_SOUND_ID)
                {
                    AudioState state;
                    state.soundId   = soundId;
                    state.soundPath = loadData.soundPath;
                    state.position  = Vec3::ZERO;
                    state.volume    = 1.0f;
                    state.isPlaying = false;
                    state.isLooped  = false;
                    state.isLoaded  = true;
                    state.isActive  = true;

                    (*m_audioStateBuffer->GetBackBuffer())[soundId] = state;
                    m_audioStateBuffer->MarkDirty(soundId);
                    m_audioAPI->NotifyCallbackReady(loadData.callbackId, soundId);
                }
                else
                {
                    DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                               Stringf("ProcessAudioCommands - LOAD_SOUND failed: path=%s", loadData.soundPath.c_str()));
                    m_audioAPI->NotifyCallbackReady(loadData.callbackId, 0);
                }
                break;
            }

        case AudioCommandType::PLAY_SOUND:
            {
                SoundPlayData const& playData = std::get<SoundPlayData>(cmd.data);
                auto* backBuffer = m_audioStateBuffer->GetBackBuffer();
                auto  it = backBuffer->find(cmd.soundId);

                if (it != backBuffer->end())
                {
                    it->second.isPlaying = true;
                    it->second.volume    = playData.volume;
                    it->second.position  = playData.position;
                    it->second.isLooped  = playData.looped;

                    m_audioStateBuffer->MarkDirty(cmd.soundId);
                    g_audio->StartSoundAt(cmd.soundId, playData.position, playData.looped, playData.volume);
                }
                else
                {
                    DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                               Stringf("ProcessAudioCommands - PLAY_SOUND: soundId=%llu not found", cmd.soundId));
                }
                break;
            }

        case AudioCommandType::STOP_SOUND:
            {
                auto* backBuffer = m_audioStateBuffer->GetBackBuffer();
                auto  it = backBuffer->find(cmd.soundId);

                if (it != backBuffer->end())
                {
                    it->second.isPlaying = false;
                    m_audioStateBuffer->MarkDirty(cmd.soundId);
                    g_audio->StopSound(cmd.soundId);
                }
                else
                {
                    DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                               Stringf("ProcessAudioCommands - STOP_SOUND: soundId=%llu not found", cmd.soundId));
                }
                break;
            }

        case AudioCommandType::SET_VOLUME:
            {
                VolumeUpdateData const& volumeData = std::get<VolumeUpdateData>(cmd.data);
                auto* backBuffer = m_audioStateBuffer->GetBackBuffer();
                auto  it = backBuffer->find(cmd.soundId);

                if (it != backBuffer->end())
                {
                    it->second.volume = volumeData.volume;
                    m_audioStateBuffer->MarkDirty(cmd.soundId);
                    g_audio->SetSoundPlaybackVolume(cmd.soundId, volumeData.volume);
                }
                else
                {
                    DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                               Stringf("ProcessAudioCommands - SET_VOLUME: soundId=%llu not found", cmd.soundId));
                }
                break;
            }

        case AudioCommandType::UPDATE_3D_POSITION:
            {
                Position3DUpdateData const& positionData = std::get<Position3DUpdateData>(cmd.data);
                auto* backBuffer = m_audioStateBuffer->GetBackBuffer();
                auto  it = backBuffer->find(cmd.soundId);

                if (it != backBuffer->end())
                {
                    it->second.position = positionData.position;
                    m_audioStateBuffer->MarkDirty(cmd.soundId);
                    g_audio->SetSoundPosition(cmd.soundId, positionData.position);
                }
                else
                {
                    DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                               Stringf("ProcessAudioCommands - UPDATE_3D_POSITION: soundId=%llu not found", cmd.soundId));
                }
                break;
            }

        default:
            DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                       Stringf("ProcessAudioCommands - Unknown command type: %d", static_cast<int>(cmd.type)));
            break;
        }
    });
}

//----------------------------------------------------------------------------------------------------
// ProcessGenericCommands
//
// Consumes all pending GenericCommands from the queue and dispatches them to registered handlers
// via GenericCommandExecutor. Follows the same ConsumeAll pattern as ProcessRenderCommands and
// ProcessAudioCommands.
//
// Called from Update() on the main render thread, after ProcessAudioCommands().
//----------------------------------------------------------------------------------------------------
void App::ProcessGenericCommands()
{
    if (!m_genericCommandQueue || !m_genericCommandExecutor)
    {
        return;
    }

    m_genericCommandQueue->ConsumeAll([this](GenericCommand const& cmd)
    {
        m_genericCommandExecutor->ExecuteCommand(cmd);
    });
}

//----------------------------------------------------------------------------------------------------
void App::RenderEntities() const
{
    EntityStateMap const* frontBuffer = m_entityStateBuffer->GetFrontBuffer();
    if (!frontBuffer)
    {
        return;
    }

    // Get active camera from camera state buffer
    Camera const* worldCamera = nullptr;
    if (m_cameraStateBuffer)
    {
        EntityID activeCameraId = m_cameraStateBuffer->GetActiveCameraID();
        if (activeCameraId != 0)
        {
            worldCamera = m_cameraStateBuffer->GetCameraById(activeCameraId);
        }
    }

    // Camera creation is async, skip rendering until camera is ready
    if (!worldCamera)
    {
        return;
    }

    g_renderer->BeginCamera(*worldCamera);

    for (auto const& [entityId, state] : *frontBuffer)
    {
        if (!state.isActive) continue;
        if (state.cameraType != "world") continue;

        VertexList_PCU const* verts = m_renderResourceManager->GetVerticesForEntity(entityId);
        if (!verts || verts->empty()) continue;

        Mat44 modelMatrix;
        modelMatrix.SetTranslation3D(state.position);
        modelMatrix.Append(state.orientation.GetAsMatrix_IFwd_JLeft_KUp());

        g_renderer->SetModelConstants(modelMatrix, state.color);
        g_renderer->BindTexture(nullptr);
        g_renderer->DrawVertexArray(static_cast<int>(verts->size()), verts->data());
    }

    g_renderer->EndCamera(*worldCamera);
}

//----------------------------------------------------------------------------------------------------
void App::RenderDebugPrimitives() const
{
    DebugPrimitiveMap const* frontBuffer = m_debugRenderStateBuffer->GetFrontBuffer();
    if (!frontBuffer)
    {
        return;
    }

    for (auto const& [primitiveId, primitive] : *frontBuffer)
    {
        if (!primitive.isActive) continue;

        // Render primitive based on type (duration=0 for single-frame rendering)
        switch (primitive.type)
        {
        case DebugPrimitiveType::LINE:
            DebugAddWorldLine(primitive.startPos, primitive.endPos, primitive.radius, 0.0f,
                             primitive.startColor, primitive.endColor);
            break;

        case DebugPrimitiveType::POINT:
            DebugAddWorldPoint(primitive.startPos, primitive.radius, 0.0f,
                              primitive.startColor, primitive.startColor);
            break;

        case DebugPrimitiveType::SPHERE:
            DebugAddWorldWireSphere(primitive.startPos, primitive.radius, 0.0f,
                                   primitive.startColor, primitive.startColor);
            break;

        case DebugPrimitiveType::AABB:
            {
                Vec3 const& min = primitive.startPos;
                Vec3 const& max = primitive.endPos;
                Vec3 corners[8] = {
                    Vec3(min.x, min.y, min.z), Vec3(max.x, min.y, min.z),
                    Vec3(max.x, max.y, min.z), Vec3(min.x, max.y, min.z),
                    Vec3(min.x, min.y, max.z), Vec3(max.x, min.y, max.z),
                    Vec3(max.x, max.y, max.z), Vec3(min.x, max.y, max.z)
                };
                constexpr float kLineRadius = 0.01f;
                // Bottom face
                DebugAddWorldLine(corners[0], corners[1], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
                DebugAddWorldLine(corners[1], corners[2], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
                DebugAddWorldLine(corners[2], corners[3], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
                DebugAddWorldLine(corners[3], corners[0], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
                // Top face
                DebugAddWorldLine(corners[4], corners[5], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
                DebugAddWorldLine(corners[5], corners[6], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
                DebugAddWorldLine(corners[6], corners[7], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
                DebugAddWorldLine(corners[7], corners[4], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
                // Vertical edges
                DebugAddWorldLine(corners[0], corners[4], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
                DebugAddWorldLine(corners[1], corners[5], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
                DebugAddWorldLine(corners[2], corners[6], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
                DebugAddWorldLine(corners[3], corners[7], kLineRadius, 0.0f, primitive.startColor, primitive.startColor);
            }
            break;

        case DebugPrimitiveType::BASIS:
            {
                Mat44 basisTransform;
                basisTransform.SetIJKT3D(primitive.basisI, primitive.basisJ, primitive.basisK, primitive.startPos);
                DebugAddWorldBasis(basisTransform, 0.0f, eDebugRenderMode::USE_DEPTH);
            }
            break;

        case DebugPrimitiveType::TEXT_2D:
            DebugAddScreenText(primitive.text, Vec2(primitive.startPos.x, primitive.startPos.y),
                              primitive.fontSize, primitive.textAlignment, 0.0f,
                              primitive.startColor, primitive.startColor);
            break;

        case DebugPrimitiveType::TEXT_3D:
            DebugAddWorldText(primitive.text, primitive.textTransform, primitive.fontSize,
                             primitive.textAlignment, 0.0f, primitive.startColor,
                             primitive.startColor, eDebugRenderMode::USE_DEPTH);
            break;

        default:
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------
// Duration semantics:
//   duration = 0.0f: Render for one frame (expires after rendered once)
//   duration > 0.0f: Render for specified seconds
//   duration < 0.0f: Render permanently (never expires)
//----------------------------------------------------------------------------------------------------
void App::UpdateDebugPrimitiveExpiration(float deltaSeconds)
{
    DebugPrimitiveMap* backBuffer = m_debugRenderStateBuffer->GetBackBuffer();
    if (!backBuffer)
    {
        return;
    }

    for (auto it = backBuffer->begin(); it != backBuffer->end();)
    {
        DebugPrimitive& primitive = it->second;

        if (!primitive.isActive)
        {
            ++it;
            continue;
        }

        primitive.timeRemaining -= deltaSeconds;

        // Remove expired primitives (duration >= 0 and time expired)
        bool const shouldRemove = (primitive.duration >= 0.0f) && (primitive.timeRemaining <= 0.0f);

        if (shouldRemove)
        {
            it = backBuffer->erase(it);
        }
        else
        {
            ++it;
        }
    }
}


