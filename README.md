# daemon-agent
> DaemonAgent — dual-language game engine bridging a C++ DaemonEngine foundation with embedded V8 JavaScript game logic

Overview
--------
DaemonAgent (formerly "daemon-agent") is a research-grade C++ game application that embeds the Google V8 JavaScript engine to enable a dual-language architecture: performance-critical systems (rendering, audio, entity management) live in C++ (DaemonEngine) while game logic and rapid prototyping are written in JavaScript and executed in V8. The project emphasizes hot-reloadable JavaScript logic, crash isolation between language runtimes, and a frame-based double-buffered communication pattern (EntityStateBuffer / CameraStateBuffer) to achieve stable rendering and lock-free parallelism.

Quick Start
-----------
Prerequisites
- Microsoft Visual Studio 2022 with the C++ development workload
- Windows 10/11 (x64)
- NuGet for package dependencies

Build & run
1. Open the solution:
   - Open `DaemonAgent.sln` in Visual Studio 2022.
2. Choose a configuration:
   - Select `Debug|x64` or `Release|x64`.
3. Build the solution:
   - Build → Build Solution (Ctrl+Shift+B).
4. Run the application:
   - From the IDE: Start Debugging / Start Without Debugging.
   - Or from the Run folder:
     - cd Run
     - Run the produced binary (e.g., `DaemonAgent_Debug_x64.exe` or `DaemonAgent_Release_x64.exe`).

If you are working on the JavaScript side:
- Edit scripts under `Run/Data/Scripts/` — hot-reload is supported by the engine.
- Use Chrome DevTools integration for JS debugging when running the application.
- Primary JS entry point: `Run/Data/Scripts/main.js` (ES6 module). The JS framework exposes a JSEngine and uses a CommandQueue facade (see `Run/Data/Scripts/Interface/CommandQueue.js`). Note: `main.js` creates the `CommandQueue` instance and sets `globalThis.CommandQueueAPI` before the game systems are initialized (CommandQueue MUST be available to JS systems such as `JSGame`).

Tools
-----
| Tool / Subsystem | Description |
|------------------|-------------|
| V8 Subsystem (embedded) | Embedded JavaScript runtime for game logic and hot-reloadable systems. Integrated with Chrome DevTools for debugging. |
| DaemonEngine (Engine/) | C++ game engine foundation providing Core, Entity, Camera, Rendering, Audio, and Resource management subsystems. |
| Entity API / EntityScriptInterface | C++ ↔ JS bindings for entity management. Includes `EntityStateBuffer` double-buffering for lock-free state exchange. |
| Camera API / CameraScriptInterface | C++ ↔ JS bindings for camera control with `CameraStateBuffer`. |
| Rendering (DirectX) | DirectX-based renderer implemented in the engine (Engine/Renderer/). |
| Audio (FMOD) / AudioStateBuffer | FMOD integration for audio playback and mixing. Audio can be conditionally disabled at build-time via `Code/Game/EngineBuildPreferences.hpp` (`ENGINE_DISABLE_AUDIO`). `AudioStateBuffer` is used alongside `EntityStateBuffer` for audio state exchange. |
| Hot-Reload System (JSEngine) | `JSEngine` (JS-side, `Run/Data/Scripts/JSEngine.js`) manages JS systems, their priorities, and hot-reload replacement. Uses an internal event bus (`Event/EventBus.js`) and priority-based execution (0-100, lower = earlier). `main.js` is the ES6 entry module loaded by the C++ host. |
| DevTools Integration | Chrome DevTools connection for JavaScript inspection and breakpointing at runtime. |
| Command/Callback Queues | JS-visible `CommandQueue` facade and C++ lock-free/queued mechanisms for submitting render and engine commands. Callback processing uses a queued/lock-free design (see `CallbackQueue` / `CallbackQueueScriptInterface`). |

Configuration
-------------
Primary runtime configuration now lives under the Run/Data tree and engine/project files:

- `Run/Data/GameConfig.xml` — primary runtime configuration for the application (game-specific settings).
- `Run/Data/Scripts/` — JavaScript game logic and hot-reloadable modules; entry point `main.js`.
- `Run/Data/` — assets (models, shaders, textures, audio) and other data files.
- `Code/Game/EngineBuildPreferences.hpp` — per-game engine build flags. The current game header defines `ENGINE_DEBUG_RENDER`, `CONSOLE_HANDLER`, and `ENGINE_SCRIPTING_ENABLED` by default. Note: `ENGINE_SCRIPTING_ENABLED` is used in Phase 2 to enable async audio support via the AudioCommandQueue. `ENGINE_DISABLE_AUDIO` remains available (commented-out by default) to disable FMOD/audio code and linkage if needed.
- `DaemonAgent.sln` — Visual Studio solution and build configuration.
- `Code/Game/` — C++ game application sources that reference the external `Engine/` repository.

Common runtime settings are configured in `Run/Data/GameConfig.xml` and per-script configuration may be present inside `Run/Data/Scripts/`. Hot-reload and debugging behavior are controlled by runtime flags and `GameConfig.xml`.

