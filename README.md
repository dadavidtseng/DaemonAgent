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
- Primary JS entry point: `Run/Data/Scripts/main.js` (ES6 module). The JS framework exposes a JSEngine and uses a CommandQueue facade (see `Run/Data/Scripts/Interface/CommandQueue.js`).

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
| Hot-Reload System (JSEngine) | `JSEngine` (JS-side, `Run/Data/Scripts/JSEngine.js`) manages JS systems, their priorities, and hot-reload replacement. `main.js` is the ES6 entry module loaded by the C++ host. |
| DevTools Integration | Chrome DevTools connection for JavaScript inspection and breakpointing at runtime. |
| Command/Callback Queues | JS-visible `CommandQueue` facade and C++ lock-free/queued mechanisms for submitting render and engine commands. Callback processing uses a queued/lock-free design (see `CallbackQueue` / `CallbackQueueScriptInterface`). |

Configuration
-------------
Primary runtime configuration now lives under the Run/Data tree and engine/project files:

- `Run/Data/GameConfig.xml` — primary runtime configuration for the application (game-specific settings).
- `Run/Data/Scripts/` — JavaScript game logic and hot-reloadable modules; entry point `main.js`.
- `Run/Data/` — assets (models, shaders, textures, audio) and other data files.
- `Code/Game/EngineBuildPreferences.hpp` — per-game engine build flags (e.g., `ENGINE_DISABLE_AUDIO`, `ENGINE_SCRIPTING_ENABLED`, `ENGINE_DEBUG_RENDER`, `CONSOLE_HANDLER`).
- `DaemonAgent.sln` — Visual Studio solution and build configuration.
- `Code/Game/` — C++ game application sources that reference the external `Engine/` repository.

Common runtime settings are configured in `Run/Data/GameConfig.xml` and per-script configuration may be present inside `Run/Data/Scripts/`. Hot-reload and debugging behavior are controlled by runtime flags and `GameConfig.xml`.

Files and important paths
-------------------------
- `DaemonAgent.sln` — Visual Studio solution (entry point for development).
- `Code/Game/` — main application sources (C++). Look for `App.cpp` / `App` entry points that drive the main loop. Notable files:
  - `Code/Game/EngineBuildPreferences.hpp` — game-specific engine build preference flags.
  - `Code/Game/Framework/App.hpp` — application lifecycle methods: `Startup()`, `Shutdown()`, `RunFrame()`, and `RunMainLoop()`. Uses `EntityStateBuffer` and `AudioStateBuffer`.
  - `Code/Game/Framework/GameScriptInterface.hpp` — exposes higher-level scriptable methods (including KADI dev-tool related file operations, input injection, and file-watcher management) to JS.
  - `Code/Game/Framework/JSGameLogicJob.hpp` — worker-thread job for JavaScript execution (uses JobSystem primitives and thread synchronization).
- `Engine/` — external engine library containing:
  - `Engine/Entity/` — `EntityAPI`, `EntityScriptInterface`, `EntityStateBuffer`
  - `Engine/Renderer/` — `CameraAPI`, `CameraScriptInterface`, `CameraStateBuffer`
  - `Engine/Audio/` — `AudioStateBuffer` and audio subsystem (FMOD integration unless disabled)
  - `Engine/Core/` — shared `StateBuffer` template and core utilities, `JobSystem.hpp`
- `Run/Data/Scripts/` — JavaScript game logic, including:
  - `main.js` — ES6 entry point (sets up `JSEngine`, `JSGame`, and `CommandQueue`); supports hot-reload cleanup of prior JS state.
  - `JSEngine.js` — core JS framework for system registration, priority execution, and hot-reload handling.
  - `Interface/CommandQueue.js` — CommandQueue facade exposed to JS for engine interactions.
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
- Communication:
  - Double-buffered state buffers (EntityStateBuffer, CameraStateBuffer, AudioStateBuffer) provide N/N+1 frame isolation
  - Render/engine command queues and minimal synchronization points avoid blocking the main render thread

