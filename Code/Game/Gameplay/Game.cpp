//----------------------------------------------------------------------------------------------------
// Game.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// Prevent Windows.h min/max macros from conflicting with V8 and standard library
#ifndef NOMINMAX
#define NOMINMAX
#endif

//----------------------------------------------------------------------------------------------------
#include "Game/Gameplay/Game.hpp"
//----------------------------------------------------------------------------------------------------
#include "Game/Framework/App.hpp"
//----------------------------------------------------------------------------------------------------
#include <any>
#include <typeinfo>
//----------------------------------------------------------------------------------------------------
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Engine/Script/ModuleLoader.hpp"
#include "Engine/Script/ScriptSubsystem.hpp"

//----------------------------------------------------------------------------------------------------
Game::Game()
{
    DAEMON_LOG(LogGame, eLogVerbosity::Log, StringFormat("(Game::Game)(start)"));

    // DebugAddWorldBasis(Mat44(), -1.f);
    //
    // Mat44 transform;
    //
    // transform.SetIJKT3D(-Vec3::Y_BASIS, Vec3::X_BASIS, Vec3::Z_BASIS, Vec3(0.25f, 0.f, 0.25f));
    // DebugAddWorldText("X-Forward", transform, 0.25f, Vec2::ONE, -1.f, Rgba8::RED);
    //
    // transform.SetIJKT3D(-Vec3::X_BASIS, -Vec3::Y_BASIS, Vec3::Z_BASIS, Vec3(0.f, 0.25f, 0.5f));
    // DebugAddWorldText("Y-Left", transform, 0.25f, Vec2::ZERO, -1.f, Rgba8::GREEN);
    //
    // transform.SetIJKT3D(-Vec3::X_BASIS, Vec3::Z_BASIS, Vec3::Y_BASIS, Vec3(0.f, -0.25f, 0.25f));
    // DebugAddWorldText("Z-Up", transform, 0.25f, Vec2(1.f, 0.f), -1.f, Rgba8::BLUE);
    //
    // DebugAddScreenText("TEST", Vec2(0, 100), 20.f, Vec2::ZERO, 10.f);

    DAEMON_LOG(LogGame, eLogVerbosity::Log, "(Game::Game)(end)");
}

//----------------------------------------------------------------------------------------------------
Game::~Game()
{
    DAEMON_LOG(LogGame, eLogVerbosity::Log, "(Game::~Game)(start)");
    DAEMON_LOG(LogGame, eLogVerbosity::Display, "(Game::~Game)(end)");
}

//----------------------------------------------------------------------------------------------------
void Game::PostInit()
{
    InitializeJavaScriptFramework();
}

//----------------------------------------------------------------------------------------------------
void Game::UpdateJS()
{
    if (g_scriptSubsystem && g_scriptSubsystem->IsInitialized())
    {
        float const systemDeltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());
        ExecuteJavaScriptCommand(StringFormat("globalThis.JSEngine.update({});", std::to_string(systemDeltaSeconds)));
    }
}

//----------------------------------------------------------------------------------------------------
void Game::RenderJS()
{
    if (g_scriptSubsystem && g_scriptSubsystem->IsInitialized())
    {
        ExecuteJavaScriptCommand(StringFormat("globalThis.JSEngine.render();"));
    }
}

//----------------------------------------------------------------------------------------------------
bool Game::IsAttractMode() const
{
    if (g_scriptSubsystem && g_scriptSubsystem->IsInitialized())
    {
        try
        {
            // Query JavaScript game state
            // globalThis.jsGameInstance.gameState returns 'ATTRACT' or 'GAME'
            std::any result = g_scriptSubsystem->ExecuteScriptWithResult("globalThis.jsGameInstance ? globalThis.jsGameInstance.gameState : 'GAME'");

            // Try to cast result to string
            if (result.type() == typeid(std::string))
            {
                std::string gameState = std::any_cast<std::string>(result);
                return gameState == "ATTRACT";
            }
        }
        catch (...)
        {
            // If there's any error, default to non-attract mode
        }
    }
    return false;  // Default to not attract mode if JavaScript not initialized
}

