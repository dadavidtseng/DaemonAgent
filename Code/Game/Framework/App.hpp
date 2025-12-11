//----------------------------------------------------------------------------------------------------
// App.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Entity/EntityStateBuffer.hpp"
//----------------------------------------------------------------------------------------------------
#include <any>

//-Forward-Declaration--------------------------------------------------------------------------------
class AudioAPI;
class AudioCommandQueue;
class AudioScriptInterface;
class Camera;
class CallbackQueue;
class CallbackQueueScriptInterface;
class CameraAPI;
class CameraStateBuffer;
class CameraScriptInterface;
class ClockScriptInterface;
class DebugRenderAPI;
// DebugRenderStateBuffer is a type alias, not a class - must include header
#include "Engine/Renderer/DebugRenderStateBuffer.hpp"
// AudioStateBuffer is a type alias, not a class - must include header
#include "Engine/Audio/AudioStateBuffer.hpp"
class DebugRenderSystemScriptInterface;
class EntityAPI;
class EntityScriptInterface;
class GameScriptInterface;
class InputScriptInterface;
class KADIScriptInterface;
class RenderCommandQueue;
class RenderResourceManager;
class JSGameLogicJob;

//----------------------------------------------------------------------------------------------------
class App
{
public:
    App();
    ~App();

    void Startup();
    void Shutdown();
    void RunFrame();

    void RunMainLoop();

    static bool OnCloseButtonClicked(EventArgs& args);
    static void RequestQuit();
    static bool m_isQuitting;

private:
    void BeginFrame() const;
    void Update();
    void Render() const;
    void EndFrame() const;

    static std::any OnPrint(std::vector<std::any> const& args);
    static std::any OnDebug(std::vector<std::any> const& args);
    static std::any OnGarbageCollection(std::vector<std::any> const& args);

    void UpdateCursorMode();
    void SetupScriptingBindings();

    // Phase 1: Async Architecture Methods
    void ProcessRenderCommands();  // Consume render commands from queue (Phase 2: Full implementation)
    void ProcessAudioCommands();   // Consume audio commands from queue (Phase 5: Audio async pattern)

    // Phase 2: Entity Rendering
    void RenderEntities() const;  // Render all active entities from EntityStateBuffer

    // Phase 4: Debug Render
    void RenderDebugPrimitives() const;  // Render all active debug primitives from DebugRenderStateBuffer
    void UpdateDebugPrimitiveExpiration(float deltaSeconds);  // Update primitive durations and remove expired ones

    std::shared_ptr<InputScriptInterface>             m_inputScriptInterface;
    std::shared_ptr<AudioScriptInterface>             m_audioScriptInterface;
    std::shared_ptr<ClockScriptInterface>             m_clockScriptInterface;
    std::shared_ptr<DebugRenderSystemScriptInterface> m_debugRenderSystemScriptInterface;
    std::shared_ptr<GameScriptInterface>              m_gameScriptInterface;
    std::shared_ptr<KADIScriptInterface>              m_kadiScriptInterface;

    // Phase 1: Async Architecture Infrastructure
    RenderCommandQueue*                           m_renderCommandQueue = nullptr;    // Lock-free command queue (JS → C++)
    AudioCommandQueue*                            m_audioCommandQueue  = nullptr;    // Lock-free audio command queue (JS → C++) - Phase 2
    CallbackQueue*                                m_callbackQueue      = nullptr;     // Lock-free callback queue (C++ → JS)
    EntityStateBuffer*                            m_entityStateBuffer  = nullptr;     // Double-buffered entity state
    CameraStateBuffer*                            m_cameraStateBuffer  = nullptr;     // Double-buffered camera state (Phase 2b)
    JSGameLogicJob*                               m_jsGameLogicJob     = nullptr;        // Worker thread job for JavaScript
    EntityAPI*                                    m_entityAPI          = nullptr;              // Direct entity management API
    CameraAPI*                                    m_cameraAPI          = nullptr;              // Direct camera management API
    std::shared_ptr<EntityScriptInterface>        m_entityScriptInterface;  // JavaScript interface for entity API
    std::shared_ptr<CameraScriptInterface>        m_cameraScriptInterface;  // JavaScript interface for camera API
    std::shared_ptr<CallbackQueueScriptInterface> m_callbackQueueScriptInterface;  // JavaScript interface for callback queue (Phase 2.4)

    // Phase 5: Render Resource Management
    RenderResourceManager* m_renderResourceManager = nullptr;  // Manages entity → VBO mapping (separation of concerns)

    // Phase 4: Debug Render Infrastructure
    DebugRenderStateBuffer* m_debugRenderStateBuffer = nullptr;  // Double-buffered debug primitive state
    DebugRenderAPI*         m_debugRenderAPI         = nullptr;  // Async API for debug primitive submission

    // Phase 5: Audio Async Infrastructure
    AudioStateBuffer* m_audioStateBuffer = nullptr;  // Double-buffered audio state (Phase 5)
    AudioAPI*         m_audioAPI         = nullptr;  // Audio management API (Phase 5)
};
