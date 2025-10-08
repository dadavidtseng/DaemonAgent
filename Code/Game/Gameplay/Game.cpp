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
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Player.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Input/InputSystem.hpp"
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

    m_gameClock = new Clock(Clock::GetSystemClock());

    DebugAddWorldBasis(Mat44(), -1.f);

    Mat44 transform;

    transform.SetIJKT3D(-Vec3::Y_BASIS, Vec3::X_BASIS, Vec3::Z_BASIS, Vec3(0.25f, 0.f, 0.25f));
    DebugAddWorldText("X-Forward", transform, 0.25f, Vec2::ONE, -1.f, Rgba8::RED);

    transform.SetIJKT3D(-Vec3::X_BASIS, -Vec3::Y_BASIS, Vec3::Z_BASIS, Vec3(0.f, 0.25f, 0.5f));
    DebugAddWorldText("Y-Left", transform, 0.25f, Vec2::ZERO, -1.f, Rgba8::GREEN);

    transform.SetIJKT3D(-Vec3::X_BASIS, Vec3::Z_BASIS, Vec3::Y_BASIS, Vec3(0.f, -0.25f, 0.25f));
    DebugAddWorldText("Z-Up", transform, 0.25f, Vec2(1.f, 0.f), -1.f, Rgba8::BLUE);

    DAEMON_LOG(LogGame, eLogVerbosity::Log, "(Game::Game)(end)");
}

//----------------------------------------------------------------------------------------------------
Game::~Game()
{
    DAEMON_LOG(LogGame, eLogVerbosity::Log, "(Game::~Game)(start)");

    GAME_SAFE_RELEASE(m_gameClock);

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
        float const gameDeltaSeconds   = static_cast<float>(m_gameClock->GetDeltaSeconds());
        float const systemDeltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());
        ExecuteJavaScriptCommand(StringFormat("globalThis.JSEngine.update({}, {});", std::to_string(gameDeltaSeconds), std::to_string(systemDeltaSeconds)));
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
Clock* Game::GetClock() const
{
    if (m_gameClock == nullptr) return nullptr;

    return m_gameClock;
}

void Game::Update(float const gameDeltaSeconds,
                  float const systemDeltaSeconds)
{
    UpdateFromKeyBoard();

    // Note: HandleJavaScriptCommands is now called from the main Update() method
    HandleConsoleCommands();
}

void Game::Render()
{
    RenderAttractMode();
}

//----------------------------------------------------------------------------------------------------
void Game::HandleConsoleCommands()
{
    // 處理開發者控制台的 JavaScript 指令
    // 這需要與 DevConsole 整合

    if (g_devConsole && g_devConsole->IsOpen())
    {
        // 檢查控制台輸入是否為 JavaScript 指令
        // 這裡需要實作具體的控制台輸入檢查邏輯

        // 範例實作（需要 DevConsole 支援）:
        /*
        std::string input = g_theConsole->GetLastInput();
        if (input.substr(0, 3) == "js:")
        {
            std::string jsCommand = input.substr(3);
            ExecuteJavaScriptCommand(jsCommand);
        }
        else if (input.substr(0, 7) == "jsfile:")
        {
            std::string filename = input.substr(7);
            ExecuteJavaScriptFile(filename);
        }
        */
    }
}

//----------------------------------------------------------------------------------------------------
void Game::UpdateFromKeyBoard()
{
    if (g_input->WasKeyJustPressed(KEYCODE_ESC))
    {
        App::RequestQuit();
    }

    if (g_input->WasKeyJustPressed(KEYCODE_O))
    {
        m_gameClock->StepSingleFrame();
    }

    if (g_input->IsKeyDown(KEYCODE_T))
    {
        m_gameClock->SetTimeScale(0.1f);
    }

    if (g_input->WasKeyJustReleased(KEYCODE_T))
    {
        m_gameClock->SetTimeScale(1.f);
    }
}

//----------------------------------------------------------------------------------------------------
void Game::UpdateFromController()
{
#pragma region XboxController
    // XboxController const& controller = g_input->GetController(0);
    //
    // if (m_gameState == eGameState::ATTRACT)
    // {
    //     if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
    //     {
    //         App::RequestQuit();
    //     }
    //
    //     if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
    //     {
    //         m_gameState = eGameState::GAME;
    //     }
    // }
    //
    // if (m_gameState == eGameState::GAME)
    // {
    //     if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
    //     {
    //         m_gameState = eGameState::ATTRACT;
    //     }
    //
    //     if (controller.WasButtonJustPressed(XBOX_BUTTON_B))
    //     {
    //         m_gameClock->TogglePause();
    //     }
    //
    //     if (controller.WasButtonJustPressed(XBOX_BUTTON_Y))
    //     {
    //         m_gameClock->StepSingleFrame();
    //     }
    //
    //     if (controller.WasButtonJustPressed(XBOX_BUTTON_X))
    //     {
    //         m_gameClock->SetTimeScale(0.1f);
    //     }
    //
    //     if (controller.WasButtonJustReleased(XBOX_BUTTON_X))
    //     {
    //         m_gameClock->SetTimeScale(1.f);
    //     }
    // }
#pragma endregion
}

//----------------------------------------------------------------------------------------------------
void Game::RenderAttractMode() const
{
    Vec2 clientDimensions = Window::s_mainWindow->GetClientDimensions();

    VertexList_PCU verts;
    AddVertsForDisc2D(verts, Vec2(clientDimensions.x * 0.5f, clientDimensions.y * 0.5f), 300.f, 10.f, Rgba8::YELLOW);
    g_renderer->SetModelConstants();
    g_renderer->SetBlendMode(eBlendMode::OPAQUE);
    g_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
    g_renderer->SetSamplerMode(eSamplerMode::BILINEAR_CLAMP);
    g_renderer->SetDepthMode(eDepthMode::DISABLED);
    g_renderer->BindTexture(nullptr);
    g_renderer->BindShader(g_renderer->CreateOrGetShaderFromFile("Data/Shaders/Default"));
    g_renderer->DrawVertexArray(verts);
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