//----------------------------------------------------------------------------------------------------
void Game::ExecuteJavaScriptCommand(String const& command)
{
    // DAEMON_LOG(LogGame, eLogVerbosity::Log, Stringf("Game::ExecuteJavaScriptCommand() start | %s", command.c_str()));

    if (g_scriptSubsystem == nullptr)
    {
        DAEMON_LOG(LogGame, eLogVerbosity::Error, Stringf("(Game::ExecuteJavaScriptCommand)(failed)(g_scriptSubsystem is nullptr!)"));
        return;
    }

    if (!g_scriptSubsystem->IsInitialized())
    {
        DAEMON_LOG(LogGame, eLogVerbosity::Error, Stringf("(Game::ExecuteJavaScriptCommand) failed| %s | ScriptSubsystem is not initialized", command.c_str()));
        return;
    }

    bool const success = g_scriptSubsystem->ExecuteScript(command);

    if (success)
    {
        String const result = g_scriptSubsystem->GetLastResult();

        if (!result.empty())
        {
            DAEMON_LOG(LogGame, eLogVerbosity::Log, Stringf("Game::ExecuteJavaScriptCommand() result | %s", result.c_str()));
        }
    }
    else
    {
        DAEMON_LOG(LogGame, eLogVerbosity::Error, Stringf("Game::ExecuteJavaScriptCommand() failed"));

        if (g_scriptSubsystem->HasError())
        {
            DAEMON_LOG(LogGame, eLogVerbosity::Error, Stringf("Game::ExecuteJavaScriptCommand() error | %s", g_scriptSubsystem->GetLastError().c_str()));
        }
    }

    // DAEMON_LOG(LogGame, eLogVerbosity::Log, Stringf("Game::ExecuteJavaScriptCommand() end | %s", command.c_str()));
}

//----------------------------------------------------------------------------------------------------
void Game::ExecuteJavaScriptFile(String const& filename)
{
    if (g_scriptSubsystem == nullptr)ERROR_AND_DIE(StringFormat("(Game::ExecuteJavaScriptFile)(g_scriptSubsystem is nullptr!)"))
    if (!g_scriptSubsystem->IsInitialized())ERROR_AND_DIE(StringFormat("(Game::ExecuteJavaScriptFile)(g_scriptSubsystem is not initialized!)"))

    DAEMON_LOG(LogGame, eLogVerbosity::Log, StringFormat("(Game::ExecuteJavaScriptFile)(start)({})", filename));

    bool const success = g_scriptSubsystem->ExecuteScriptFile(filename);

    if (!success)
    {
        DAEMON_LOG(LogGame, eLogVerbosity::Error, StringFormat("(Game::ExecuteJavaScriptFile)(fail)({})", filename));

        if (g_scriptSubsystem->HasError())
        {
            DAEMON_LOG(LogGame, eLogVerbosity::Error, StringFormat("(Game::ExecuteJavaScriptFile)(fail)(error: {})", g_scriptSubsystem->GetLastError()));
        }

        return;
    }

    DAEMON_LOG(LogGame, eLogVerbosity::Log, StringFormat("(Game::ExecuteJavaScriptFile)(end)({})", filename.c_str()));
}

//----------------------------------------------------------------------------------------------------
void Game::ExecuteModuleFile(String const& modulePath)
{
    if (g_scriptSubsystem == nullptr)ERROR_AND_DIE(StringFormat("(Game::ExecuteModuleFile)(g_scriptSubsystem is nullptr!)"))
    if (!g_scriptSubsystem->IsInitialized())ERROR_AND_DIE(StringFormat("(Game::ExecuteModuleFile)(g_scriptSubsystem is not initialized!)"))

    DAEMON_LOG(LogGame, eLogVerbosity::Log, StringFormat("(Game::ExecuteModuleFile)(start)({})", modulePath));

    bool const success = g_scriptSubsystem->ExecuteModule(modulePath);

    if (!success)
    {
        DAEMON_LOG(LogGame, eLogVerbosity::Error, StringFormat("(Game::ExecuteModuleFile)(fail)({})", modulePath));

        if (g_scriptSubsystem->HasError())
        {
            DAEMON_LOG(LogGame, eLogVerbosity::Error, StringFormat("(Game::ExecuteModuleFile)(fail)(error: {})", g_scriptSubsystem->GetLastError()));
        }

        return;
    }

    DAEMON_LOG(LogGame, eLogVerbosity::Log, StringFormat("(Game::ExecuteModuleFile)(end)({})", modulePath.c_str()));
}

