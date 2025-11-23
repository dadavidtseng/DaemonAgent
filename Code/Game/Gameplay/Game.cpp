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
#include "ThirdParty/imgui/imgui.h"
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

    // Phase 2.3: ImGui CANNOT be called from worker thread!
    // ImGui requires main thread context (g.WithinFrameScope assertion)
    // Restored ImGui rendering to main thread UpdateJS() method

    // ImGui Debug Window (Phase 0, Task 0.1: IMGUI Integration Test) - Direct Integration
    ImGui::Begin("SimpleMiner Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("IMGUI Integration Successful!");
    ImGui::Separator();

    ImGui::Text("Game Stats:");

    ImGui::Separator();
    ImGui::Text("Phase 0: Prerequisites");
    ImGui::BulletText("Task 0.1: IMGUI Integration - COMPLETE");
    ImGui::BulletText("Task 0.2: Curve Editor - Pending");
    ImGui::BulletText("Task 0.3: Chunk Regen Controls - Pending");
    ImGui::BulletText("Task 0.4: Noise Visualization - Pending");

    // Demo Window Button
    ImGui::Separator();
    ImGui::Text("ImGui Windows:");

    if (ImGui::Button(m_showDemoWindow ? "Hide Demo Window" : "Show Demo Window"))
    {
        m_showDemoWindow = !m_showDemoWindow;
    }

    // Show windows based on toggles
    if (m_showDemoWindow)
    {
        ShowSimpleDemoWindow();
    }

    ImGui::End();
}

//----------------------------------------------------------------------------------------------------
void Game::RenderJS()
{
    if (g_scriptSubsystem && g_scriptSubsystem->IsInitialized())
    {
        ExecuteJavaScriptCommand(StringFormat("globalThis.JSEngine.render();"));
    }
}

