# C++ ↔ JavaScript Bridge Architecture

Three distinct connection patterns between C++ and JavaScript in the ProtogameJS3D engine.

## Overview Diagram

```mermaid
graph TB
    subgraph JS["JS Worker Thread"]
        direction TB

        subgraph Facades["JavaScript Facade Layer"]
            EntityJS["EntityAPI.js"]
            CameraJS["CameraAPI.js"]
            AudioJS["AudioAPI.js"]
            CmdQJS["CommandQueue.js"]
            InputJS["InputInterface.js"]
            ClockJS["ClockInterface.js"]
        end

        subgraph ScriptInterfaces["V8 ScriptInterface Layer (Anti-Corruption Boundary)"]
            EntitySI["EntityScriptInterface"]
            CameraSI["CameraScriptInterface"]
            AudioSI["AudioScriptInterface"]
            GenCmdSI["GenericCommandScriptInterface"]
            InputSI["InputScriptInterface"]
            ClockSI["ClockScriptInterface"]
        end

        EntityJS --> EntitySI
        CameraJS --> CameraSI
        AudioJS --> AudioSI
        CmdQJS --> GenCmdSI
        InputJS --> InputSI
        ClockJS --> ClockSI
    end

    subgraph Queues["SPSC Ring Buffers (Lock-Free, Cross-Thread)"]
        RCQ["RenderCommandQueue\n(std::variant, 72B/cmd)\ncapacity: 1000"]
        ACQ["AudioCommandQueue\ncapacity: 200"]
        GCQ["GenericCommandQueue\n(std::any, ~208B/cmd)\ncapacity: 500"]
        CBQ["CallbackQueue\n(shared, all types)\ncapacity: 100"]
    end

    subgraph CPP["C++ Main Thread"]
        direction TB

        subgraph Processing["Command Processing (App::Update)"]
            PRC["ProcessRenderCommands()"]
            PAC["ProcessAudioCommands()"]
            PGC["ProcessGenericCommands()"]
        end

        subgraph APIs["Engine APIs"]
            EntityAPI["EntityAPI"]
            CameraAPI["CameraAPI"]
            DebugAPI["DebugRenderAPI"]
            AudioAPI["AudioAPI (C++)"]
            GCE["GenericCommandExecutor\n(handler registry)"]
        end

        PRC --> EntityAPI
        PRC --> CameraAPI
        PRC --> DebugAPI
        PAC --> AudioAPI
        PGC --> GCE

        EntityAPI --> CBQ
        CameraAPI --> CBQ
        AudioAPI --> CBQ
        GCE --> CBQ
    end

    %% Pattern 1: Async Command Queue (JS → C++)
    EntitySI -->|"Submit()"| RCQ
    CameraSI -->|"Submit()"| RCQ
    AudioSI -->|"Submit()"| ACQ
    GenCmdSI -->|"Submit()"| GCQ

    RCQ -->|"ConsumeAll()"| PRC
    ACQ -->|"ConsumeAll()"| PAC
    GCQ -->|"ConsumeAll()"| PGC

    %% Pattern 2: Sync Direct Read (JS → C++, immediate)
    InputSI -.->|"sync read\n(no queue)"| CPP
    ClockSI -.->|"sync read\n(no queue)"| CPP

    %% Pattern 3: Callback Delivery (C++ → JS)
    CBQ -->|"dequeueAll()"| JS

    %% Styling
    classDef jsClass fill:#2563eb,stroke:#1d4ed8,color:#fff
    classDef cppClass fill:#dc2626,stroke:#b91c1c,color:#fff
    classDef queueClass fill:#f59e0b,stroke:#d97706,color:#000
    classDef syncClass fill:#10b981,stroke:#059669,color:#fff
    classDef newClass fill:#8b5cf6,stroke:#7c3aed,color:#fff

    class EntityJS,CameraJS,AudioJS,CmdQJS,InputJS,ClockJS jsClass
    class EntityAPI,CameraAPI,DebugAPI,AudioAPI,PRC,PAC,PGC cppClass
    class RCQ,ACQ,GCQ,CBQ queueClass
    class InputSI,ClockSI syncClass
    class CmdQJS,GenCmdSI,GCQ,GCE,PGC newClass
```

## Three Connection Patterns

### Pattern 1: Async Command Queue (Mutation Path)

```mermaid
sequenceDiagram
    participant JS as JS Worker Thread
    participant SI as ScriptInterface
    participant Q as CommandQueue (SPSC)
    participant Main as C++ Main Thread
    participant API as Engine API
    participant CB as CallbackQueue

    JS->>SI: submit(type, payload, callback)
    SI->>Q: Submit(command)
    Note over Q: Lock-free SPSC ring buffer<br/>One-frame latency

    Main->>Q: ConsumeAll()
    Q->>Main: command
    Main->>API: ExecuteCommand(command)
    API->>CB: Enqueue(CallbackData)

    Note over CB: Next JS frame
    JS->>CB: dequeueAll()
    CB->>JS: {callbackId, resultId, error, type}
    JS->>JS: handleCallback()
```