//----------------------------------------------------------------------------------------------------
void Game::InitializeJavaScriptFramework()
{
    DAEMON_LOG(LogGame, eLogVerbosity::Display, "Game::InitializeJavaScriptFramework() start");

    if (!g_scriptSubsystem || !g_scriptSubsystem->IsInitialized())
    {
        DAEMON_LOG(LogGame, eLogVerbosity::Error, "Game::InitializeJavaScriptFramework() failed - ScriptSubsystem not available");
        return;
    }

    try
    {
        // Load ES6 module entry point (imports all other modules via import statements)
        DAEMON_LOG(LogGame, eLogVerbosity::Display, "Loading main.js (ES6 module entry point)...");
        ExecuteModuleFile("Data/Scripts/main.js");

        DAEMON_LOG(LogGame, eLogVerbosity::Display, "Game::InitializeJavaScriptFramework() complete - Pure ES6 Module architecture initialized");
    }
    catch (...)
    {
        DAEMON_LOG(LogGame, eLogVerbosity::Error, "Game::InitializeJavaScriptFramework() exception occurred");
    }
}

//----------------------------------------------------------------------------------------------------
// IJSGameLogicContext Interface Implementation (Worker Thread)
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// UpdateJSWorkerThread (Worker Thread)
//
// Execute JavaScript update logic on worker thread.
// Called by JSGameLogicJob from worker thread context.
//
// Thread Safety:
//   - Called from worker thread
//   - Must acquire v8::Locker before V8 API calls
//   - entityBuffer: Write to back buffer only (safe)
//   - commandQueue: Lock-free SPSC queue (safe)
//----------------------------------------------------------------------------------------------------
void Game::UpdateJSWorkerThread(float deltaTime, EntityStateBuffer* entityBuffer, RenderCommandQueue* commandQueue)
{
    // TODO Phase 2: Implement JavaScript update logic execution
    // This will call into JSEngine.update() through ScriptSubsystem
    // and update entity state buffer based on JavaScript logic

    UNUSED(deltaTime);
    UNUSED(entityBuffer);
    UNUSED(commandQueue);

    // Phase 2 implementation will:
    // 1. Acquire v8::Locker
    // 2. Call ScriptSubsystem::ExecuteUpdateSystemJS()
    // 3. JavaScript updates entities via EntityScriptInterface
    // 4. EntityScriptInterface writes to entityBuffer back buffer
    // 5. RenderCommands submitted to commandQueue
}

//----------------------------------------------------------------------------------------------------
// RenderJSWorkerThread (Worker Thread)
//
// Execute JavaScript render logic on worker thread.
// Called by JSGameLogicJob from worker thread context.
//
// Thread Safety:
//   - Called from worker thread
//   - Must acquire v8::Locker before V8 API calls
//   - cameraBuffer: Write to back buffer only (safe)
//   - commandQueue: Lock-free SPSC queue (safe)
//----------------------------------------------------------------------------------------------------
void Game::RenderJSWorkerThread(float deltaTime, CameraStateBuffer* cameraBuffer, RenderCommandQueue* commandQueue)
{
    // TODO Phase 2: Implement JavaScript render logic execution
    // This will call into JSEngine.render() through ScriptSubsystem
    // and update camera state buffer based on JavaScript logic

    UNUSED(deltaTime);
    UNUSED(cameraBuffer);
    UNUSED(commandQueue);

    // Phase 2 implementation will:
    // 1. Acquire v8::Locker
    // 2. Call ScriptSubsystem::ExecuteRenderSystemJS()
    // 3. JavaScript updates cameras via CameraScriptInterface (M5)
    // 4. CameraScriptInterface writes to cameraBuffer back buffer
    // 5. RenderCommands submitted to commandQueue
}

//----------------------------------------------------------------------------------------------------
// HandleJSException (Worker Thread)
//
// Handle JavaScript exception from worker thread.
// Called by JSGameLogicJob when JavaScript errors occur.
//
// Thread Safety:
//   - Called from worker thread
//   - Should log error, signal recovery
//   - Should NOT crash worker thread
//----------------------------------------------------------------------------------------------------
void Game::HandleJSException(char const* errorMessage, char const* stackTrace)
{
    // TODO Phase 3: Implement JavaScript error recovery
    // This will log the error and attempt hot-reload recovery

    DAEMON_LOG(LogGame, eLogVerbosity::Error,
               StringFormat("JavaScript Exception (Worker Thread):\n{}\n{}",
                            errorMessage, stackTrace));

    // Phase 3 implementation will:
    // 1. Log error to console with stack trace
    // 2. Attempt hot-reload recovery (reload last known good script)
    // 3. Signal main thread of error state (visual indicator)
    // 4. Continue worker thread execution with last known good state
}