void Game::ShowSimpleDemoWindow()
{
    ImGui::Begin("ImGui Demo Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::CollapsingHeader("Basic Widgets"))
    {
        // Buttons
        if (ImGui::Button("Button"))
        {
            // Handle button click
        }
        ImGui::SameLine();
        if (ImGui::Button("Another Button"))
        {
            // Handle button click
        }

        // Checkbox
        static bool checkbox_val = false;
        ImGui::Checkbox("Enable Feature", &checkbox_val);

        // Radio buttons
        static int radio_option = 0;
        ImGui::RadioButton("Option A", &radio_option, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Option B", &radio_option, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Option C", &radio_option, 2);
    }

    if (ImGui::CollapsingHeader("Sliders"))
    {
        // Float slider
        static float float_val = 0.0f;
        ImGui::SliderFloat("Float Slider", &float_val, 0.0f, 1.0f);

        // Int slider
        static int int_val = 0;
        ImGui::SliderInt("Int Slider", &int_val, 0, 100);

        // Range slider
        static float range_val = 0.0f;
        ImGui::SliderFloat("Range Slider", &range_val, -10.0f, 10.0f);
    }

    if (ImGui::CollapsingHeader("Color Controls"))
    {
        // Color picker
        static float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        ImGui::ColorEdit3("Color", color);

        // Color button
        static ImVec4 color_button = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        if (ImGui::ColorButton("Color Button", color_button))
        {
            // Handle color selection
        }
    }

    if (ImGui::CollapsingHeader("Text Input"))
    {
        // Text input
        static char text_buf[256] = "Hello, ImGui!";
        ImGui::InputText("Text Input", text_buf, sizeof(text_buf));

        // Multiline text
        static char text_multiline[1024] = "This is a\nmultiline\n text area.";
        ImGui::InputTextMultiline("Multiline", text_multiline, sizeof(text_multiline));
    }

    if (ImGui::CollapsingHeader("Progress Bars"))
    {
        // Progress bar
        static float progress = 0.0f;
        ImGui::ProgressBar(progress, ImVec2(200, 0));
        ImGui::SameLine();
        ImGui::Text("Progress: %.0f%%", progress * 100);

        if (ImGui::Button("Add 25%"))
        {
            progress = (std::min)(progress + 0.25f, 1.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            progress = 0.0f;
        }
    }

    if (ImGui::CollapsingHeader("Simple Tree"))
    {
        if (ImGui::TreeNode("Root Node"))
        {
            if (ImGui::TreeNode("Child 1"))
            {
                ImGui::Text("Leaf content 1");
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Child 2"))
            {
                ImGui::Text("Leaf content 2");
                if (ImGui::TreeNode("Sub-child"))
                {
                    ImGui::Text("Sub-leaf content");
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }
    }

    // Advanced Input Widgets
    if (ImGui::CollapsingHeader("Advanced Input"))
    {
        static float drag_float = 0.0f;
        ImGui::DragFloat("Drag Float", &drag_float, 0.01f, 0.0f, 100.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Drag to adjust value");

        static float input_float = 0.0f;
        ImGui::InputFloat("Input Float", &input_float);

        static int input_int = 0;
        ImGui::InputInt("Input Int", &input_int);

        static float angle = 0.0f;
        ImGui::SliderAngle("Rotation", &angle);

        static float vec3[3] = { 0.0f, 0.0f, 0.0f };
        ImGui::DragFloat3("Position", vec3, 0.1f);
    }

    // Tables & Data Display
    if (ImGui::CollapsingHeader("Tables"))
    {
        if (ImGui::BeginTable("DemoTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Position");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Vec3");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("(10.0, 5.0, 2.0)");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Health");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Int");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("100");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Speed");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Float");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("5.5");

            ImGui::EndTable();
        }
    }

    // Tabs
    if (ImGui::CollapsingHeader("Tabs"))
    {
        if (ImGui::BeginTabBar("DemoTabs"))
        {
            if (ImGui::BeginTabItem("Tab 1"))
            {
                ImGui::Text("This is Tab 1 content");
                ImGui::BulletText("Feature A");
                ImGui::BulletText("Feature B");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Tab 2"))
            {
                ImGui::Text("This is Tab 2 content");
                ImGui::BulletText("Setting X");
                ImGui::BulletText("Setting Y");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Tab 3"))
            {
                ImGui::Text("This is Tab 3 content");
                static bool option_enabled = false;
                ImGui::Checkbox("Enable Option", &option_enabled);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    // Child Windows & Scrolling
    if (ImGui::CollapsingHeader("Child Windows"))
    {
        ImGui::Text("Scrollable child region:");
        ImGui::BeginChild("ChildRegion", ImVec2(0, 100), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (int i = 0; i < 20; i++)
        {
            ImGui::Text("Line %d - This is a scrollable content area", i);
        }
        ImGui::EndChild();
    }

    // Popups & Modals
    if (ImGui::CollapsingHeader("Popups & Modals"))
    {
        if (ImGui::Button("Open Modal"))
        {
            ImGui::OpenPopup("DemoModal");
        }

        if (ImGui::BeginPopupModal("DemoModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("This is a modal dialog");
            ImGui::Separator();
            ImGui::Text("Click OK to close");

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Right-Click Menu"))
        {
            ImGui::OpenPopup("ContextMenu");
        }

        if (ImGui::BeginPopup("ContextMenu"))
        {
            if (ImGui::Selectable("Option 1")) { /* Handle option 1 */ }
            if (ImGui::Selectable("Option 2")) { /* Handle option 2 */ }
            if (ImGui::Selectable("Option 3")) { /* Handle option 3 */ }
            ImGui::EndPopup();
        }
    }

    // Plotting
    if (ImGui::CollapsingHeader("Plotting"))
    {
        static float values[90] = {};
        static int values_offset = 0;
        static float refresh_time = 0.0f;

        // Generate sine wave data
        if (refresh_time == 0.0f)
        {
            for (int i = 0; i < 90; i++)
            {
                values[i] = sinf(i * 0.1f);
            }
        }
        refresh_time += 0.016f;

        ImGui::PlotLines("Sine Wave", values, 90, values_offset, nullptr, -1.0f, 1.0f, ImVec2(0, 80));

        static float histogram[10] = { 0.1f, 0.3f, 0.5f, 0.7f, 0.9f, 0.7f, 0.5f, 0.3f, 0.2f, 0.1f };
        ImGui::PlotHistogram("Histogram", histogram, 10, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 80));
    }

    ImGui::End();
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

        // CRITICAL: Verify that main.js successfully initialized globalThis.JSEngine
        // This diagnostic check helps identify if module loading failed silently
        String checkCode = "typeof globalThis.JSEngine !== 'undefined' && typeof globalThis.JSEngine.update === 'function'";
        bool jsEngineCreated = false;

        if (g_scriptSubsystem->ExecuteScript(checkCode))
        {
            String result = g_scriptSubsystem->GetLastResult();
            jsEngineCreated = (result == "true");
        }

        if (jsEngineCreated)
        {
            DAEMON_LOG(LogGame, eLogVerbosity::Display,
                       "Game::InitializeJavaScriptFramework() SUCCESS - globalThis.JSEngine verified");
        }
        else
        {
            DAEMON_LOG(LogGame, eLogVerbosity::Error,
                       "Game::InitializeJavaScriptFramework() FAILED - globalThis.JSEngine not found after loading main.js");
            DAEMON_LOG(LogGame, eLogVerbosity::Error,
                       "  This means main.js either failed to load or failed to create JSEngine instance");
            DAEMON_LOG(LogGame, eLogVerbosity::Error,
                       "  Check for module loading errors in the log above");
        }

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
    // Phase 2.3: Execute JavaScript update logic on worker thread
    // This is called from JSGameLogicJob worker thread with v8::Locker already acquired
    if (g_scriptSubsystem && g_scriptSubsystem->IsInitialized())
    {
        // Phase 2.3 FIX: Check if JSEngine is initialized before calling it
        // First verify globalThis.JSEngine exists (defensive programming)
        String checkCode = "typeof globalThis.JSEngine !== 'undefined' && typeof globalThis.JSEngine.update === 'function'";
        bool jsEngineReady = false;

        if (g_scriptSubsystem->ExecuteScript(checkCode))
        {
            String result = g_scriptSubsystem->GetLastResult();
            jsEngineReady = (result == "true");
        }

        if (jsEngineReady)
        {
            // JSEngine is ready - execute update
            ExecuteJavaScriptCommand(StringFormat("globalThis.JSEngine.update({});", std::to_string(deltaTime)));
        }
        else
        {
            // JSEngine not ready - log warning (only once per second to avoid spam)
            static float lastWarningTime = 0.0f;
            float currentTime = static_cast<float>(Clock::GetSystemClock().GetTotalSeconds());

            if (currentTime - lastWarningTime >= 1.0f)
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                           "UpdateJSWorkerThread: globalThis.JSEngine not initialized - skipping JavaScript update");
                lastWarningTime = currentTime;
            }
        }
    }

    // Phase 2.3: ImGui CANNOT be called from worker thread!
    // ImGui requires main thread context (g.WithinFrameScope assertion)
    // Moved ImGui rendering back to main thread UpdateJS() method

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
    // Phase 2.3: Execute JavaScript render logic on worker thread
    // This is called from JSGameLogicJob worker thread with v8::Locker already acquired
    if (g_scriptSubsystem && g_scriptSubsystem->IsInitialized())
    {
        // Phase 2.3 FIX: Check if JSEngine is initialized before calling it
        // First verify globalThis.JSEngine exists (defensive programming)
        String checkCode = "typeof globalThis.JSEngine !== 'undefined' && typeof globalThis.JSEngine.render === 'function'";
        bool jsEngineReady = false;

        if (g_scriptSubsystem->ExecuteScript(checkCode))
        {
            String result = g_scriptSubsystem->GetLastResult();
            jsEngineReady = (result == "true");
        }

        if (jsEngineReady)
        {
            // JSEngine is ready - execute render
            ExecuteJavaScriptCommand(StringFormat("globalThis.JSEngine.render();"));
        }
        else
        {
            // JSEngine not ready - silently skip (already logged in UpdateJSWorkerThread)
        }
    }
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
