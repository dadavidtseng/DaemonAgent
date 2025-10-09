//----------------------------------------------------------------------------------------------------
// GameScriptInterface.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Framework/GameScriptInterface.hpp"
//----------------------------------------------------------------------------------------------------
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
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
        ScriptMethodInfo("appRequestQuit",
                         "Request quit to app",
                         {},
                         "void"),

        ScriptMethodInfo("executeCommand",
                         "執行 JavaScript 指令",
                         {"string"},
                         "string"),

        ScriptMethodInfo("executeFile",
                         "執行 JavaScript 檔案",
                         {"string"},
                         "string"),
    };
}

//----------------------------------------------------------------------------------------------------
std::vector<String> GameScriptInterface::GetAvailableProperties() const
{
    return {};
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::CallMethod(String const&     methodName,
                                                   ScriptArgs const& args)
{
    try
    {
        if (methodName == "appRequestQuit")
        {
            return ExecuteAppRequestQuit(args);
        }
        else if (methodName == "executeCommand")
        {
            return ExecuteJavaScriptCommand(args);
        }
        else if (methodName == "executeFile")
        {
            return ExecuteJavaScriptFile(args);
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

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::ExecuteJavaScriptCommand(const ScriptArgs& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "executeCommand");
    if (!result.success) return result;

    try
    {
        String command = ScriptTypeExtractor::ExtractString(args[0]);
        m_game->ExecuteJavaScriptCommand(command);
        return ScriptMethodResult::Success(String("指令執行: " + command));
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("執行 JavaScript 指令失敗: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::ExecuteJavaScriptFile(const ScriptArgs& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "executeFile");
    if (!result.success) return result;

    try
    {
        String filename = ScriptTypeExtractor::ExtractString(args[0]);
        m_game->ExecuteJavaScriptFile(filename);
        return ScriptMethodResult::Success(String("檔案執行: " + filename));
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("執行 JavaScript 檔案失敗: " + String(e.what()));
    }
}
