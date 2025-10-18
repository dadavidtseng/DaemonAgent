//----------------------------------------------------------------------------------------------------
// App.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once


#include <any>

#include "Engine/Core/EventSystem.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class AudioScriptInterface;
class CameraScriptInterface;
class Camera;
class ClockScriptInterface;
class DebugRenderSystemScriptInterface;
class GameScriptInterface;
class InputScriptInterface;
class RendererScriptInterface;
class KADIScriptInterface;

// Phase 1: Async Architecture Forward Declarations
class RenderCommandQueue;
class EntityStateBuffer;
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
    void ProcessRenderCommands();  // Consume render commands from queue (placeholder in Phase 1)

    std::shared_ptr<InputScriptInterface>             m_inputScriptInterface;
    std::shared_ptr<AudioScriptInterface>             m_audioScriptInterface;
    std::shared_ptr<CameraScriptInterface>            m_cameraScriptInterface;
    std::shared_ptr<ClockScriptInterface>             m_clockScriptInterface;
    std::shared_ptr<DebugRenderSystemScriptInterface> m_debugRenderSystemScriptInterface;
    std::shared_ptr<GameScriptInterface>              m_gameScriptInterface;
    std::shared_ptr<RendererScriptInterface>          m_rendererScriptInterface;
    std::shared_ptr<KADIScriptInterface>              m_kadiScriptInterface;

    // Phase 1: Async Architecture Infrastructure
    RenderCommandQueue* m_renderCommandQueue;    // Lock-free command queue (JS → C++)
    EntityStateBuffer*  m_entityStateBuffer;     // Double-buffered entity state
    JSGameLogicJob*     m_jsGameLogicJob;        // Worker thread job for JavaScript
    Camera*             m_mainCamera;            // Main perspective camera for rendering entities
};