Files and important paths
-------------------------
- `DaemonAgent.sln` — Visual Studio solution (entry point for development).
- `Code/Game/` — main application sources (C++). Look for `App.cpp` / `App` entry points that drive the main loop. Notable files:
  - `Code/Game/EngineBuildPreferences.hpp` — game-specific engine build preference flags (now defines `ENGINE_DEBUG_RENDER`, `CONSOLE_HANDLER`, and `ENGINE_SCRIPTING_ENABLED` by default; `ENGINE_DISABLE_AUDIO` may be uncommented to disable audio).
  - `Code/Game/Framework/App.hpp` — application lifecycle methods: `Startup()`, `Shutdown()`, `RunFrame()`, and `RunMainLoop()`. Uses `EntityStateBuffer` and `AudioStateBuffer`. The App class contains the per-process quitting state (`static bool m_isQuitting`) and exposes frame-phase methods (`BeginFrame()`, `Update()`, `Render()`, `EndFrame()`).
  - `Code/Game/Framework/GameScriptInterface.hpp` — exposes higher-level scriptable methods (including KADI dev-tool related file operations, input injection, and file-watcher management) to JS. New script-callable methods include create/read/delete script file operations, key-hold sequence handling, key-hold status queries, cancellation, and listing active key holds, plus watched-file add/remove APIs.
  - `Code/Game/Framework/JSGameLogicJob.hpp` — worker-thread job for JavaScript execution (uses JobSystem primitives and thread synchronization). The JS worker is implemented around an abstract `IJSGameLogicContext` and coordinates with callback/command queues.
  - `Code/Game/Framework/GameCommon.hpp` — common helpers and globals (`g_app`, `g_game`) and utilities such as the `GAME_SAFE_RELEASE` macro.
- `Engine/` — external engine library containing:
  - `Engine/Entity/` — `EntityAPI`, `EntityScriptInterface`, `EntityStateBuffer`
  - `Engine/Renderer/` — `CameraAPI`, `CameraScriptInterface`, `CameraStateBuffer`
  - `Engine/Audio/` — `AudioStateBuffer` and audio subsystem (FMOD integration unless disabled)
  - `Engine/Core/` — shared `StateBuffer` template and core utilities, `JobSystem.hpp`
- `Run/Data/Scripts/` — JavaScript game logic, including:
  - `main.js` — ES6 entry point (creates `CommandQueue`, sets `globalThis.CommandQueueAPI`, initializes `JSEngine`/`JSGame`); contains hot-reload cleanup logic for prior JS game instances.
  - `JSEngine.js` — core JS framework for system registration, priority execution, hot-reload handling, and event-bus integration.
  - `JSGame.js` — JS-side game orchestration (imported by `main.js`) that wires up subsystems such as input and audio.
  - `Interface/CommandQueue.js` — CommandQueue facade exposed to JS for engine interactions. `main.js` creates an instance and exposes it as `globalThis.CommandQueueAPI`.
  - `InputSystemCommon.js` — shared keycode/constants used by input-related systems.
  - `test_scripts.js` — example/test scripts.
- `Run/Data/` — assets and configuration (`GameConfig.xml`).
- `Docs/` — extended project documentation and technical design notes (e.g., async architecture documents).
- `Run/` — runtime output folder containing build artifacts and packaged binaries after a successful build.

Architecture
------------
DaemonAgent implements a dual-language, frame-based parallel runtime where C++ rendering and JavaScript game logic run on separate threads and communicate via lock-free, double-buffered state buffers.

Runtime execution flow (high level)
- Main thread (C++ / App.cpp):
  - BeginFrame()
  - Update() -> reads front buffers (EntityStateBuffer, CameraStateBuffer, AudioStateBuffer) and executes C++ systems
  - Render() -> stable 60 FPS rendering on DirectX
  - EndFrame() -> brief buffer swap with the worker thread (<1ms lock)
- Worker thread (JavaScript / JSGameLogicJob):
  - Runs a persistent V8 execution context under a worker job (`JSGameLogicJob`) using the engine JobSystem
  - Uses `std::mutex` + `std::condition_variable` to coordinate frame wake/sleep between main and worker
  - All V8 API calls are protected by `v8::Locker` to ensure thread safety
  - Writes to back buffers (EntityStateBuffer / CameraStateBuffer / AudioStateBuffer) and submits render/engine commands via CommandQueue
  - The worker is implemented against an abstract `IJSGameLogicContext` to allow clear separation of the JS execution context from job orchestration
- Communication:
  - Double-buffered state buffers (EntityStateBuffer, CameraStateBuffer, AudioStateBuffer) provide N/N+1 frame isolation
  - Render/engine command queues and minimal synchronization points avoid blocking the main render thread
  - CallbackQueue / CommandQueue patterns are used for async callback and command execution

Key components
- App / Main loop: `Code/Game/Framework/App.hpp` drives lifecycle and exposes static events (e.g., `RequestQuit`, `OnCloseButtonClicked`).
- JS worker job: `JSGameLogicJob` — dedicated JS worker using JobSystem primitives, conditional variables, and V8 protections to isolate JS execution and enable hot-reload safety. It coordinates with an `IJSGameLogicContext` and callback queues.
- EntityScriptInterface / CameraScriptInterface: C++ bindings exposed to JavaScript for manipulating entities and cameras.
- State buffers: lock-free, double-buffered structures for safe cross-thread state exchange (`EntityStateBuffer`, `CameraStateBuffer`, `AudioStateBuffer`).
- JSEngine (JS-side): `Run/Data/Scripts/JSEngine.js` — system registration, priority execution, and automatic subsystem hot-reload replacement; uses an EventBus pattern for dependency/inversion.
- CommandQueue (JS/C++): JS-visible `CommandQueue` API (created in `main.js`) used to enqueue engine/renderer commands from JS logic.
- GameScriptInterface: `Code/Game/Framework/GameScriptInterface.hpp` exposes script methods for dev-tools: create/read/delete script files, input injection (key press/hold/sequence), key-hold queries/cancellation, listing active key-holds, and watched-file management (add/remove).
- Hot-reload: V8 script watcher