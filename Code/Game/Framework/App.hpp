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
class ClockScriptInterface;
class GameScriptInterface;
class InputScriptInterface;
class RendererScriptInterface;

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

    std::shared_ptr<GameScriptInterface>     m_gameScriptInterface;
    std::shared_ptr<InputScriptInterface>    m_inputScriptInterface;
    std::shared_ptr<AudioScriptInterface>    m_audioScriptInterface;
    std::shared_ptr<CameraScriptInterface>   m_cameraScriptInterface;
    std::shared_ptr<RendererScriptInterface> m_rendererScriptInterface;
    std::shared_ptr<ClockScriptInterface>    m_clockScriptInterface;
};
