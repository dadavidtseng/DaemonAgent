//----------------------------------------------------------------------------------------------------
// App.hpp
//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Entity/EntityStateBuffer.hpp"
#include "Engine/Renderer/DebugRenderStateBuffer.hpp"
#include "Engine/Audio/AudioStateBuffer.hpp"
//----------------------------------------------------------------------------------------------------
#include <any>

//----------------------------------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------------------------------
class CameraStateBuffer;
class CallbackQueue;
class CallbackQueueScriptInterface;
class ClockScriptInterface;
class GameScriptInterface;
class GenericCommandExecutor;
class GenericCommandQueue;
class GenericCommandScriptInterface;
class InputScriptInterface;
class JSGameLogicJob;
class KADIScriptInterface;
class RenderResourceManager;

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

    // Command Processing
    void ProcessGenericCommands();

    // Rendering
    void RenderEntities() const;
    // void RenderDebugPrimitives() const;
    // void UpdateDebugPrimitiveExpiration(float deltaSeconds);

    //------------------------------------------------------------------------------------------------
    // Script Interfaces
    //------------------------------------------------------------------------------------------------
    std::shared_ptr<InputScriptInterface>             m_inputScriptInterface;
    std::shared_ptr<ClockScriptInterface>             m_clockScriptInterface;
    std::shared_ptr<GameScriptInterface>              m_gameScriptInterface;
    std::shared_ptr<KADIScriptInterface>              m_kadiScriptInterface;
    std::shared_ptr<CallbackQueueScriptInterface>     m_callbackQueueScriptInterface;
    std::shared_ptr<GenericCommandScriptInterface>    m_genericCommandScriptInterface;

    //------------------------------------------------------------------------------------------------
    // Async Architecture Infrastructure
    //------------------------------------------------------------------------------------------------
    CallbackQueue*          m_callbackQueue          = nullptr;
    GenericCommandQueue*    m_genericCommandQueue    = nullptr;
    GenericCommandExecutor* m_genericCommandExecutor = nullptr;
    JSGameLogicJob*         m_jsGameLogicJob         = nullptr;

    //------------------------------------------------------------------------------------------------
    // State Buffers (Double-buffered for async updates)
    //------------------------------------------------------------------------------------------------
    EntityStateBuffer*      m_entityStateBuffer      = nullptr;
    CameraStateBuffer*      m_cameraStateBuffer      = nullptr;
    // DebugRenderStateBuffer* m_debugRenderStateBuffer = nullptr;
    AudioStateBuffer*       m_audioStateBuffer       = nullptr;

    //------------------------------------------------------------------------------------------------
    // APIs (Direct management interfaces)
    //------------------------------------------------------------------------------------------------
    RenderResourceManager* m_renderResourceManager = nullptr;
};
