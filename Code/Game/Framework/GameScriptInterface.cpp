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
#include "Engine/Input/InputSystem.hpp"
//----------------------------------------------------------------------------------------------------
// Phase 6a: File I/O includes
#include <fstream>
#include <sstream>
#include <filesystem>
namespace fs = std::filesystem;

//----------------------------------------------------------------------------------------------------
// Helper function: Escape special characters in JSON strings
//----------------------------------------------------------------------------------------------------
static std::string EscapeJsonString(const std::string& input)
{
    std::string escaped;
    escaped.reserve(static_cast<size_t>(input.length() * 1.2)); // Reserve extra space for escape characters

    for (char c : input)
    {
        switch (c)
        {
            case '\\': escaped += "\\\\"; break; // Backslash
            case '\"': escaped += "\\\""; break; // Quote
            case '\n': escaped += "\\n"; break;  // Newline
            case '\r': escaped += "\\r"; break;  // Carriage return
            case '\t': escaped += "\\t"; break;  // Tab
            case '\b': escaped += "\\b"; break;  // Backspace
            case '\f': escaped += "\\f"; break;  // Form feed
            default: escaped += c; break;
        }
    }

    return escaped;
}

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

        // Phase 6a: KADI Development Tools - File Operations
        ScriptMethodInfo("createScriptFile",
                         "Create new JavaScript file in Scripts directory",
                         {"filePath:string", "content:string", "overwrite:boolean"},
                         "object"),

        ScriptMethodInfo("readScriptFile",
                         "Read existing JavaScript file from Scripts directory",
                         {"filePath:string"},
                         "object"),

        ScriptMethodInfo("deleteScriptFile",
                         "Delete JavaScript file from Scripts directory",
                         {"filePath:string"},
                         "object"),

        // Phase 6a: KADI Development Tools - Input Injection
        ScriptMethodInfo("injectKeyPress",
                         "Inject a key press with duration",
                         {"keyCode:number", "durationMs:number"},
                         "object"),

        ScriptMethodInfo("injectKeyHold",
                         "Inject a key hold with duration and optional repeat",
                         {"keyCode:number", "durationMs:number", "repeat:boolean"},
                         "object"),

        // Phase 6b: KADI Development Tools - FileWatcher Management
        ScriptMethodInfo("addWatchedFile",
                         "Add JavaScript file to hot-reload file watcher",
                         {"filePath:string"},
                         "object"),

        ScriptMethodInfo("removeWatchedFile",
                         "Remove JavaScript file from hot-reload file watcher",
                         {"filePath:string"},
                         "object"),

        ScriptMethodInfo("getWatchedFiles",
                         "Get list of all watched JavaScript files",
                         {},
                         "object"),
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
        // Phase 6a: KADI Development Tools - File Operations
        else if (methodName == "createScriptFile")
        {
            return ExecuteCreateScriptFile(args);
        }
        else if (methodName == "readScriptFile")
        {
            return ExecuteReadScriptFile(args);
        }
        else if (methodName == "deleteScriptFile")
        {
            return ExecuteDeleteScriptFile(args);
        }
        // Phase 6a: KADI Development Tools - Input Injection
        else if (methodName == "injectKeyPress")
        {
            return ExecuteInjectKeyPress(args);
        }
        else if (methodName == "injectKeyHold")
        {
            return ExecuteInjectKeyHold(args);
        }
        // Phase 6b: KADI Development Tools - FileWatcher Management
        else if (methodName == "addWatchedFile")
        {
            return ExecuteAddWatchedFile(args);
        }
        else if (methodName == "removeWatchedFile")
        {
            return ExecuteRemoveWatchedFile(args);
        }
        else if (methodName == "getWatchedFiles")
        {
            return ExecuteGetWatchedFiles(args);
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
//----------------------------------------------------------------------------------------------------
// Phase 6a: KADI Development Tools - File Operations Implementation
// Append these implementations to GameScriptInterface.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::ExecuteCreateScriptFile(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 3, "createScriptFile");
    if (!result.success) return result;

    try
    {
        // Extract parameters
        String filePath = ScriptTypeExtractor::ExtractString(args[0]);
        String content = ScriptTypeExtractor::ExtractString(args[1]);
        bool overwrite = ScriptTypeExtractor::ExtractBool(args[2]);

        // Validation: Empty file path
        if (filePath.empty())
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid file path: cannot be empty\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Security: Path validation (prevent directory traversal)
        if (filePath.find("..") != std::string::npos)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid file path: directory traversal not allowed\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Security: Extension validation (only .js files)
        if (filePath.find(".js") == std::string::npos)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid file extension: only .js files allowed\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Build full path (Run/Data/Scripts/)
        fs::path scriptsDir = fs::current_path() / "Data" / "Scripts";
        fs::path fullPath = scriptsDir / filePath;

        // Check if file exists
        if (fs::exists(fullPath) && !overwrite)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"File already exists and overwrite=false: " << EscapeJsonString(filePath) << "\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Create parent directories if needed
        fs::path parentDir = fullPath.parent_path();
        if (!fs::exists(parentDir))
        {
            fs::create_directories(parentDir);
        }

        // Write file
        std::ofstream outFile(fullPath, std::ios::out | std::ios::trunc);
        if (!outFile.is_open())
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Failed to open file for writing: " << EscapeJsonString(filePath) << "\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        outFile << content;
        outFile.close();

        size_t bytesWritten = content.length();

        // Return success with file info (escape path for JSON)
        std::ostringstream resultJson;
        resultJson << "{";
        resultJson << "\"success\":true,";
        resultJson << "\"filePath\":\"" << EscapeJsonString(fullPath.string()) << "\",";
        resultJson << "\"bytesWritten\":" << bytesWritten;
        resultJson << "}";

        return ScriptMethodResult::Success(resultJson.str());
    }
    catch (std::exception const& e)
    {
        std::ostringstream errorJson;
        errorJson << "{";
        errorJson << "\"success\":false,";
        errorJson << "\"error\":\"Create script file exception: " << EscapeJsonString(e.what()) << "\"";
        errorJson << "}";
        return ScriptMethodResult::Success(errorJson.str());
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::ExecuteReadScriptFile(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "readScriptFile");
    if (!result.success) return result;

    try
    {
        // Extract parameters
        String filePath = ScriptTypeExtractor::ExtractString(args[0]);

        // Security: Path validation (prevent directory traversal)
        if (filePath.find("..") != std::string::npos)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid file path: directory traversal not allowed\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Build full path (Run/Data/Scripts/)
        fs::path scriptsDir = fs::current_path() / "Data" / "Scripts";
        fs::path fullPath = scriptsDir / filePath;

        // Check if file exists
        if (!fs::exists(fullPath))
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"File not found: " << EscapeJsonString(filePath) << "\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Read file
        std::ifstream inFile(fullPath, std::ios::in);
        if (!inFile.is_open())
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Failed to open file for reading: " << EscapeJsonString(filePath) << "\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        std::stringstream buffer;
        buffer << inFile.rdbuf();
        inFile.close();

        std::string content = buffer.str();
        size_t lineCount = std::count(content.begin(), content.end(), '\n') + 1;
        size_t byteSize = content.length();

        // Escape content using helper function
        std::string escapedContent = EscapeJsonString(content);

        // Return success with file content (escape path for JSON)
        std::ostringstream resultJson;
        resultJson << "{";
        resultJson << "\"success\":true,";
        resultJson << "\"filePath\":\"" << EscapeJsonString(fullPath.string()) << "\",";
        resultJson << "\"content\":\"" << escapedContent << "\",";
        resultJson << "\"lineCount\":" << lineCount << ",";
        resultJson << "\"byteSize\":" << byteSize;
        resultJson << "}";

        return ScriptMethodResult::Success(resultJson.str());
    }
    catch (std::exception const& e)
    {
        std::ostringstream errorJson;
        errorJson << "{";
        errorJson << "\"success\":false,";
        errorJson << "\"error\":\"Read script file exception: " << EscapeJsonString(e.what()) << "\"";
        errorJson << "}";
        return ScriptMethodResult::Success(errorJson.str());
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::ExecuteDeleteScriptFile(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "deleteScriptFile");
    if (!result.success) return result;

    try
    {
        // Extract parameters
        String filePath = ScriptTypeExtractor::ExtractString(args[0]);

        // Security: Path validation (prevent directory traversal)
        if (filePath.find("..") != std::string::npos)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid file path: directory traversal not allowed\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Security: Protected files list (Phase 6 plan requirement)
        std::vector<std::string> protectedFiles = {
            "JSEngine.js",
            "JSGame.js",
            "InputSystem.js",
            "main.js",
            "kadi/KADIGameControl.js",
            "kadi/GameControlHandler.js",
            "kadi/GameControlTools.js",
            "kadi/DevelopmentToolHandler.js",
            "kadi/DevelopmentTools.js",
            "core/Subsystem.js",
            "components/RendererSystem.js",
            "components/Prop.js"
        };

        // Normalize path for comparison (replace backslashes with forward slashes)
        std::string normalizedPath = filePath;
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

        // Check if file is protected
        for (const auto& protectedFile : protectedFiles)
        {
            if (normalizedPath == protectedFile || normalizedPath.find(protectedFile) != std::string::npos)
            {
                std::ostringstream errorJson;
                errorJson << "{";
                errorJson << "\"success\":false,";
                errorJson << "\"error\":\"Cannot delete protected file: " << EscapeJsonString(filePath) << "\"";
                errorJson << "}";
                return ScriptMethodResult::Success(errorJson.str());
            }
        }

        // Build full path (Run/Data/Scripts/)
        fs::path scriptsDir = fs::current_path() / "Data" / "Scripts";
        fs::path fullPath = scriptsDir / filePath;

        // Check if file exists
        bool existed = fs::exists(fullPath);

        // Delete file if it exists
        if (existed)
        {
            fs::remove(fullPath);
        }

        // Return success with deletion info (escape path for JSON)
        std::ostringstream resultJson;
        resultJson << "{";
        resultJson << "\"success\":true,";
        resultJson << "\"filePath\":\"" << EscapeJsonString(fullPath.string()) << "\",";
        resultJson << "\"existed\":" << (existed ? "true" : "false");
        resultJson << "}";

        return ScriptMethodResult::Success(resultJson.str());
    }
    catch (std::exception const& e)
    {
        std::ostringstream errorJson;
        errorJson << "{";
        errorJson << "\"success\":false,";
        errorJson << "\"error\":\"Delete script file exception: " << EscapeJsonString(e.what()) << "\"";
        errorJson << "}";
        return ScriptMethodResult::Success(errorJson.str());
    }
}

//----------------------------------------------------------------------------------------------------
// Phase 6a: KADI Development Tools - Input Injection Implementation
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::ExecuteInjectKeyPress(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "injectKeyPress");
    if (!result.success) return result;

    try
    {
        // Extract parameters
        int keyCode = ScriptTypeExtractor::ExtractInt(args[0]);
        int durationMs = ScriptTypeExtractor::ExtractInt(args[1]);

        // Validate parameters
        if (keyCode < 0 || keyCode > 255)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid keyCode: must be 0-255\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        if (durationMs < 0)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid durationMs: must be >= 0\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Call InputSystem to inject key press
        if (!g_input)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"InputSystem not available\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        g_input->InjectKeyPress(static_cast<unsigned char>(keyCode), durationMs);

        // Return success with injection info
        std::ostringstream resultJson;
        resultJson << "{";
        resultJson << "\"success\":true,";
        resultJson << "\"keyCode\":" << keyCode << ",";
        resultJson << "\"durationMs\":" << durationMs;
        resultJson << "}";

        return ScriptMethodResult::Success(resultJson.str());
    }
    catch (std::exception const& e)
    {
        std::ostringstream errorJson;
        errorJson << "{";
        errorJson << "\"success\":false,";
        errorJson << "\"error\":\"Inject key press exception: " << EscapeJsonString(e.what()) << "\"";
        errorJson << "}";
        return ScriptMethodResult::Success(errorJson.str());
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::ExecuteInjectKeyHold(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 3, "injectKeyHold");
    if (!result.success) return result;

    try
    {
        // Extract parameters
        int keyCode = ScriptTypeExtractor::ExtractInt(args[0]);
        int durationMs = ScriptTypeExtractor::ExtractInt(args[1]);
        bool repeat = ScriptTypeExtractor::ExtractBool(args[2]);

        // Validate parameters
        if (keyCode < 0 || keyCode > 255)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid keyCode: must be 0-255\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        if (durationMs < 0)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid durationMs: must be >= 0\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Call InputSystem to inject key hold
        if (!g_input)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"InputSystem not available\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        g_input->InjectKeyHold(static_cast<unsigned char>(keyCode), durationMs, repeat);

        // Return success with injection info
        std::ostringstream resultJson;
        resultJson << "{";
        resultJson << "\"success\":true,";
        resultJson << "\"keyCode\":" << keyCode << ",";
        resultJson << "\"durationMs\":" << durationMs << ",";
        resultJson << "\"repeat\":" << (repeat ? "true" : "false");
        resultJson << "}";

        return ScriptMethodResult::Success(resultJson.str());
    }
    catch (std::exception const& e)
    {
        std::ostringstream errorJson;
        errorJson << "{";
        errorJson << "\"success\":false,";
        errorJson << "\"error\":\"Inject key hold exception: " << EscapeJsonString(e.what()) << "\"";
        errorJson << "}";
        return ScriptMethodResult::Success(errorJson.str());
    }
}

//----------------------------------------------------------------------------------------------------
// Phase 6b: KADI Development Tools - FileWatcher Management Implementation
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::ExecuteAddWatchedFile(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "addWatchedFile");
    if (!result.success) return result;

    try
    {
        // Extract parameter
        String filePath = ScriptTypeExtractor::ExtractString(args[0]);

        // Validation: Empty file path
        if (filePath.empty())
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid file path: cannot be empty\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Security: Path validation (prevent directory traversal)
        if (filePath.find("..") != std::string::npos)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid file path: directory traversal not allowed\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Security: Extension validation (only .js files)
        if (filePath.find(".js") == std::string::npos)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid file extension: only .js files allowed\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Access ScriptSubsystem through global pointer
        if (!g_scriptSubsystem)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"ScriptSubsystem not available\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Build relative path (Data/Scripts/ prefix)
        std::string relativePath = "Data/Scripts/" + filePath;

        // Add to FileWatcher
        g_scriptSubsystem->AddWatchedFile(relativePath);

        // Return success with file info
        std::ostringstream resultJson;
        resultJson << "{";
        resultJson << "\"success\":true,";
        resultJson << "\"filePath\":\"" << EscapeJsonString(filePath) << "\",";
        resultJson << "\"relativePath\":\"" << EscapeJsonString(relativePath) << "\"";
        resultJson << "}";

        return ScriptMethodResult::Success(resultJson.str());
    }
    catch (std::exception const& e)
    {
        std::ostringstream errorJson;
        errorJson << "{";
        errorJson << "\"success\":false,";
        errorJson << "\"error\":\"Add watched file exception: " << EscapeJsonString(e.what()) << "\"";
        errorJson << "}";
        return ScriptMethodResult::Success(errorJson.str());
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::ExecuteRemoveWatchedFile(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "removeWatchedFile");
    if (!result.success) return result;

    try
    {
        // Extract parameter
        String filePath = ScriptTypeExtractor::ExtractString(args[0]);

        // Validation: Empty file path
        if (filePath.empty())
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid file path: cannot be empty\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Security: Path validation (prevent directory traversal)
        if (filePath.find("..") != std::string::npos)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"Invalid file path: directory traversal not allowed\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Access ScriptSubsystem through global pointer
        if (!g_scriptSubsystem)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"ScriptSubsystem not available\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Build relative path (Data/Scripts/ prefix)
        std::string relativePath = "Data/Scripts/" + filePath;

        // Remove from FileWatcher
        g_scriptSubsystem->RemoveWatchedFile(relativePath);

        // Return success with file info
        std::ostringstream resultJson;
        resultJson << "{";
        resultJson << "\"success\":true,";
        resultJson << "\"filePath\":\"" << EscapeJsonString(filePath) << "\",";
        resultJson << "\"relativePath\":\"" << EscapeJsonString(relativePath) << "\"";
        resultJson << "}";

        return ScriptMethodResult::Success(resultJson.str());
    }
    catch (std::exception const& e)
    {
        std::ostringstream errorJson;
        errorJson << "{";
        errorJson << "\"success\":false,";
        errorJson << "\"error\":\"Remove watched file exception: " << EscapeJsonString(e.what()) << "\"";
        errorJson << "}";
        return ScriptMethodResult::Success(errorJson.str());
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult GameScriptInterface::ExecuteGetWatchedFiles(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "getWatchedFiles");
    if (!result.success) return result;

    try
    {
        // Access ScriptSubsystem through global pointer
        if (!g_scriptSubsystem)
        {
            std::ostringstream errorJson;
            errorJson << "{";
            errorJson << "\"success\":false,";
            errorJson << "\"error\":\"ScriptSubsystem not available\"";
            errorJson << "}";
            return ScriptMethodResult::Success(errorJson.str());
        }

        // Get watched files list
        std::vector<std::string> watchedFiles = g_scriptSubsystem->GetWatchedFiles();

        // Build JSON array
        std::ostringstream resultJson;
        resultJson << "{";
        resultJson << "\"success\":true,";
        resultJson << "\"count\":" << watchedFiles.size() << ",";
        resultJson << "\"files\":[";

        for (size_t i = 0; i < watchedFiles.size(); ++i)
        {
            resultJson << "\"" << EscapeJsonString(watchedFiles[i]) << "\"";
            if (i < watchedFiles.size() - 1)
            {
                resultJson << ",";
            }
        }

        resultJson << "]";
        resultJson << "}";

        return ScriptMethodResult::Success(resultJson.str());
    }
    catch (std::exception const& e)
    {
        std::ostringstream errorJson;
        errorJson << "{";
        errorJson << "\"success\":false,";
        errorJson << "\"error\":\"Get watched files exception: " << EscapeJsonString(e.what()) << "\"";
        errorJson << "}";
        return ScriptMethodResult::Success(errorJson.str());
    }
}
