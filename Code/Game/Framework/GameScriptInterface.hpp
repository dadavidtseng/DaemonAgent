//----------------------------------------------------------------------------------------------------
// GameScriptInterface.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class Game;

//----------------------------------------------------------------------------------------------------
class GameScriptInterface : public IScriptableObject
{
public:
    explicit GameScriptInterface(Game* game);

    std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
    StringList                    GetAvailableProperties() const override;

    ScriptMethodResult CallMethod(String const& methodName, ScriptArgs const& args) override;
    std::any           GetProperty(String const& propertyName) const override;
    bool               SetProperty(String const& propertyName, std::any const& value) override;

private:
    Game* m_game;

    void InitializeMethodRegistry() override;

    ScriptMethodResult ExecuteAppRequestQuit(ScriptArgs const& args);
    ScriptMethodResult ExecuteJavaScriptCommand(ScriptArgs const& args);
    ScriptMethodResult ExecuteJavaScriptFile(ScriptArgs const& args);
};