Key components
- App / Main loop: `Code/Game/Framework/App.hpp` drives lifecycle and exposes static events (e.g., `RequestQuit`, `OnCloseButtonClicked`).
- JS worker job: `JSGameLogicJob` — dedicated JS worker using JobSystem primitives, conditional variables, and V8 protections to isolate JS execution and enable hot-reload safety.
- EntityScriptInterface / CameraScriptInterface: C++ bindings exposed to JavaScript for manipulating entities and cameras.
- State buffers: lock-free, double-buffered structures for safe cross-thread state exchange (`EntityStateBuffer`, `CameraStateBuffer`, `AudioStateBuffer`).
- JSEngine (JS-side): `Run/Data/Scripts/JSEngine.js` — system registration, priority execution, and automatic subsystem hot-reload replacement.
- CommandQueue (JS/C++): JS-visible `CommandQueue` API (created in `main.js`) used to enqueue engine/renderer commands from JS logic.
- GameScriptInterface: `Code/Game/Framework/GameScriptInterface.hpp` exposes script methods for dev-tools: create/read/delete script files, input injection (key press/hold/sequence), key-hold queries/cancellation, and watched-file management (add/remove).
- Hot-reload: V8 script watcher and JS-side hot-reload handling (e.g., `main.js` cleans up previous game objects and recreates systems).
- DevTools: Chrome DevTools integration for JS inspection and breakpointing.

Development
-----------
Recommended workflow
1. Open `DaemonAgent.sln` in Visual Studio 2022.
2. Build the solution (`Debug|x64` or `Release|x64`).
3. Edit C++ code under `Code/Game/` and engine code under `Engine/` (if working with the engine repo).
4. Edit JavaScript logic under `Run/Data/Scripts/` and use hot-reload to iterate quickly. Primary JS entry is `main.js`; `JSEngine.js` provides the system framework.

Coding standards and notes
- C++: C++20 standard, RAII patterns, SOLID-based modular design. The project emphasizes Dependency Inversion: application code depends on `EntityAPI`/`CameraAPI` abstractions.
- JavaScript: ES6+ modules and class-based systems optimized for hot-reload compatibility. `main.js` is an ES6 module and sets up global JS framework objects (e.g., `globalThis.CommandQueueAPI`, `globalThis.jsGameInstance`) to support hot-reload swaps.
- Build-time flags: Customize engine build behavior for the game in `Code/Game/EngineBuildPreferences.hpp` (e.g., `ENGINE_DISABLE_AUDIO`, `ENGINE_SCRIPTING_ENABLED`, `ENGINE_DEBUG_RENDER`, `CONSOLE_HANDLER`).
- Debugging: Use Visual Studio for native debugging and Chrome DevTools for JS runtime debugging.

Testing & QA
- Manual integration testing for dual-language interactions and hot-reload workflows.
- JavaScript test scripts located at `Run/Data/Scripts/test_scripts.js`.
- Performance profiling and interop latency testing are part of the development checklist.
- JS worker timeout/hang detection and graceful rollback behaviors are implemented in the async architecture design.

Packaging & Deployment
----------------------
- Build artifacts are produced by Visual Studio / MSBuild into the `Run/` or configured output directories.
- Packaged releases should include:
  - Built executable(s) from `Run/` (e.g., `DaemonAgent_*.exe`)
  - `Run/Data/GameConfig.xml`
  - `Run/Data/Scripts/` (JS modules) and critical assets
  - `Docs/` (usage and architecture notes) as needed for researchers or integrators

Contributing
------------
- Follow the repository branching and PR guidelines.
- Document any added engine interfaces, JS APIs, or config fields in `Docs/`.
- Run native tests and validate JS hot-reload scenarios before submitting PRs.
- If contributing engine-side changes, ensure compatibility with the interop bindings: `EntityScriptInterface`, `CameraScriptInterface`, and `StateBuffer` semantics.
- When adding game-specific build behavior, prefer `Code/Game/EngineBuildPreferences.hpp` to toggle engine features for this game.

License and Contacts
--------------------
- See `LICENSE` in the repository root for license terms (project uses Apache 2.0 as indicated in Docs).
- For architecture or integration questions consult `Docs/` (including async architecture and design notes) or open an issue in the repository.

References
----------
- Code/Game/CLAUDE.md — Game module documentation and design notes
- Docs/async-architecture-technical-document.md — Asynchronous architecture and frame-based parallelism details
- Engine/ — External DaemonEngine repository (Entity, Camera, Core subsystems)
- Run/Data/Scripts/main.js — JS entry module and hot-reload cleanup pattern
- Run/Data/Scripts/JSEngine.js — JS-side system registration and hot-reload framework

If any AGENTS/platform-specific orchestration (kadi/npm) integration is still required for your deployment, mention it in an issue — the project currently focuses on a native Windows + Visual Studio workflow and the dual-language engine integration documented above.