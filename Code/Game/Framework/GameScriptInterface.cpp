//----------------------------------------------------------------------------------------------------
// GameScriptInterface.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Framework/GameScriptInterface.hpp"
//----------------------------------------------------------------------------------------------------
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Player.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Script/ScriptTypeExtractor.hpp"

//----------------------------------------------------------------------------------------------------
GameScriptInterface::GameScriptInterface(Game* game)
    : m_game(game)
{
    if (!g_game)
    {
        ERROR_AND_DIE("GameScriptInterface: Game pointer cannot be null")
    }

    GameScriptInterface::InitializeMethodRegistry();
}

//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> GameScriptInterface::GetAvailableMethods() const
{
    return {
        ScriptMethodInfo("pauseGameClock",
                         "Pause game clock",
                         {},
                         "void"),

        ScriptMethodInfo("appRequestQuit",
                         "Request quit to app",
                         {},
                         "void"),

        ScriptMethodInfo("update",
                         "JavaScript GameLoop Update",
                         {"float", "float"},
                         "void"),

        ScriptMethodInfo("render",
                         "JavaScript GameLoop Render",
                         {},
                         "void"),
    };
}

//----------------------------------------------------------------------------------------------------
std::vector<String> GameScriptInterface::GetAvailableProperties() const
{
    return {
        "attractMode",
        "gameState"
    };
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::CallMethod(String const&     methodName,
                                                   ScriptArgs const& args)
{
    try
    {
        if (methodName == "pauseGameClock")
        {
            return ExecutePauseGameClock(args);
        }
        if (methodName == "appRequestQuit")
        {
            return ExecuteAppRequestQuit(args);
        }
        else if (methodName == "update")
        {
            return ExecuteUpdate(args);
        }
        else if (methodName == "render")
        {
            return ExecuteRender(args);
        }

        return ScriptMethodResult::Error("未知的方法: " + methodName);
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("方法執行時發生例外: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
std::any GameScriptInterface::GetProperty(const String& propertyName) const
{
    UNUSED(propertyName)

    return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool GameScriptInterface::SetProperty(const String& propertyName, const std::any& value)
{
    UNUSED(propertyName)
    UNUSED(value)

    return false;
}

void GameScriptInterface::InitializeMethodRegistry()
{
}

ScriptMethodResult GameScriptInterface::ExecutePauseGameClock(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "pauseGameClock");
    if (!result.success) return result;

    try
    {
        m_game->GetClock()->TogglePause();
        return ScriptMethodResult::Success();
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("PauseGameClock: " + String(e.what()));
    }
}

ScriptMethodResult GameScriptInterface::ExecuteAppRequestQuit(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "appRequestQuit");
    if (!result.success) return result;

    try
    {
        App::RequestQuit();
        return ScriptMethodResult::Success();
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("創建立方體失敗: " + String(e.what()));
    }
}

ScriptMethodResult GameScriptInterface::ExecuteRender(const ScriptArgs& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "Render");
    if (!result.success) return result;

    try
    {
        m_game->Render();
        return ScriptMethodResult::Success(Stringf("Render Success"));
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Render failed: " + String(e.what()));
    }
}

ScriptMethodResult GameScriptInterface::ExecuteUpdate(const ScriptArgs& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "Update");
    if (!result.success) return result;

    try
    {
        float gameDeltaSeconds   = ScriptTypeExtractor::ExtractFloat(args[0]);
        float systemDeltaSeconds = ScriptTypeExtractor::ExtractFloat(args[1]);

        m_game->Update(gameDeltaSeconds, systemDeltaSeconds);
        return ScriptMethodResult::Success(Stringf("Update Success"));
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Update failed: " + String(e.what()));
    }
}