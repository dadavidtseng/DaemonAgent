//----------------------------------------------------------------------------------------------------
// Game.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
class Game
{
public:
    Game();
    ~Game();

    void PostInit();
    void UpdateJS();
    void RenderJS();

    // JavaScript callback functions
    void ExecuteJavaScriptCommand(String const& command);
    void ExecuteJavaScriptFile(String const& filename);
    void ExecuteModuleFile(String const& modulePath);

private:
    void InitializeJavaScriptFramework();
};
