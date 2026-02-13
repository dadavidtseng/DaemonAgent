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
#include "Engine/Entity/EntityStateBuffer.hpp"
#include "Engine/Input/InputScriptInterface.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Network/KADIScriptInterface.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/CameraAPI.hpp"
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

    // === GenericCommand handler: "create_camera" (Task 8.4 — CameraScriptInterface migration) ===
    // Replaces CameraScriptInterface::ExecuteCreateCamera with GenericCommand pipeline.
    // Handler generates cameraId immediately; render command queued for async GPU-side creation.
    // Note: callbackId in CameraCreationData set to 0 — GenericCommand pipeline handles callbacks.
    m_genericCommandExecutor->RegisterHandler("create_camera",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            // Extract position [x, y, z] — default [0, 0, 0]
            auto posArr = json.value("position", std::vector<double>{0.0, 0.0, 0.0});
            Vec3 position(
                static_cast<float>(posArr.size() > 0 ? posArr[0] : 0.0),
                static_cast<float>(posArr.size() > 1 ? posArr[1] : 0.0),
                static_cast<float>(posArr.size() > 2 ? posArr[2] : 0.0)
            );

            // Extract orientation [yaw, pitch, roll] — default [0, 0, 0]
            auto oriArr = json.value("orientation", std::vector<double>{0.0, 0.0, 0.0});
            EulerAngles orientation(
                static_cast<float>(oriArr.size() > 0 ? oriArr[0] : 0.0),
                static_cast<float>(oriArr.size() > 1 ? oriArr[1] : 0.0),
                static_cast<float>(oriArr.size() > 2 ? oriArr[2] : 0.0)
            );

            String type = json.value("type", "world");

            // Generate camera ID (same as CameraAPI::CreateCamera)
            EntityID cameraId = m_cameraAPI->GenerateCameraID();

            // Build render command (same as CameraAPI::CreateCamera)
            CameraCreationData cameraData;
            cameraData.position    = position;
            cameraData.orientation = orientation;
            cameraData.type        = type;
            cameraData.callbackId  = 0;  // GenericCommand pipeline handles callbacks, bypass CameraAPI's callback system

            RenderCommand command(RenderCommandType::CREATE_CAMERA, cameraId, cameraData);
            bool submitted = m_renderCommandQueue->Submit(command);

            if (!submitted)
            {
                DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                           Stringf("GenericCommand [create_camera]: RenderCommandQueue full, camera %llu dropped", cameraId));
                return HandlerResult::Error("ERR_QUEUE_FULL: render command queue is full");
            }

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [create_camera]: cameraId=%llu, type=%s, pos=(%.1f,%.1f,%.1f)",
                           cameraId, type.c_str(), position.x, position.y, position.z));

            return HandlerResult::Success({{"resultId", std::any(static_cast<uint64_t>(cameraId))}});
        });

    // === GenericCommand handler: "set_active_camera" (Task 8.4) ===
    // Replaces CameraScriptInterface::ExecuteSetActiveCamera.
    // Callback marked ready immediately (same as CameraAPI::SetActiveCamera).
    m_genericCommandExecutor->RegisterHandler("set_active_camera",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            EntityID cameraId = json.value("cameraId", static_cast<uint64_t>(0));
            if (cameraId == 0)
            { return HandlerResult::Error("ERR_INVALID_PARAM: cameraId is required and must be non-zero"); }

            RenderCommand command(RenderCommandType::SET_ACTIVE_CAMERA, cameraId, std::monostate{});
            bool submitted = m_renderCommandQueue->Submit(command);

            if (!submitted)
            {
                DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                           Stringf("GenericCommand [set_active_camera]: RenderCommandQueue full, camera %llu dropped", cameraId));
                return HandlerResult::Error("ERR_QUEUE_FULL: render command queue is full");
            }

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [set_active_camera]: cameraId=%llu", cameraId));

            return HandlerResult::Success({{"resultId", std::any(static_cast<uint64_t>(cameraId))}});
        });

    // === GenericCommand handler: "update_camera_type" (Task 8.4) ===
    // Replaces CameraScriptInterface::ExecuteUpdateCameraType.
    // Callback marked ready immediately (same as CameraAPI::UpdateCameraType).
    m_genericCommandExecutor->RegisterHandler("update_camera_type",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            EntityID cameraId = json.value("cameraId", static_cast<uint64_t>(0));
            if (cameraId == 0)
            { return HandlerResult::Error("ERR_INVALID_PARAM: cameraId is required and must be non-zero"); }

            String type = json.value("type", "");
            if (type.empty())
            { return HandlerResult::Error("ERR_INVALID_PARAM: type is required"); }

            CameraTypeUpdateData typeUpdateData;
            typeUpdateData.type = type;

            RenderCommand command(RenderCommandType::UPDATE_CAMERA_TYPE, cameraId, typeUpdateData);
            bool submitted = m_renderCommandQueue->Submit(command);

            if (!submitted)
            {
                DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                           Stringf("GenericCommand [update_camera_type]: RenderCommandQueue full, camera %llu dropped", cameraId));
                return HandlerResult::Error("ERR_QUEUE_FULL: render command queue is full");
            }

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [update_camera_type]: cameraId=%llu, type=%s", cameraId, type.c_str()));

            return HandlerResult::Success({{"resultId", std::any(static_cast<uint64_t>(cameraId))}});
        });

    // === GenericCommand handler: "destroy_camera" (Task 8.4) ===
    // Replaces CameraScriptInterface::ExecuteDestroyCamera.
    // Callback marked ready immediately (same as CameraAPI::DestroyCamera).
    m_genericCommandExecutor->RegisterHandler("destroy_camera",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            EntityID cameraId = json.value("cameraId", static_cast<uint64_t>(0));
            if (cameraId == 0)
            { return HandlerResult::Error("ERR_INVALID_PARAM: cameraId is required and must be non-zero"); }

            RenderCommand command(RenderCommandType::DESTROY_CAMERA, cameraId, std::monostate{});
            bool submitted = m_renderCommandQueue->Submit(command);

            if (!submitted)
            {
                DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                           Stringf("GenericCommand [destroy_camera]: RenderCommandQueue full, camera %llu dropped", cameraId));
                return HandlerResult::Error("ERR_QUEUE_FULL: render command queue is full");
            }

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [destroy_camera]: cameraId=%llu", cameraId));

            return HandlerResult::Success({{"resultId", std::any(static_cast<uint64_t>(cameraId))}});
        });

    // === GenericCommand handler: "load_sound" (Task 8.2 — AudioScriptInterface migration) ===
    // Replaces AudioScriptInterface::ExecuteLoadSoundAsync + ProcessAudioCommands LOAD_SOUND path.
    // Handler runs on main thread; calls g_audio->CreateOrGetSound() directly, updates AudioStateBuffer.
    m_genericCommandExecutor->RegisterHandler("load_sound",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            String soundPath = json.value("soundPath", "");
            if (soundPath.empty())
            { return HandlerResult::Error("ERR_INVALID_PARAM: soundPath is required"); }

            // Dimension: default Sound2D (2D is always audible; use Sound3D explicitly for spatial audio)
            String dimStr = json.value("dimension", "Sound2D");
            eAudioSystemSoundDimension dimension = eAudioSystemSoundDimension::Sound3D;
            if (dimStr == "Sound2D" || dimStr == "2D")
            { dimension = eAudioSystemSoundDimension::Sound2D; }

            SoundID soundId = g_audio->CreateOrGetSound(soundPath, dimension);

            if (soundId == MISSING_SOUND_ID)
            {
                DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                           Stringf("GenericCommand [load_sound]: failed to load '%s'", soundPath.c_str()));
                return HandlerResult::Error("ERR_LOAD_FAILED: sound file not found or invalid");
            }

            // Update AudioStateBuffer (same as ProcessAudioCommands LOAD_SOUND)
            AudioState state;
            state.soundId   = soundId;
            state.soundPath = soundPath;
            state.position  = Vec3::ZERO;
            state.volume    = 1.0f;
            state.isPlaying = false;
            state.isLooped  = false;
            state.isLoaded  = true;
            state.isActive  = true;

            (*m_audioStateBuffer->GetBackBuffer())[soundId] = state;
            m_audioStateBuffer->MarkDirty(soundId);

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [load_sound]: soundId=%llu, path=%s", soundId, soundPath.c_str()));

            return HandlerResult::Success({{"resultId", std::any(static_cast<uint64_t>(soundId))}});
        });

    // === GenericCommand handler: "play_sound" (Task 8.2) ===
    // Replaces AudioScriptInterface::ExecutePlaySoundAsync + ProcessAudioCommands PLAY_SOUND path.
    m_genericCommandExecutor->RegisterHandler("play_sound",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("soundId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: soundId is required"); }
            SoundID soundId = json.value("soundId", static_cast<uint64_t>(0));

            float volume = json.value("volume", 1.0f);
            bool  looped = json.value("looped", false);

            // Verify sound exists in AudioStateBuffer
            auto* backBuffer = m_audioStateBuffer->GetBackBuffer();
            auto  it = backBuffer->find(soundId);
            if (it == backBuffer->end())
            {
                return HandlerResult::Error(Stringf("ERR_NOT_FOUND: soundId %llu not in AudioStateBuffer", soundId));
            }

            // 2D vs 3D: match the playback mode to how the sound was loaded.
            // Sound2D (FMOD_DEFAULT) → StartSound (no FMOD_3D flag)
            // Sound3D (FMOD_3D)      → StartSoundAt (with 3D position)
            SoundPlaybackID playbackId = MISSING_SOUND_ID;
            bool const has3DPosition = json.contains("position");

            if (has3DPosition)
            {
                auto posArr = json.value("position", std::vector<double>{0.0, 0.0, 0.0});
                Vec3 position(
                    static_cast<float>(posArr.size() > 0 ? posArr[0] : 0.0),
                    static_cast<float>(posArr.size() > 1 ? posArr[1] : 0.0),
                    static_cast<float>(posArr.size() > 2 ? posArr[2] : 0.0)
                );
                it->second.position = position;
                playbackId = g_audio->StartSoundAt(soundId, position, looped, volume);
            }
            else
            {
                // 2D playback — matches Sound2D (FMOD_DEFAULT) load mode.
                // This is the same path as the original synchronous AudioInterface.startSound().
                playbackId = g_audio->StartSound(soundId, looped, volume);
            }

            // Update state
            it->second.isPlaying = true;
            it->second.volume    = volume;
            it->second.isLooped  = looped;
            m_audioStateBuffer->MarkDirty(soundId);

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [play_sound]: soundId=%llu, playbackId=%llu, vol=%.2f, looped=%d, 3D=%d",
                               soundId, playbackId, volume, looped, has3DPosition));

            return HandlerResult::Success({{"resultId", std::any(static_cast<uint64_t>(playbackId))}});
        });

    // === GenericCommand handler: "stop_sound" (Task 8.2) ===
    // Replaces AudioScriptInterface::ExecuteStopSoundAsync + ProcessAudioCommands STOP_SOUND path.
    m_genericCommandExecutor->RegisterHandler("stop_sound",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("soundId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: soundId is required"); }
            SoundID soundId = json.value("soundId", static_cast<uint64_t>(0));

            auto* backBuffer = m_audioStateBuffer->GetBackBuffer();
            auto  it = backBuffer->find(soundId);
            if (it != backBuffer->end())
            {
                it->second.isPlaying = false;
                m_audioStateBuffer->MarkDirty(soundId);
            }

            g_audio->StopSound(soundId);

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [stop_sound]: soundId=%llu", soundId));

            return HandlerResult::Success({{"resultId", std::any(static_cast<uint64_t>(soundId))}});
        });

    // === GenericCommand handler: "set_volume" (Task 8.2) ===
    // Replaces AudioScriptInterface::ExecuteSetVolumeAsync + ProcessAudioCommands SET_VOLUME path.
    m_genericCommandExecutor->RegisterHandler("set_volume",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("soundId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: soundId is required"); }
            SoundID soundId = json.value("soundId", static_cast<uint64_t>(0));

            float volume = json.value("volume", 1.0f);

            auto* backBuffer = m_audioStateBuffer->GetBackBuffer();
            auto  it = backBuffer->find(soundId);
            if (it != backBuffer->end())
            {
                it->second.volume = volume;
                m_audioStateBuffer->MarkDirty(soundId);
            }

            g_audio->SetSoundPlaybackVolume(soundId, volume);

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [set_volume]: soundId=%llu, volume=%.2f", soundId, volume));

            return HandlerResult::Success({{"resultId", std::any(static_cast<uint64_t>(soundId))}});
        });

    // === GenericCommand handler: "update_3d_position" (Task 8.2) ===
    // Replaces AudioScriptInterface::ExecuteUpdate3DPositionAsync + ProcessAudioCommands UPDATE_3D_POSITION path.
    m_genericCommandExecutor->RegisterHandler("update_3d_position",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("soundId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: soundId is required"); }
            SoundID soundId = json.value("soundId", static_cast<uint64_t>(0));

            auto posArr = json.value("position", std::vector<double>{0.0, 0.0, 0.0});
            Vec3 position(
                static_cast<float>(posArr.size() > 0 ? posArr[0] : 0.0),
                static_cast<float>(posArr.size() > 1 ? posArr[1] : 0.0),
                static_cast<float>(posArr.size() > 2 ? posArr[2] : 0.0)
            );

            auto* backBuffer = m_audioStateBuffer->GetBackBuffer();
            auto  it = backBuffer->find(soundId);
            if (it != backBuffer->end())
            {
                it->second.position = position;
                m_audioStateBuffer->MarkDirty(soundId);
            }

            g_audio->SetSoundPosition(soundId, position);

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [update_3d_position]: soundId=%llu, pos=(%.1f,%.1f,%.1f)",
                           soundId, position.x, position.y, position.z));

            return HandlerResult::Success({{"resultId", std::any(static_cast<uint64_t>(soundId))}});
        });

    // === GenericCommand handler: "load_texture" (Task 8.1 — ResourceScriptInterface migration) ===
    // Replaces ResourceScriptInterface::ExecuteLoadTexture with GenericCommand pipeline.
    // Handler runs on main thread; calls g_resourceSubsystem->CreateOrGetTextureFromFile() directly.
    // Returns opaque resourceId (Texture* cast to uint64_t) matching ResourceLoadJob pattern.
    m_genericCommandExecutor->RegisterHandler("load_texture",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            String path = json.value("path", "");
            if (path.empty())
            { return HandlerResult::Error("ERR_INVALID_PARAM: path is required"); }

            if (!g_resourceSubsystem)
            { return HandlerResult::Error("ERR_NOT_INITIALIZED: ResourceSubsystem is null"); }

            Texture* texture = g_resourceSubsystem->CreateOrGetTextureFromFile(path);
            if (!texture)
            {
                DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                           Stringf("GenericCommand [load_texture]: failed to load '%s'", path.c_str()));
                return HandlerResult::Error(Stringf("ERR_LOAD_FAILED: texture not found or invalid: %s", path.c_str()));
            }

            uint64_t resourceId = reinterpret_cast<uint64_t>(texture);

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [load_texture]: resourceId=%llu, path=%s", resourceId, path.c_str()));

            return HandlerResult::Success({{"resultId", std::any(resourceId)}});
        });

    // === GenericCommand handler: "load_model" (Task 8.1 — ResourceScriptInterface migration) ===
    // Replaces ResourceScriptInterface::ExecuteLoadModel with GenericCommand pipeline.
    // NOTE: ResourceSubsystem does not yet have CreateOrGetModelFromFile().
    // Returns error until model loading is implemented (matches ResourceLoadJob behavior).
    m_genericCommandExecutor->RegisterHandler("load_model",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            String path = json.value("path", "");
            if (path.empty())
            { return HandlerResult::Error("ERR_INVALID_PARAM: path is required"); }

            // Model loading via ResourceSubsystem not yet implemented
            DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                       Stringf("GenericCommand [load_model]: not yet implemented for '%s'", path.c_str()));

            return HandlerResult::Error(Stringf("ERR_NOT_IMPLEMENTED: model loading not yet supported: %s", path.c_str()));
        });

    // === GenericCommand handler: "load_shader" (Task 8.1 — ResourceScriptInterface migration) ===
    // Replaces ResourceScriptInterface::ExecuteLoadShader with GenericCommand pipeline.
    // Handler runs on main thread; calls g_resourceSubsystem->CreateOrGetShaderFromFile() directly.
    // Returns opaque resourceId (Shader* cast to uint64_t) matching ResourceLoadJob pattern.
    m_genericCommandExecutor->RegisterHandler("load_shader",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            String path = json.value("path", "");
            if (path.empty())
            { return HandlerResult::Error("ERR_INVALID_PARAM: path is required"); }

            if (!g_resourceSubsystem)
            { return HandlerResult::Error("ERR_NOT_INITIALIZED: ResourceSubsystem is null"); }

            Shader* shader = g_resourceSubsystem->CreateOrGetShaderFromFile(path);
            if (!shader)
            {
                DAEMON_LOG(LogApp, eLogVerbosity::Warning,
                           Stringf("GenericCommand [load_shader]: failed to load '%s'", path.c_str()));
                return HandlerResult::Error(Stringf("ERR_LOAD_FAILED: shader not found or invalid: %s", path.c_str()));
            }

            uint64_t resourceId = reinterpret_cast<uint64_t>(shader);

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [load_shader]: resourceId=%llu, path=%s", resourceId, path.c_str()));

            return HandlerResult::Success({{"resultId", std::any(resourceId)}});
        });

    // === GenericCommand handler: "entity.update_position" (Task 9.1.1 — EntityAPI fire-and-forget migration) ===
    // Replaces EntityScriptInterface::ExecuteUpdatePosition with GenericCommand pipeline.
    // Fire-and-forget: no callback needed, position applied immediately on main thread.
    // Flow: JS submit("entity.update_position", {entityId, x, y, z}) → handler → EntityAPI::UpdatePosition()
    m_genericCommandExecutor->RegisterHandler("entity.update_position",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            // Extract entityId (required)
            if (!json.contains("entityId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: entityId is required"); }
            uint64_t entityId = json.value("entityId", static_cast<uint64_t>(0));

            // Extract position components (required)
            double x = json.value("x", 0.0);
            double y = json.value("y", 0.0);
            double z = json.value("z", 0.0);

            Vec3 position(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));

            m_entityAPI->UpdatePosition(entityId, position);

            // Fire-and-forget: no callback result needed
            return HandlerResult::Success();
        });

    // === GenericCommand handler: "entity.move_by" (Task 9.1.2 — EntityAPI fire-and-forget migration) ===
    // Replaces EntityScriptInterface::ExecuteMoveBy with GenericCommand pipeline.
    // Fire-and-forget: relative movement applied immediately on main thread.
    // Flow: JS submit("entity.move_by", {entityId, dx, dy, dz}) → handler → EntityAPI::MoveBy()
    m_genericCommandExecutor->RegisterHandler("entity.move_by",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            // Extract entityId (required)
            if (!json.contains("entityId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: entityId is required"); }
            uint64_t entityId = json.value("entityId", static_cast<uint64_t>(0));

            // Extract delta components
            double dx = json.value("dx", 0.0);
            double dy = json.value("dy", 0.0);
            double dz = json.value("dz", 0.0);

            Vec3 delta(static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz));

            m_entityAPI->MoveBy(entityId, delta);

            // Fire-and-forget: no callback result needed
            return HandlerResult::Success();
        });

    // === GenericCommand handler: "entity.update_orientation" (Task 9.1.3) ===
    // Fire-and-forget: orientation applied immediately on main thread.
    m_genericCommandExecutor->RegisterHandler("entity.update_orientation",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("entityId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: entityId is required"); }
            uint64_t entityId = json.value("entityId", static_cast<uint64_t>(0));

            double yaw   = json.value("yaw",   0.0);
            double pitch = json.value("pitch", 0.0);
            double roll  = json.value("roll",  0.0);

            EulerAngles orientation(static_cast<float>(yaw), static_cast<float>(pitch), static_cast<float>(roll));

            m_entityAPI->UpdateOrientation(entityId, orientation);

            return HandlerResult::Success();
        });

    // === GenericCommand handler: "entity.update_color" (Task 9.1.4) ===
    // Fire-and-forget: color applied immediately on main thread.
    m_genericCommandExecutor->RegisterHandler("entity.update_color",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("entityId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: entityId is required"); }
            uint64_t entityId = json.value("entityId", static_cast<uint64_t>(0));

            int r = json.value("r", 255);
            int g = json.value("g", 255);
            int b = json.value("b", 255);
            int a = json.value("a", 255);

            Rgba8 color(static_cast<unsigned char>(r),
                        static_cast<unsigned char>(g),
                        static_cast<unsigned char>(b),
                        static_cast<unsigned char>(a));

            m_entityAPI->UpdateColor(entityId, color);

            return HandlerResult::Success();
        });

    // === GenericCommand handler: "entity.destroy" (Task 9.1.5) ===
    // Lifecycle operation with optional callback for confirmation.
    m_genericCommandExecutor->RegisterHandler("entity.destroy",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("entityId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: entityId is required"); }
            uint64_t entityId = json.value("entityId", static_cast<uint64_t>(0));

            m_entityAPI->DestroyEntity(entityId);

            DAEMON_LOG(LogApp, eLogVerbosity::Log,
                       Stringf("GenericCommand [entity.destroy]: entityId=%llu", entityId));

            return HandlerResult::Success({{"resultId", std::any(entityId)}});
        });

    // === GenericCommand handler: "camera.update" (Task 9.2.2 — CameraAPI fire-and-forget migration) ===
    // Atomic position+orientation update. Replaces direct C++ camera.update() call.
    // Flow: JS submit("camera.update", {cameraId, posX, posY, posZ, yaw, pitch, roll}) → handler → CameraAPI::UpdateCamera()
    m_genericCommandExecutor->RegisterHandler("camera.update",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("cameraId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: cameraId is required"); }
            uint64_t cameraId = json.value("cameraId", static_cast<uint64_t>(0));

            float posX  = json.value("posX", 0.0f);
            float posY  = json.value("posY", 0.0f);
            float posZ  = json.value("posZ", 0.0f);
            float yaw   = json.value("yaw", 0.0f);
            float pitch = json.value("pitch", 0.0f);
            float roll  = json.value("roll", 0.0f);

            m_cameraAPI->UpdateCamera(cameraId, Vec3(posX, posY, posZ), EulerAngles(yaw, pitch, roll));

            return HandlerResult::Success();
        });

    // === GenericCommand handler: "camera.update_position" (Task 9.2.3) ===
    // DEPRECATED: Use camera.update for atomic updates. Kept for backward compatibility.
    m_genericCommandExecutor->RegisterHandler("camera.update_position",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("cameraId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: cameraId is required"); }
            uint64_t cameraId = json.value("cameraId", static_cast<uint64_t>(0));

            float x = json.value("x", 0.0f);
            float y = json.value("y", 0.0f);
            float z = json.value("z", 0.0f);

            m_cameraAPI->UpdateCameraPosition(cameraId, Vec3(x, y, z));

            return HandlerResult::Success();
        });

    // === GenericCommand handler: "camera.update_orientation" (Task 9.2.4) ===
    // DEPRECATED: Use camera.update for atomic updates. Kept for backward compatibility.
    m_genericCommandExecutor->RegisterHandler("camera.update_orientation",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("cameraId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: cameraId is required"); }
            uint64_t cameraId = json.value("cameraId", static_cast<uint64_t>(0));

            float yaw   = json.value("yaw", 0.0f);
            float pitch = json.value("pitch", 0.0f);
            float roll  = json.value("roll", 0.0f);

            m_cameraAPI->UpdateCameraOrientation(cameraId, EulerAngles(yaw, pitch, roll));

            return HandlerResult::Success();
        });

    // === GenericCommand handler: "camera.move_by" (Task 9.2.5) ===
    // Relative camera movement by delta vector.
    m_genericCommandExecutor->RegisterHandler("camera.move_by",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("cameraId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: cameraId is required"); }
            uint64_t cameraId = json.value("cameraId", static_cast<uint64_t>(0));

            float dx = json.value("dx", 0.0f);
            float dy = json.value("dy", 0.0f);
            float dz = json.value("dz", 0.0f);

            m_cameraAPI->MoveCameraBy(cameraId, Vec3(dx, dy, dz));

            return HandlerResult::Success();
        });

    // === GenericCommand handler: "camera.look_at" (Task 9.2.6) ===
    // Make camera look at target position. (Phase 2 stub in CameraAPI — will work when implemented.)
    m_genericCommandExecutor->RegisterHandler("camera.look_at",
        [this](std::any const& payload) -> HandlerResult
        {
            String payloadStr;
            try { payloadStr = std::any_cast<String>(payload); }
            catch (std::bad_any_cast const&)
            { return HandlerResult::Error("ERR_INVALID_PAYLOAD: expected JSON string"); }

            nlohmann::json json;
            try { json = nlohmann::json::parse(payloadStr); }
            catch (nlohmann::json::exception const& e)
            { return HandlerResult::Error(Stringf("ERR_JSON_PARSE: %s", e.what())); }

            if (!json.contains("cameraId"))
            { return HandlerResult::Error("ERR_INVALID_PARAM: cameraId is required"); }
            uint64_t cameraId = json.value("cameraId", static_cast<uint64_t>(0));

            float targetX = json.value("targetX", 0.0f);
            float targetY = json.value("targetY", 0.0f);
            float targetZ = json.value("targetZ", 0.0f);

            m_cameraAPI->LookAtCamera(cameraId, Vec3(targetX, targetY, targetZ));

            return HandlerResult::Success();
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

    m_clockScriptInterface = std::make_shared<ClockScriptInterface>();
    g_scriptSubsystem->RegisterScriptableObject("clock", m_clockScriptInterface);

#ifdef ENGINE_SCRIPTING_ENABLED
    // Configure AudioCommandQueue for async JavaScript audio operations
    if (m_audioCommandQueue && m_callbackQueue && g_audio)
    {
        g_audio->SetCommandQueue(m_audioCommandQueue, m_callbackQueue);
    }
#endif

    // Register debug render script interface
    if (m_debugRenderAPI)
    {
        m_debugRenderSystemScriptInterface = std::make_shared<DebugRenderSystemScriptInterface>(m_debugRenderAPI, m_cameraAPI);
        g_scriptSubsystem->RegisterScriptableObject("debugRenderInterface", m_debugRenderSystemScriptInterface);
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


