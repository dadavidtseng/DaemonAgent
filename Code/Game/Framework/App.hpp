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
class AudioScriptInterface;
class Camera;
class CameraAPI;
class CameraStateBuffer;
class CameraScriptInterface;
class ClockScriptInterface;
class DebugRenderSystemScriptInterface;
class EntityAPI;
class EntityScriptInterface;
class GameScriptInterface;
class InputScriptInterface;
class KADIScriptInterface;
class RenderCommandQueue;
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

    // Phase 2: Entity Rendering
    void RenderEntities() const;  // Render all active entities from EntityStateBuffer

    // Phase 2: Geometry Creation Helper
    int CreateGeometryForMeshType(String const& meshType, float radius, Rgba8 const& color);

    std::shared_ptr<InputScriptInterface>             m_inputScriptInterface;
    std::shared_ptr<AudioScriptInterface>             m_audioScriptInterface;
    std::shared_ptr<ClockScriptInterface>             m_clockScriptInterface;
    std::shared_ptr<DebugRenderSystemScriptInterface> m_debugRenderSystemScriptInterface;
    std::shared_ptr<GameScriptInterface>              m_gameScriptInterface;
    std::shared_ptr<KADIScriptInterface>              m_kadiScriptInterface;

    // Phase 1: Async Architecture Infrastructure
    RenderCommandQueue*                    m_renderCommandQueue = nullptr;    // Lock-free command queue (JS → C++)
    EntityStateBuffer*                     m_entityStateBuffer  = nullptr;     // Double-buffered entity state
    CameraStateBuffer*                     m_cameraStateBuffer  = nullptr;     // Double-buffered camera state (Phase 2b)
    JSGameLogicJob*                        m_jsGameLogicJob     = nullptr;        // Worker thread job for JavaScript
    EntityAPI*                             m_entityAPI          = nullptr;              // Direct entity management API
    CameraAPI*                             m_cameraAPI          = nullptr;              // Direct camera management API
    std::shared_ptr<EntityScriptInterface> m_entityScriptInterface;  // JavaScript interface for entity API
    std::shared_ptr<CameraScriptInterface> m_cameraScriptInterface;  // JavaScript interface for camera API
};