**Characteristics:**
- Lock-free, non-blocking SPSC ring buffer
- One-frame latency (submit frame N, execute frame N+1)
- Callback delivery via shared CallbackQueue

| Queue | Payload Type | Capacity | Use Case |
|-------|-------------|----------|----------|
| RenderCommandQueue | `std::variant` (72B) | 1000 | Entity, Camera, DebugRender |
| AudioCommandQueue | typed | 200 | Sound playback |
| **GenericCommandQueue** | **`std::any` (~208B)** | **500** | **Runtime-extensible commands** |

---

### Pattern 2: Sync Direct Read (Query Path)

```mermaid
sequenceDiagram
    participant JS as JS Worker Thread
    participant SI as ScriptInterface
    participant Data as C++ Shared Data

    JS->>SI: getKeyState('W')
    SI->>Data: Read (thread-safe)
    Data-->>SI: value
    SI-->>JS: true/false (immediate)

    Note over JS,Data: No queue, no latency<br/>Thread-safe reads only
```

**Characteristics:**
- Immediate return, zero latency
- No queue involved
- Thread-safe read-only access
- Used for high-frequency queries (every frame)

| Interface | Data Source | Example Methods |
|-----------|-----------|-----------------|
| InputScriptInterface | Input state buffer | `isKeyDown()`, `getMousePos()` |
| ClockScriptInterface | Clock data | `getDeltaSeconds()`, `getFPS()` |

---

### Pattern 3: Callback Delivery (Return Path)

```mermaid
sequenceDiagram
    participant APIs as C++ APIs<br/>(Entity, Camera, Audio, GenericExecutor)
    participant CB as CallbackQueue<br/>(shared SPSC)
    participant Engine as JSEngine.processCallbacks()
    participant Handlers as JS API Handlers

    APIs->>CB: Enqueue({callbackId, resultId, error, type})
    Note over CB: Single shared queue<br/>All callback types

    Engine->>CB: callbackQueue.dequeueAll()
    CB-->>Engine: CallbackData[]

    loop For each callback
        Engine->>Engine: switch(type)
        alt ENTITY_CREATED
            Engine->>Handlers: EntityAPI.handleCallback()
        else CAMERA_CREATED
            Engine->>Handlers: CameraAPI.handleCallback()
        else RESOURCE_LOADED
            Engine->>Handlers: AudioAPI.handleCallback()
        else GENERIC
            Engine->>Handlers: CommandQueue.handleCallback()
        end
    end
```

**Characteristics:**
- Single shared `CallbackQueue` for ALL return paths
- `CallbackType` enum discriminator for routing
- Processed at start of each JS frame (`JSEngine.update()`)
- Lock-free SPSC, capacity 100

| CallbackType | Routed To | Source |
|-------------|-----------|--------|
| `ENTITY_CREATED` | `EntityAPI.handleCallback()` | EntityAPI |
| `CAMERA_CREATED` | `CameraAPI.handleCallback()` | CameraAPI |
| `RESOURCE_LOADED` | `AudioAPI.handleCallback()` | AudioAPI |
| **`GENERIC`** | **`CommandQueue.handleCallback()`** | **GenericCommandExecutor** |

---

## What GenericCommand Changes

```mermaid
graph LR
    subgraph Before["Before GenericCommand"]
        direction TB
        B1["New C++ capability"] --> B2["New XXXAPI class"]
        B2 --> B3["New XXXScriptInterface"]
        B3 --> B4["New XXXStateBuffer"]
        B4 --> B5["Wire into App.hpp/cpp"]
        B5 --> B6["New XXXAPI.js facade"]
        B6 --> B7["Recompile C++"]
    end

    subgraph After["After GenericCommand"]
        direction TB
        A1["New C++ capability"] --> A2["Write handler function"]
        A2 --> A3["RegisterHandler in App.cpp"]
        A3 --> A4["JS: CommandQueue.submit()"]
        A4 --> A5["Recompile C++"]
    end

    style Before fill:#fee2e2,stroke:#dc2626
    style After fill:#dcfce7,stroke:#16a34a
```

**Reduction**: 6 files/classes → 1 handler function + 1 registration call

---

## Industry Comparison

```mermaid
graph LR
    A["Compile-time rigid"] ---|"Unreal\n(UFUNCTION)"| B
    B ---|"Unity DOTS\n(ECB typed)"| C
    C ---|"ProtogameJS3D\n(variant + std::any)"| D
    D ---|"Godot\n(MessageQueue\n+ Callable)"| E["Runtime flexible"]

    style C fill:#8b5cf6,stroke:#7c3aed,color:#fff
```

ProtogameJS3D sits as a **pragmatic hybrid**: compile-time typed queues for hot paths (render, audio), runtime dispatch for extensibility (GenericCommand).
