//----------------------------------------------------------------------------------------------------
// Game.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Entity/EntityStateBuffer.hpp"
#include "Engine/Script/IJSGameLogicContext.hpp"

//----------------------------------------------------------------------------------------------------
class Game : public IJSGameLogicContext
{
public:
    Game();
    ~Game();

    void PostInit();
    void UpdateJS();
    void RenderJS();
    void ShowSimpleDemoWindow();
    // Game state queries
    bool IsAttractMode() const;

    // JavaScript callback functions
    void ExecuteJavaScriptCommand(String const& command);
    void ExecuteJavaScriptFile(String const& filename);
    void ExecuteModuleFile(String const& modulePath);

    //------------------------------------------------------------------------------------------------
    // IJSGameLogicContext Interface Implementation (Worker Thread)
    //------------------------------------------------------------------------------------------------

    // Execute JavaScript update logic on worker thread
    // Called by JSGameLogicJob from worker thread
    void UpdateJSWorkerThread(float deltaTime, EntityStateBuffer* entityBuffer, RenderCommandQueue* commandQueue) override;

    // Execute JavaScript render logic on worker thread
    // Called by JSGameLogicJob from worker thread
    void RenderJSWorkerThread(float deltaTime, CameraStateBuffer* cameraBuffer, RenderCommandQueue* commandQueue) override;

    // Handle JavaScript exception from worker thread
    // Called by JSGameLogicJob when JavaScript errors occur
    void HandleJSException(char const* errorMessage, char const* stackTrace) override;

private:
    void InitializeJavaScriptFramework();

    bool m_showDemoWindow = true;
};
