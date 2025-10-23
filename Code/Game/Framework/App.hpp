//----------------------------------------------------------------------------------------------------
// App.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once


#include <any>

#include "Engine/Core/EventSystem.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class AudioScriptInterface;
// Phase 2b: CameraScriptInterface removed - replaced by CameraStateBuffer
// class CameraScriptInterface;
class Camera;
class ClockScriptInterface;
class DebugRenderSystemScriptInterface;
class GameScriptInterface;
class InputScriptInterface;
// Phase 2: RendererScriptInterface removed - replaced by EntityScriptInterface
// class RendererScriptInterface;
class KADIScriptInterface;
struct Rgba8;

// Phase 1: Async Architecture Forward Declarations
class RenderCommandQueue;
class EntityStateBuffer;
class CameraStateBuffer;
class JSGameLogicJob;

// Phase 2: High-Level Entity API Forward Declarations
class HighLevelEntityAPI;
class EntityScriptInterface;

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
    int CreateGeometryForMeshType(std::string const& meshType, float radius, Rgba8 const& color);

    std::shared_ptr<InputScriptInterface>             m_inputScriptInterface;
    std::shared_ptr<AudioScriptInterface>             m_audioScriptInterface;
    // Phase 2b: CameraScriptInterface removed - replaced by CameraStateBuffer
    // std::shared_ptr<CameraScriptInterface>            m_cameraScriptInterface;
    std::shared_ptr<ClockScriptInterface>             m_clockScriptInterface;
    std::shared_ptr<DebugRenderSystemScriptInterface> m_debugRenderSystemScriptInterface;
    std::shared_ptr<GameScriptInterface>              m_gameScriptInterface;
    // Phase 2: RendererScriptInterface removed - replaced by EntityScriptInterface
    // std::shared_ptr<RendererScriptInterface>          m_rendererScriptInterface;
    std::shared_ptr<KADIScriptInterface>              m_kadiScriptInterface;

    // Phase 1: Async Architecture Infrastructure
    RenderCommandQueue* m_renderCommandQueue;    // Lock-free command queue (JS → C++)
    EntityStateBuffer*  m_entityStateBuffer;     // Double-buffered entity state
    CameraStateBuffer*  m_cameraStateBuffer;     // Double-buffered camera state (Phase 2b)
    JSGameLogicJob*     m_jsGameLogicJob;        // Worker thread job for JavaScript
    Camera*             m_mainCamera;            // Main perspective camera for rendering entities

    // Phase 2: High-Level Entity API
    HighLevelEntityAPI*                    m_highLevelEntityAPI;       // High-level entity/camera management API
    std::shared_ptr<EntityScriptInterface> m_entityScriptInterface;    // JavaScript interface for entity API
};
