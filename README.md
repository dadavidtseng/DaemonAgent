# daemon-agent
> Lightweight C++ game daemon agent for AGENTS multi-agent orchestration

Overview
--------
`daemon-agent` is a C++-based game agent designed to run as a background daemon inside the AGENTS orchestration environment. It exposes an IPC interface for other agents, runs the core game loop, and provides hook points for AI modules, logging, and metrics. The package is intended to be managed by the platform toolchain (npm + kadi) while the core is built with CMake.

Quick Start
-----------
1. Install Node toolchain dependencies:
- Run `npm install` in the package root.

2. Install platform tools and registered plugins:
- Run `kadi install` (this will read `kadi.yml` and install runtime tooling).

3. Start the daemon under kadi supervision:
- Run `kadi run start`

Typical sequence:
- `npm install`
- `kadi install`
- `kadi run start`

Tools
-----
| Tool | Description |
|------|-------------|
| `logger` | Central logging tool that captures and routes stdout/stderr and structured logs to file and the platform log backend. Controls `logLevel` and `logPath` (see config). |
| `ipc` | Local IPC transport (Unix domain socket / named pipe). Exposes `ipcPath` for incoming agent connections and provides message framing for JSON-RPC style commands. |
| `game-state` | In-memory game state manager offering snapshotting and state mutation APIs. Used by the core game loop and AI modules. |
| `ai` | Pluggable AI driver loader. Loads AI modules from `ai/modules/` and forwards events and action requests. |
| `metrics` | Metrics exporter that exposes runtime counters and histograms over a local HTTP `/metrics` endpoint. Configurable via `metrics.port`. |

Configuration
-------------
Configuration files and locations:
- `config/daemon-agent.yml` — primary YAML config used by the daemon at startup.
- `config/daemon.json` — alternate JSON config (supported for local tooling).
- `.env` — environment overrides for runtime-sensitive settings.
- `kadi.yml` — kadi run definitions and tool registrations.
- `package.json` — Node build and lifecycle hooks.

Common configuration fields (keys present in `config/daemon-agent.yml`):
- `agentId` (string) — Unique identifier for this daemon (e.g., `daemon-1`).
- `logLevel` (string) — `debug|info|warn|error` (default: `info`).
- `logPath` (string) — Path to the log file (default: `logs/daemon.log`).
- `ipcPath` (string) — Unix socket or named pipe path for IPC (default: `/tmp/daemon-agent.sock`).
- `port` (integer) — HTTP/metrics port (default: `8081`).
- `gameTickMs` (integer) — Milliseconds per game tick (default: `50`).
- `maxPlayers` (integer) — Maximum concurrent players this daemon supports (default: `64`).
- `aiEnabled` (boolean) — Enable the AI loader (default: `true`).
- `aiConfig.path` (string) — Path to AI modules (default: `ai/modules/`).
- `metrics.port` (integer) — Port used by the metrics exporter (default: `9090`).

Example (conceptual) values in `config/daemon-agent.yml`:
- `agentId: "daemon-1"`
- `logLevel: "debug"`
- `ipcPath: "/tmp/daemon-agent.sock"`
- `gameTickMs: 33`

Files and important paths:
- `src/main.cpp` — application entry point (parses config, initializes tools, starts loop).
- `src/daemon.cpp` — main daemon lifecycle and supervisor.
- `src/game_engine.cpp` — core tick loop and game state updates.
- `src/agent_worker.cpp` — per-connection worker handling IPC messages.
- `include/*.h` — public headers for modules and plugins.
- `CMakeLists.txt` — build configuration for the C++ project.
- `kadi.yml` — declares `start`, `stop`, `build`, and any tool registrations.
- `package.json` — contains Node lifecycle hooks (preinstall/postinstall) if needed.

Architecture
------------
High level data flow and components:

- CLI / Orchestrator (kadi)
  - Starts the daemon using the `kadi run start` definition.
  - Provides environment and tool lifecycle.

- Daemon bootstrap (`src/main.cpp`, `config/daemon-agent.yml`)
  - Loads configuration and sets process-wide parameters (logging, IPC path, metrics port).
  - Initializes registered tools (`logger`, `ipc`, `metrics`, `game-state`, `ai`).

- IPC layer (`src/agent_worker.cpp`, tool `ipc`)
  - Listens on `ipcPath` and accepts inbound agent connections.
  - Frames messages (JSON-RPC style) and dispatches commands to the command router.

- Command Router / Worker
  - Validates and routes incoming commands to either the game engine, AI modules, or persistence layer.
  - Runs command handlers on worker threads (thread pool managed by the daemon).

- Game Engine Core (`src/game_engine.cpp`, tool `game-state`)
  - Runs a deterministic tick loop at `gameTickMs` intervals.
  - Applies state changes, produces events, and emits snapshots.
  - Exposes synchronous APIs for read-only queries and asynchronous APIs for state mutation.

- AI Modules (`ai/modules/*`, tool `ai`)
  - Loaded dynamically from `aiConfig.path`.
  - Subscribe to events and request actions through the command router.
  - Can be enabled/disabled via `aiEnabled`.

- Metrics and Logging (`metrics`, `logger`)
  - Metrics exporter exposes counters and histograms on `metrics.port`.
  - Logger writes structured logs to `logPath` and supports `logLevel` runtime changes.

Key runtime interactions:
- Incoming IPC message -> agent_worker -> command router -> game_engine or ai -> state change -> game_engine emits events -> metrics/logger record results -> optional persistence.

Development
-----------
Build and test steps (native C++ build using CMake):
- Install Node deps: `npm install`
- Install kadi tools: `kadi install`
- Configure environment: copy `config/daemon-agent.yml.example` to `config/daemon-agent.yml` and edit fields.
- Build native binaries:
  - `mkdir -p build && cd build`
  - `cmake ..`
  - `cmake --build . --config Release`
- Run unit tests:
  - `ctest --output-on-failure` (from `build`)

Development notes:
- Code layout:
  - Public headers: `include/`
  - Sources: `src/`
  - AI modules: `ai/modules/`
  - Tests: `test/`
- Style and tooling:
  - Use `clang-format` for C++ sources (project includes `.clang-format` rules).
  - Run static analysis via `clang-tidy` as part of CI (configured in `build/`).
- Hot reload:
  - AI modules support hot reload by touching a module file; the `ai` tool watches `aiConfig.path` by default.
- Debugging:
  - Start with `kadi run start --env DEBUG=true` or set `logLevel: debug` in config.
  - IPC socket path is `ipcPath` — interact using `socat`, `nc`, or the AGENTS test harness.

Packaging & Deployment
- Binaries are produced into `build/bin/`.
- Use `kadi` to manage deployments: `kadi run deploy` (deployment steps provided in `kadi.yml`).
- When packaging a release, include:
  - `build/bin/daemon-agent` binary
  - `config/daemon-agent.yml` example
  - `ai/modules/` if bundling AI
  - `kadi.yml` for runtime orchestration

Contributing
------------
- Follow the repository branching and PR guidelines.
- Run `npm test` and native tests (`ctest`) before submitting a PR.
- Add documentation for any added config fields or tools in `docs/`.

License and Contacts
--------------------
- See `LICENSE` in the repository root for license terms.
- For platform integration questions, consult the AGENTS developer guide or open an issue in the repository.