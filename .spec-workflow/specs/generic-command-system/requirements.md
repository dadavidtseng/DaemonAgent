# Requirements Document

## Introduction

The GenericCommand system provides a flexible, runtime-extensible command queue architecture for the DaemonAgent dual-language game engine. This feature enables AI agents to dynamically create and submit new command types from JavaScript without requiring C++ recompilation, supporting the vision of autonomous multi-agent game development.

The current typed command system (CallbackQueue, ResourceCommandQueue, RenderCommandQueue, AudioCommandQueue) requires C++ code changes for each new command type, limiting AI agent autonomy. The GenericCommand system removes this bottleneck while maintaining the engine's performance and thread-safety guarantees.

This is a **two-phase migration**: Phase 1 introduces GenericCommand alongside existing typed commands; Phase 2 migrates existing systems to GenericCommand and removes typed command infrastructure if successful.

## How the Generic Command System Works

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     JavaScript Thread (AI Agent)                 │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ CommandQueue.submit("SpawnEntity", {pos: {x: 5}}, "agent") │ │
│  └──────────────────────┬─────────────────────────────────────┘ │
└─────────────────────────┼───────────────────────────────────────┘
                          │ (1) Submit with V8 object payload
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                GenericCommandQueue (Lock-Free SPSC)              │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ GenericCommand {                                           │ │
│  │   type: "SpawnEntity"                                      │ │
│  │   payload: v8::Persistent(v8::Object) ref to JS object    │ │
│  │   agentId: "agent"                                         │ │
│  │   timestamp: 1234567890                                    │ │
│  │ }                                                          │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────┬───────────────────────────────────────┘
                          │ (2) ConsumeAll() in C++ main thread
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│              GenericCommandExecutor (Handler Registry)           │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ std::unordered_map(string, HandlerFunc)                   │ │
│  │   "SpawnEntity"    → [](v8::Local(v8::Object) payload) { │ │
│  │                         auto pos = payload->Get("pos");   │ │
│  │                         CreateEntity(pos);                │ │
│  │                       }                                   │ │
│  │   "UpdatePhysics"  → [](payload) { ... }                 │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────┬───────────────────────────────────────┘
                          │ (3) Execute handler with payload
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                    C++ Game Systems (Engine)                     │
│  EntitySystem, PhysicsSystem, AudioSystem, etc.                 │
└─────────────────────────────────────────────────────────────────┘
```

### Execution Flow

**Step 1: JavaScript Submission**
```javascript
// AI agent submits command from JavaScript

// Option A: Fire-and-forget (no callback)
CommandQueue.submit(
  "SpawnEntity",                    // Command type (string)
  { position: { x: 5, y: 0, z: 3 }, // Payload (JavaScript object)
    prefabName: "Enemy" },
  "planner-agent"                   // Agent ID for tracking
);

// Option B: Async with callback (like EntityAPI pattern)
CommandQueue.submit(
  "SpawnEntity",
  { position: { x: 5, y: 0, z: 3 },
    prefabName: "Enemy" },
  "planner-agent",
  (result) => {                     // Optional callback function
    console.log("Entity created with ID:", result.entityId);
    console.log("Command completed in:", result.executionTimeMs, "ms");
  }
);
```

**Step 2: C++ Queue Storage**
```cpp
// GenericCommand stored in lock-free ring buffer
struct GenericCommand {
  String type;                        // "SpawnEntity"
  v8::Persistent<v8::Object> payload; // Persistent handle to JS object
  String agentId;                     // "planner-agent"
  uint64_t timestamp;                 // 1234567890

  // Async callback support (optional)
  CallbackID callbackId;              // 0 if no callback, else unique ID
  v8::Persistent<v8::Function> callback; // JavaScript callback function (if provided)
};
// V8 Persistent handle keeps JS object alive until command executes
```

**Step 3: C++ Handler Execution**
```cpp
// Main thread processes commands (App::Update)
commandQueue->ConsumeAll([&](GenericCommand const& cmd) {
  auto handler = handlerRegistry.GetHandler(cmd.type);
  if (handler) {
    v8::Local<v8::Object> payload = cmd.payload.Get(isolate);

    // Execute handler and capture result (for callbacks)
    HandlerResult result = handler(payload);

    // If callback provided, enqueue to CallbackQueue
    if (cmd.callbackId != 0) {
      CallbackData callbackData;
      callbackData.callbackId = cmd.callbackId;
      callbackData.resultData = result.toV8Object(isolate);  // Convert result to V8 object
      callbackData.errorMessage = result.error;

      callbackQueue->Enqueue(callbackData);  // Async callback delivery
    }
  }
});

// JavaScript worker thread processes callbacks
callbackQueue->DequeueAll([&](CallbackData const& cb) {
  // Look up callback function by callbackId
  auto callback = m_pendingCallbacks[cb.callbackId];

  // Execute JavaScript callback with result
  v8::Local<v8::Function> func = callback.Get(isolate);
  v8::Local<v8::Value> result = cb.resultData.Get(isolate);
  func->Call(context, v8::Undefined(isolate), 1, &result);
});
```

### Key Benefits

1. **Runtime Extensibility**: New command types added from JavaScript without C++ recompilation
2. **AI Agent Autonomy**: Agents define custom commands for new game mechanics on-the-fly
3. **Thread-Safe**: Lock-free SPSC queue + V8 Persistent handles prevent race conditions
4. **Type Flexibility**: V8 object payloads support arbitrary JSON-like structures
5. **Performance**: Less than 10µs submission latency, 1000+ commands/frame throughput
6. **Async Callback Support**: Optional callbacks following EntityAPI pattern for asynchronous operations

### Async Callback Architecture

GenericCommand supports **optional async callbacks** following the proven CallbackQueue pattern used by EntityAPI/CameraAPI:

```
┌────────────────────────────────────────────────────────────────┐
│                JavaScript Thread (AI Agent)                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ CommandQueue.submit("SpawnEntity", payload, agentId,     │  │
│  │   (result) => { console.log(result.entityId); }          │  │
│  │ );                                                        │  │
│  └───────────────────────┬──────────────────────────────────┘  │
└──────────────────────────┼─────────────────────────────────────┘
                           │ (1) Submit with callback function
                           ▼
┌────────────────────────────────────────────────────────────────┐
│           GenericCommandQueue (Lock-Free SPSC)                  │
│  GenericCommand {                                               │
│    type: "SpawnEntity", payload: {...}, agentId: "agent",      │
│    callbackId: 12345,                                           │
│    callback: v8::Persistent(v8::Function)                       │
│  }                                                              │
└───────────────────────┬────────────────────────────────────────┘
                        │ (2) Main thread processes
                        ▼
┌────────────────────────────────────────────────────────────────┐
│           GenericCommandExecutor (Main Thread)                  │
│  • Execute handler: result = handler(payload)                  │
│  • If callbackId != 0:                                         │
│      callbackQueue->Enqueue({callbackId, result, error})       │
└───────────────────────┬────────────────────────────────────────┘
                        │ (3) Enqueue callback result
                        ▼
┌────────────────────────────────────────────────────────────────┐
│              CallbackQueue (Lock-Free SPSC)                     │
│  CallbackData {                                                 │
│    callbackId: 12345,                                           │
│    resultData: v8::Persistent(v8::Object),                      │
│    errorMessage: ""                                             │
│  }                                                              │
└───────────────────────┬────────────────────────────────────────┘
                        │ (4) JavaScript worker dequeues
                        ▼
┌────────────────────────────────────────────────────────────────┐
│              JavaScript Worker Thread                           │
│  callbackQueue.DequeueAll((data) => {                          │
│    callback = lookupCallback(data.callbackId);                 │
│    callback(data.resultData);  // Execute JS callback           │
│  });                                                            │
└────────────────────────────────────────────────────────────────┘
```

**Key Design Points:**

1. **Optional Callbacks**: Submit with or without callback - both patterns supported
   ```javascript
   // Fire-and-forget (no callback)
   CommandQueue.submit("LogEvent", {msg: "test"}, "agent");

   // Async with callback
   CommandQueue.submit("CreateResource", {path: "model.obj"}, "agent", (result) => {
     console.log("Resource loaded:", result.resourceId);
   });
   ```

2. **CallbackQueue Integration**: Reuses existing `CallbackQueue` infrastructure
   - Same lock-free SPSC pattern as EntityAPI/CameraAPI
   - Same thread-safety guarantees (Main → Worker communication)
   - Same backpressure handling (queue full = drop callback with warning)

3. **Handler Result Contract**: Handlers return `HandlerResult` struct:
   ```cpp
   struct HandlerResult {
     std::unordered_map<String, std::any> data;  // Result data (converted to V8 object)
     String error;                                // Empty if success, error message if failure

     v8::Local<v8::Object> toV8Object(v8::Isolate* isolate) const;  // Convert to JS object
   };
   ```

4. **Callback Lifecycle**:
   - Submit: JavaScript stores callback, generates callbackId
   - Execute: Main thread runs handler, enqueues result to CallbackQueue
   - Deliver: JavaScript worker dequeues, executes callback, cleans up
   - Cleanup: Callback v8::Persistent handle disposed after execution

5. **Error Handling**: Errors propagated through callback
   ```javascript
   CommandQueue.submit("LoadFile", {path: "missing.txt"}, "agent", (result) => {
     if (result.error) {
       console.error("Command failed:", result.error);
     } else {
       console.log("Success:", result.data);
     }
   });
   ```

### Comparison: Typed vs Generic Commands

| Aspect | Typed Commands (Current) | GenericCommand (New) |
|--------|-------------------------|----------------------|
| **Adding New Command** | Edit C++ enum + struct + recompile | JavaScript: `CommandQueue.submit("NewType", {...})` |
| **Handler Registration** | Hardcoded in C++ switch statement | Runtime: `RegisterHandler("NewType", handler)` |
| **Payload Type** | Fixed C++ struct per command | Flexible V8 object (JSON-like) |
| **AI Agent Autonomy** | ❌ Requires C++ developer | ✅ Fully autonomous |
| **Hot-Reload Support** | ❌ C++ recompilation breaks | ✅ Handlers re-register on reload |
| **Async Callbacks** | ✅ EntityAPI/CameraAPI use CallbackQueue | ✅ **Optional** async callbacks via CallbackQueue |

### Example Use Case: AI Agent Creating Custom Command

```javascript
// AI Planner Agent: "I need a command to spawn waves of enemies"
// WITHOUT GenericCommand: Ask human to write C++, wait hours/days
// WITH GenericCommand: Define and use immediately

// Step 1: Register handler (in C++ or via script binding)
CommandQueue.registerHandler("SpawnWave", (payload) => {
  const { count, enemyType, formation } = payload;
  for (let i = 0; i < count; i++) {
    const pos = CalculateFormationPos(i, formation);
    EntityAPI.createEntity(enemyType, pos);
  }
});

// Step 2: AI agent immediately uses new command
CommandQueue.submit("SpawnWave", {
  count: 10,
  enemyType: "Zombie",
  formation: "Circle"
}, "planner-agent");
```

### Handler Implementation: C++ vs JavaScript

The GenericCommand system supports **two handler implementation approaches**, each with different trade-offs:

#### Approach A: Static C++ Handlers (Performance-Critical)

**When to use**: Performance-critical operations, low-level engine systems

```cpp
// In App::Initialize() or GameSubsystem.cpp (C++ side)
void InitializeGenericCommandHandlers() {
    // Register C++ lambda as handler (compiled, not hot-reloadable)
    GenericCommandExecutor::RegisterHandler("SpawnEntity",
        [](v8::Local<v8::Object> payload, v8::Isolate* isolate) {
            // Direct C++ implementation - maximum performance
            auto pos = V8Helper::GetVector3(payload, "position", isolate);
            auto prefab = V8Helper::GetString(payload, "prefabName", isolate);

            // Call C++ subsystems directly (no ScriptInterface overhead)
            EntityID id = EntitySystem::CreateEntity(prefab, pos);
            PhysicsSystem::AddRigidBody(id);
        }
    );
}
```

**Characteristics**:
- ✅ **Performance**: Direct C++ execution, no JavaScript overhead
- ✅ **Type Safety**: C++ compile-time type checking
- ❌ **Not Hot-Reloadable**: Requires C++ recompilation
- ❌ **No AI Agent Autonomy**: Needs C++ developer

---

#### Approach B: Dynamic JavaScript Handlers (AI Agent Autonomy)

**When to use**: AI-generated commands, rapid prototyping, game logic

```javascript
// In JSGame.js (JavaScript side) - fully hot-reloadable!
class CustomCommandHandlers {
    static initialize() {
        // Register JavaScript function as handler (runtime, hot-reloadable)
        CommandQueue.registerHandler("SpawnEntity", (payload) => {
            // JavaScript implementation - calls existing ScriptInterfaces
            const { position, prefabName } = payload;

            // Use existing C++/JS bridges (EntityScriptInterface, etc.)
            const entityId = EntityAPI.createEntity(prefabName, position);
            PhysicsAPI.addRigidBody(entityId);

            // AI agents can add custom logic here
            console.log(`AI spawned entity: ${prefabName} at`, position);
        });

        // AI agents can register new handlers at runtime
        CommandQueue.registerHandler("CustomAICommand", (payload) => {
            // Fully autonomous AI-defined behavior
            // ...
        });
    }
}
```

**Characteristics**:
- ✅ **Hot-Reloadable**: Changes apply without C++ recompilation
- ✅ **AI Agent Autonomy**: Agents define handlers at runtime
- ✅ **Rapid Prototyping**: Iterate on game logic in seconds
- ❌ **Slower**: JavaScript → C++ call overhead per subsystem call

---

#### JavaScript Handler Execution Flow

When GenericCommandExecutor encounters a JavaScript handler registered via `CommandQueue.registerHandler()`:

1. **Handler Lookup**: Executor retrieves the JavaScript function reference from the handler registry
2. **V8 Context Entry**: Executor acquires V8 isolate lock and enters the JavaScript context
3. **Function Invocation**: Executor calls `v8::Function::Call()` with the command payload as argument
4. **JavaScript Execution**: Handler function executes in V8 runtime
5. **ScriptInterface Calls**: Handler calls existing ScriptInterface APIs (EntityAPI, CameraAPI, etc.)
6. **C++ Subsystem Invocation**: ScriptInterfaces marshal calls back to C++ subsystems
7. **Return to C++**: Executor releases V8 context and continues processing next command

```
JavaScript Handler Registration:
┌─────────────────────────────────────────────────────────────┐
│ CommandQueue.registerHandler("MyCommand", handlerFunc)      │
└────────────────────┬────────────────────────────────────────┘
                     │ (Stores v8::Persistent(v8::Function))
                     ▼
┌─────────────────────────────────────────────────────────────┐
│ GenericCommandExecutor::m_jsHandlers["MyCommand"]           │
│   = v8::Persistent(v8::Function)(handlerFunc)               │
└─────────────────────────────────────────────────────────────┘

Command Execution:
┌─────────────────────────────────────────────────────────────┐
│ GenericCommandExecutor::ExecuteCommand(cmd)                 │
│   ├─ auto jsHandler = m_jsHandlers[cmd.type]                │
│   ├─ v8::HandleScope scope(isolate)                         │
│   ├─ v8::Local(v8::Function) func = jsHandler.Get(isolate)  │
│   ├─ func->Call(context, payload)  ← Invokes JS handler     │
│   └─ (Handler calls EntityAPI, CameraAPI, etc.)             │
└─────────────────────────────────────────────────────────────┘
```

**Thread Safety Note**: JavaScript handler invocation always occurs on the main thread during `App::Update()`, ensuring V8 isolate is accessed from a single thread.

---

### C++/JavaScript Connection Architecture

**Question**: Is ScriptInterface the only connection between C++ and JavaScript?

**Answer**: **No** - there are **multiple connection layers** that work together:

```
┌───────────────────────────────────────────────────────────────┐
│                    JavaScript Layer (Hot-Reloadable)           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ // AI agent submits command                             │  │
│  │ CommandQueue.submit("SpawnEntity", {...}, "agent-id")   │  │
│  │                                                          │  │
│  │ // AI agent registers handler                           │  │
│  │ CommandQueue.registerHandler("SpawnEntity", (payload) => {│ │
│  │   EntityAPI.createEntity(...);  // ← Calls existing API │  │
│  │ });                                                      │  │
│  └───────────────────┬─────────────────────────────────────┘  │
└──────────────────────┼────────────────────────────────────────┘
                       │ (1) V8 C++ Bindings
                       ▼
┌───────────────────────────────────────────────────────────────┐
│        Layer 1: GenericCommandScriptInterface (C++)            │
│  • Exposes CommandQueue.submit() to JavaScript                │
│  • Exposes CommandQueue.registerHandler() to JavaScript       │
│  • Converts V8::Object → GenericCommand struct                │
└───────────────────┬───────────────────────────────────────────┘
                    │ (2) Submits to queue
                    ▼
┌───────────────────────────────────────────────────────────────┐
│           GenericCommandQueue (Lock-Free SPSC)                 │
│  Ring buffer with v8::Persistent(v8::Object) payloads         │
└───────────────────┬───────────────────────────────────────────┘
                    │ (3) ConsumeAll in main thread
                    ▼
┌───────────────────────────────────────────────────────────────┐
│              GenericCommandExecutor (C++)                      │
│  • Handler registry (maps command type → handler function)    │
│  • Executes registered handler (C++ or JavaScript)            │
└───────────────────┬───────────────────────────────────────────┘
                    │ (4) Handler execution
                    ├─────────────────┬─────────────────────────┐
                    ▼ (C++ Handler)   ▼ (JavaScript Handler)    │
          ┌──────────────────┐  ┌─────────────────────────────┐│
          │ C++ Subsystems   │  │ JavaScript Handler Calls    ││
          │ (Direct)         │  │ Existing ScriptInterfaces   ││
          │                  │  │                             ││
          │ EntitySystem::   │  │ Layer 2: Existing APIs      ││
          │ CreateEntity()   │  │ ┌─────────────────────────┐ ││
          └──────────────────┘  │ │ EntityScriptInterface   │ ││
                                │ │ CameraScriptInterface   │ ││
                                │ │ AudioScriptInterface    │ ││
                                │ │ PhysicsScriptInterface  │ ││
                                │ └───────┬─────────────────┘ ││
                                └─────────┼───────────────────┘│
                                          ▼                     │
                                ┌───────────────────────────┐  │
                                │ C++ Game Subsystems       │  │
                                │ • EntitySystem            │  │
                                │ • PhysicsSystem           │  │
                                │ • AudioSystem             │  │
                                └───────────────────────────┘  │
                                                                │
                                                                │
```

### Connection Layers Summary

| Layer | Component | Purpose | Hot-Reloadable? | Used By |
|-------|-----------|---------|-----------------|---------|
| **Layer 1** | GenericCommandScriptInterface | Exposes `CommandQueue.submit()` and `registerHandler()` | ❌ C++ recompile | JavaScript command submission |
| **Layer 2** | EntityScriptInterface | Exposes entity operations (`createEntity`, `destroyEntity`) | ❌ C++ recompile | JavaScript handlers |
| **Layer 2** | CameraScriptInterface | Exposes camera operations (`setPosition`, `lookAt`) | ❌ C++ recompile | JavaScript handlers |
| **Layer 2** | AudioScriptInterface | Exposes audio operations (`playSound`, `stopSound`) | ❌ C++ recompile | JavaScript handlers |
| **Layer 3** | JavaScript Handlers | Custom command handlers defined in JavaScript | ✅ Hot-reload | AI agents, game logic |

### Key Architectural Insight

**GenericCommand does NOT replace existing ScriptInterfaces** - it **builds on top of them**:

```javascript
// JavaScript handler leverages existing C++/JS bridges
CommandQueue.registerHandler("ComplexGameplay", (payload) => {
    // Handler can call ANY existing ScriptInterface API
    EntityAPI.createEntity(payload.entity);      // EntityScriptInterface
    CameraAPI.setPosition(payload.cameraPos);    // CameraScriptInterface
    AudioAPI.playSound(payload.soundEffect);     // AudioScriptInterface
    PhysicsAPI.setGravity(payload.gravity);      // PhysicsScriptInterface

    // This is the power: compose existing APIs into new commands
});
```

**ScriptInterfaces are the building blocks that JavaScript handlers compose into custom commands.**

---

### Recommended Pattern: Hybrid Approach

**Best Practice**: Use C++ handlers for engine-critical operations, JavaScript handlers for gameplay:

```cpp
// C++ handlers (performance-critical engine operations)
GenericCommandExecutor::RegisterHandler("UpdatePhysics", CppUpdatePhysicsHandler);
GenericCommandExecutor::RegisterHandler("RenderDebugPrimitive", CppRenderDebugHandler);
```

```javascript
// JavaScript handlers (gameplay, AI-driven features)
CommandQueue.registerHandler("SpawnWave", JsSpawnWaveHandler);
CommandQueue.registerHandler("TriggerCutscene", JsTriggerCutsceneHandler);
CommandQueue.registerHandler("CustomAIBehavior", JsCustomAIHandler);  // AI-generated
```

### Schema Validation Architecture

The system provides **optional schema validation** for command payloads to catch errors early:

#### Schema Definition

Schemas are defined using a declarative validation API in JavaScript:

```javascript
// Define schema for a command type
CommandQueue.registerSchema("SpawnEntity", {
    position: {
        type: "object",
        required: true,
        properties: {
            x: { type: "number", required: true },
            y: { type: "number", required: true },
            z: { type: "number", required: true }
        }
    },
    prefabName: { type: "string", required: true },
    health: { type: "number", required: false, default: 100 }
});
```

#### Validation Flow

```
JavaScript Submission:
┌─────────────────────────────────────────────────────────────┐
│ CommandQueue.submit("SpawnEntity", payload, agentId)        │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│ SchemaValidator.validate(payload, schema)                   │
│   ├─ Check required fields present                          │
│   ├─ Check field types match                                │
│   ├─ Apply default values for missing optional fields       │
│   └─ Return validation result                               │
└────────────────────┬────────────────────────────────────────┘
                     │
       ┌─────────────┴─────────────┐
       │ Valid                     │ Invalid
       ▼                           ▼
┌──────────────────┐      ┌───────────────────────────────┐
│ Enqueue to C++   │      │ Throw JavaScript TypeError    │
│                  │      │ "Invalid payload for command  │
│                  │      │  SpawnEntity: missing field   │
│                  │      │  'position.x'"                │
└──────────────────┘      └───────────────────────────────┘
```

#### Performance Impact

- **Validation Cost**: ~5-10µs per command (negligible for typical payloads)
- **Optimization**: Schemas compiled to fast validation functions at registration time
- **Bypass Option**: Validation can be disabled globally via `CommandQueue.setValidationEnabled(false)` for performance-critical scenarios

---

## GenericCommand vs Existing API/StateBuffer Pattern

### Understanding the Existing Architecture

The DaemonEngine currently uses a **typed API/StateBuffer pattern** for async JavaScript-C++ integration:

```
┌────────────────────────────────────────────────────────────┐
│             Existing Typed Architecture                     │
├────────────────────────────────────────────────────────────┤
│                                                              │
│  JavaScript Thread:                                         │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ EntityAPI.createMesh(..., callback)  ──────┐         │  │
│  │ CameraAPI.create(..., callback)      ──────┤         │  │
│  └────────────────────────────────────────────┼─────────┘  │
│                                                │             │
│                                  (1) Submit typed commands  │
│                                                ▼             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │     RenderCommandQueue (Typed Commands)              │  │
│  │  • CreateEntityCommand { pos, scale, color, type }   │  │
│  │  • CreateCameraCommand { pos, orient, type }         │  │
│  │  • UpdateEntityPositionCommand { id, position }      │  │
│  └────────────────────────────────────────────┬─────────┘  │
│                                                │             │
│                                  (2) Process in main thread │
│                                                ▼             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │            EntityStateBuffer (Double-Buffered)       │  │
│  │            CameraStateBuffer (Double-Buffered)       │  │
│  │  • Worker writes to back buffer (lock-free)          │  │
│  │  • Main thread reads from front buffer (lock-free)   │  │
│  │  • SwapBuffers() at frame boundary (brief lock)      │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────┘
```

**Key Components:**
- **EntityAPI/CameraAPI**: High-level JavaScript-exposed APIs with async callbacks
- **EntityStateBuffer/CameraStateBuffer**: Double-buffered state storage using `StateBuffer<T>` template
- **RenderCommandQueue**: Typed command queue with specific command structs per operation
- **Pattern**: Each subsystem (Entity, Camera, Audio, etc.) has its own API + StateBuffer + typed commands

### What GenericCommand Changes

GenericCommand **does NOT replace EntityAPI/CameraAPI or StateBuffer** - instead, it provides a **flexible alternative to typed RenderCommandQueue**:

```
┌────────────────────────────────────────────────────────────┐
│          GenericCommand Architecture (New)                  │
├────────────────────────────────────────────────────────────┤
│                                                              │
│  JavaScript Thread:                                         │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ CommandQueue.submit("SpawnEntity", {...}, "agent")   │  │
│  │ CommandQueue.submit("UpdateCamera", {...}, "agent")  │  │
│  └────────────────────────────────────────────────────┬─┘  │
│                                                        │     │
│                              (1) Submit generic commands    │
│                                                        ▼     │
│  ┌──────────────────────────────────────────────────────┐  │
│  │     GenericCommandQueue (Flexible Commands)          │  │
│  │  GenericCommand {                                    │  │
│  │    type: "SpawnEntity"                               │  │
│  │    payload: v8::Persistent(v8::Object) (JS object)   │  │
│  │    agentId: "planner-agent"                          │  │
│  │  }                                                   │  │
│  └────────────────────────────────────────────────────┬─┘  │
│                                                        │     │
│                              (2) Execute handler registry   │
│                                                        ▼     │
│  ┌──────────────────────────────────────────────────────┐  │
│  │       GenericCommandExecutor (Handler Registry)      │  │
│  │  • "SpawnEntity" → C++ handler OR JS handler        │  │
│  │  • "UpdateCamera" → C++ handler OR JS handler       │  │
│  └────────────────────────────────────────────────────┬─┘  │
│                                                        │     │
│                   (3) Handlers call existing APIs/systems   │
│                                                        ▼     │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  EntityAPI.CreateMesh() (existing)                   │  │
│  │  CameraAPI.Create() (existing)                       │  │
│  │  EntityStateBuffer (existing, unchanged)             │  │
│  │  CameraStateBuffer (existing, unchanged)             │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────┘
```

### Architectural Relationship

| Component | Current System | GenericCommand System | Status |
|-----------|----------------|----------------------|---------|
| **EntityAPI** | ✅ Exists (high-level entity operations) | ✅ **Reused** by GenericCommand handlers | Unchanged |
| **CameraAPI** | ✅ Exists (high-level camera operations) | ✅ **Reused** by GenericCommand handlers | Unchanged |
| **EntityStateBuffer** | ✅ Exists (double-buffered entity state) | ✅ **Reused** (no changes needed) | Unchanged |
| **CameraStateBuffer** | ✅ Exists (double-buffered camera state) | ✅ **Reused** (no changes needed) | Unchanged |
| **RenderCommandQueue** | ✅ Typed commands (CreateEntity, UpdateCamera) | 🔄 **Coexists** with GenericCommandQueue (Phase 1) | Coexistence |
| **GenericCommandQueue** | ❌ Does not exist | ✅ **New** flexible command queue | New Component |
| **GenericCommandExecutor** | ❌ Does not exist | ✅ **New** handler registry | New Component |

### Key Design Decision: GenericCommand is a Layer Above APIs

**GenericCommand does NOT include EntityAPI/CameraAPI equivalents** because:

1. **Layered Architecture**: GenericCommand operates **above** existing APIs
   ```javascript
   // JavaScript handler for GenericCommand
   CommandQueue.registerHandler("SpawnEntity", (payload) => {
       // Handler CALLS existing EntityAPI
       const entityId = EntityAPI.createEntity(payload.prefabName, payload.position);
       PhysicsAPI.addRigidBody(entityId);
   });
   ```

2. **Existing APIs Remain Primary Interface**: EntityAPI and CameraAPI continue as the **main API surface** for:
   - Type-safe C++ operations
   - Direct entity/camera manipulation
   - Performance-critical paths
   - Existing game code compatibility

3. **GenericCommand Enables Composition**: Handlers compose existing APIs into **custom workflows**:
   ```javascript
   // AI agent defines complex operation using existing APIs
   CommandQueue.registerHandler("SetupLevel", (payload) => {
       // Compose EntityAPI, CameraAPI, AudioAPI into one command
       const entities = payload.entities.map(e => EntityAPI.createEntity(e.type, e.pos));
       const camera = CameraAPI.create(payload.cameraPos, payload.cameraOrient, "world");
       AudioAPI.playMusic(payload.bgm);
   });
   ```

4. **StateBuffers Remain Unchanged**: GenericCommand handlers **write through APIs**, which then use StateBuffers:
   - JavaScript handler → EntityAPI.createEntity() → EntityStateBuffer (back buffer)
   - Main thread → EntityStateBuffer.SwapBuffers() → Rendering (front buffer)

### Migration Path: Two-Phase Approach

**Phase 1: Coexistence** (Current Plan)
- EntityAPI/CameraAPI remain as primary APIs
- StateBuffers unchanged
- GenericCommand introduced for AI agent flexibility
- RenderCommandQueue coexists with GenericCommandQueue
- **No XXXAPI/StateBuffer equivalents in GenericCommand**

**Phase 2: Selective Migration** (Future Consideration)
- IF GenericCommand proves successful:
  - **Option A**: Keep both systems (typed for performance, generic for flexibility)
  - **Option B**: Migrate some typed commands to GenericCommand (case-by-case evaluation)
  - **Option C**: Create GenericEntityAPI / GenericCameraAPI as wrappers (if needed)
- StateBuffers likely remain unchanged (proven architecture)
- EntityAPI/CameraAPI may become thin wrappers over GenericCommand (if full migration occurs)

### Answer to Review Feedback

**Question**: "In the existed type command system, there are XXXAPI and XXXStateBuffer. Will the generic command system include them?"

**Answer**: **No, GenericCommand does NOT include XXXAPI/StateBuffer equivalents.** Instead:

1. **GenericCommand handlers CALL existing EntityAPI/CameraAPI** - it's a layer above, not a replacement
2. **StateBuffers remain unchanged** - proven async architecture continues to work
3. **Existing APIs remain the primary interface** - GenericCommand adds flexibility, not replaces APIs
4. **Two-phase migration ensures safety** - coexistence first, selective migration later (if successful)

This design:
- ✅ Preserves existing APIs and their thread-safety guarantees
- ✅ Enables AI agent flexibility through command composition
- ✅ Avoids duplicating EntityAPI/CameraAPI functionality
- ✅ Allows gradual adoption without breaking existing code

## Alignment with Product Vision

This feature directly supports DaemonAgent's core vision as outlined in the project documentation:

- **Dual-Language Architecture**: Strengthens JavaScript's role as the "Agent API Surface" by enabling runtime command extensibility while preserving existing EntityAPI/CameraAPI interfaces
- **AI-Driven Development**: Empowers AI agents to autonomously extend game functionality through custom commands that compose existing APIs
- **Hot-Reload Support**: Aligns with existing hot-reload philosophy—no C++ recompilation for agent-driven features
- **Multi-Agent Collaboration**: Provides shared command vocabulary for KĀDI protocol agent communication
- **Research Project Goals**: Demonstrates cutting-edge runtime extensibility for game engine architectures
- **Preserves Proven Architecture**: Maintains existing StateBuffer async pattern and EntityAPI/CameraAPI interfaces

## Requirements

### Requirement 1: Generic Command Structure

**User Story:** As an AI agent developer, I want to submit commands with dynamic payloads to the C++ engine, so that I can prototype new game mechanics without C++ code changes.

#### Acceptance Criteria

1. WHEN an AI agent creates a command with type "SpawnDynamicEntity" and payload `{position: {x: 10, y: 0, z: 5}, prefabName: "Enemy"}` THEN the system SHALL store the command with V8 object payload intact
2. WHEN the command queue capacity is reached THEN the system SHALL reject new submissions and return false
3. WHEN a command is submitted with agentId "calculator-agent" THEN the system SHALL record the agentId for audit tracking
4. IF the V8 isolate is locked by another thread THEN the system SHALL safely handle the lock contention without corruption
5. WHEN a command's V8 object payload is accessed THEN the system SHALL use proper V8 handle scopes to prevent memory leaks
6. WHEN JavaScript modifies a payload object after submission THEN the system SHALL use the object state at submission time (deep copy or immutable snapshot prevents post-submission mutations)
7. WHEN a payload contains circular references THEN the system SHALL detect and reject the command with error code ERR_CIRCULAR_REFERENCE

### Requirement 2: Runtime Handler Registry

**User Story:** As a game engine developer, I want to register command handlers at runtime, so that the engine can execute dynamically-defined command types.

#### Acceptance Criteria

1. WHEN a handler is registered for command type "UpdatePhysics" THEN subsequent commands of that type SHALL invoke the registered handler
2. WHEN a handler is unregistered for command type "SpawnEntity" THEN the system SHALL safely remove it without affecting other handlers
3. IF multiple threads attempt to register handlers concurrently THEN the system SHALL serialize access with mutex protection
4. WHEN a command type has no registered handler THEN the system SHALL log a warning with command type and agentId
5. WHEN a handler throws an exception THEN the system SHALL catch it, log the error, and continue processing remaining commands

### Requirement 3: JavaScript CommandQueue API

**User Story:** As an AI agent writing JavaScript, I want a clean API to submit commands to C++, so that I can focus on game logic without understanding engine internals.

#### Acceptance Criteria

1. WHEN calling `CommandQueue.submit("SpawnEntity", {position: {x: 5, y: 0, z: 3}}, "planner-agent")` THEN the system SHALL enqueue the command to C++ GenericCommandQueue
2. WHEN calling `CommandQueue.submit("SpawnEntity", payload, "agent", callbackFunction)` with callback THEN the system SHALL store callback, generate callbackId, and enqueue command with callback reference
3. WHEN calling `CommandQueue.registerSchema("UpdateCamera", {position: isVector3, rotation: isEulerAngles})` THEN the system SHALL validate subsequent commands against the schema
4. IF a command payload fails schema validation THEN the system SHALL throw a descriptive JavaScript error without submitting to C++
5. WHEN JavaScript queries `CommandQueue.getRegisteredTypes()` THEN the system SHALL return an array of all registered command types
6. WHEN the C++ command queue is full THEN `CommandQueue.submit()` SHALL return false and log a backpressure warning
7. WHEN calling `CommandQueue.registerHandler("SpawnWave", handlerFunction)` THEN the system SHALL register the JavaScript function as a command handler
8. WHEN JavaScript calls `CommandQueue.unregisterHandler("SpawnWave")` THEN the system SHALL remove the handler and log warnings for subsequent commands of that type
9. WHEN JavaScript files are hot-reloaded THEN the system SHALL allow handler re-registration for all command types
10. WHEN a handler is re-registered during hot-reload THEN the system SHALL replace the old handler atomically without affecting in-flight commands
11. WHEN calling `CommandQueue.executePendingCallbacks()` THEN the system SHALL dequeue callbacks from CallbackQueue and execute registered JavaScript callbacks

### Requirement 4: Thread Safety with V8 Integration

**User Story:** As an engine architect, I want GenericCommand to be thread-safe across worker threads and main thread, so that the async JavaScript architecture remains stable.

#### Acceptance Criteria

1. WHEN JavaScript worker thread submits a command with V8 object payload THEN the system SHALL use v8::Persistent handles to extend object lifetime
2. WHEN main thread executes a GenericCommand THEN the system SHALL acquire V8 isolate lock before accessing payload
3. IF V8 garbage collection occurs during command execution THEN the system SHALL prevent payload object from being collected prematurely
4. WHEN copying GenericCommand in the ring buffer THEN the system SHALL properly copy v8::Persistent handles without reference corruption
5. WHEN GenericCommandQueue is destroyed THEN the system SHALL properly dispose of all V8 persistent handles to prevent memory leaks

### Requirement 5: Safety Measures and Monitoring

**User Story:** As a game developer, I want the system to prevent AI agent abuse and provide debugging tools, so that malicious or buggy agents cannot crash the engine.

#### Acceptance Criteria

1. WHEN an agent submits more than 100 commands per second (measured via sliding 1-second window) THEN the system SHALL reject excess commands with error code ERR_RATE_LIMITED and log the rejection
2. WHEN a command is submitted THEN the system SHALL log it to an audit trail with timestamp, agentId, command type, and success/failure status
3. IF an agent submits 10 consecutive invalid commands THEN the system SHALL log a warning about potential agent malfunction
4. WHEN querying `GenericCommandQueue.GetStatistics()` THEN the system SHALL return total submitted, executed, failed, and dropped command counts per agent
5. WHEN the spec-workflow dashboard is opened THEN it SHALL display real-time command submission rates, queue depth, and top command types via WebSocket integration

### Requirement 6: Performance Characteristics

**User Story:** As an engine performance engineer, I want GenericCommand to maintain acceptable latency, so that AI agent commands do not degrade frame rate.

#### Acceptance Criteria

1. WHEN submitting a GenericCommand THEN the submission latency SHALL be less than 10 microseconds (excluding V8 overhead)
2. WHEN executing 100 GenericCommands per frame THEN the frame time overhead SHALL be less than 1 millisecond
3. IF the command queue reaches 80% capacity THEN the system SHALL log a performance warning
4. WHEN V8 object payload size exceeds 1 KB THEN the system SHALL log a warning about potential performance impact
5. WHEN GenericCommand execution is profiled THEN handler lookup latency SHALL be less than 0.1 microseconds per command

### Requirement 7: Async Callback Support (Optional)

**User Story:** As an AI agent developer, I want GenericCommand to support optional async callbacks like EntityAPI/CameraAPI, so that I can receive results from long-running or asynchronous operations.

#### Acceptance Criteria

1. WHEN calling `CommandQueue.submit("CreateResource", payload, agentId, callback)` with callback function THEN the system SHALL store the callback and generate unique callbackId
2. WHEN a handler completes execution and returns HandlerResult THEN the system SHALL enqueue CallbackData to CallbackQueue (if callback was provided)
3. WHEN JavaScript worker calls `ExecutePendingCallbacks()` THEN the system SHALL dequeue callbacks from CallbackQueue and execute JavaScript callback functions
4. IF callback execution throws JavaScript error THEN the system SHALL catch error, log warning, and continue processing remaining callbacks (error isolation)
5. WHEN callback is executed THEN the system SHALL dispose `v8::Persistent<v8::Function>` handle to prevent memory leak
6. WHEN CallbackQueue is full THEN the system SHALL drop callback, log backpressure warning, and continue (same behavior as EntityAPI)
7. WHEN handler throws C++ exception THEN the system SHALL enqueue callback with error message in CallbackData.errorMessage field
8. WHEN submitting command without callback (fire-and-forget) THEN the system SHALL set callbackId = 0 and skip CallbackQueue enqueue
9. WHEN HandlerResult contains complex data structures THEN the system SHALL convert to V8 object using `toV8Object(isolate)` method
10. WHEN multiple callbacks pending THEN the system SHALL execute in FIFO order (same as CallbackQueue for EntityAPI)

### Requirement 8: Migration Path from Typed Commands

**User Story:** As an engine maintainer, I want a clear migration strategy from typed to generic commands, so that existing systems transition smoothly without breaking changes.

#### Acceptance Criteria

1. WHEN Phase 1 is complete THEN both typed commands (EntityAPI, CameraAPI) and GenericCommand SHALL coexist without conflicts
2. WHEN an existing typed command is migrated to GenericCommand THEN the JavaScript API SHALL remain backward compatible
3. IF GenericCommand proves unstable THEN the system SHALL allow rollback to typed commands without code loss
4. WHEN Phase 2 migration is complete THEN all typed command queues SHALL be removed and GenericCommand SHALL handle all operations
5. WHEN typed commands are removed THEN the system SHALL maintain or improve performance compared to typed command baseline

## Non-Functional Requirements

### Code Architecture and Modularity

- **Single Responsibility Principle**: GenericCommandQueue handles command queueing only; handler registry and execution are separate concerns
- **Modular Design**:
  - `GenericCommand.hpp` - Command structure definition (includes callbackId and callback fields)
  - `GenericCommandQueue.hpp` - Queue implementation using CommandQueueBase template
  - `GenericCommandExecutor.hpp` - Handler registry, execution logic, and CallbackQueue integration
  - `GenericCommandScriptInterface.hpp` - JavaScript V8 bindings
  - `CommandQueue.js` - JavaScript API facade (includes callback management)
  - `HandlerResult.hpp` - Handler return value structure for callback data
- **Dependency Management**:
  - GenericCommand depends on V8 API and CallbackQueue
  - GenericCommandQueue inherits from CommandQueueBase (reuses existing SPSC lock-free implementation)
  - GenericCommandExecutor depends on CallbackQueue for async result delivery
  - Reuses existing CallbackQueue infrastructure (no new queue implementation needed)
- **Clear Interfaces**:
  - C++ handler registry uses `std::function<HandlerResult(v8::Local<v8::Object>)>` for handler signatures
  - JavaScript API supports optional callbacks: `submit(type, payload, agentId, callback?)`
  - CallbackQueue interface identical to EntityAPI/CameraAPI pattern
  - V8 integration uses standard v8::Persistent and v8::Local handle patterns

### Performance

- **Submission Latency**: < 10µs per command (excluding V8 object copy overhead and callback storage)
- **Execution Throughput**: 1000+ commands per frame without frame rate impact (target: 60 FPS)
- **Callback Overhead**: < 5µs per callback enqueue to CallbackQueue (same as EntityAPI)
- **Memory Overhead**:
  - GenericCommand size: ~400 bytes (v8::Persistent payload + callback + metadata)
  - Queue capacity: 500 commands (200 KB total memory)
  - CallbackQueue: Reuses existing 100-callback queue (4 KB, shared with EntityAPI/CameraAPI)
  - Handler registry: O(1) hash map lookup
- **V8 Integration**: Minimize isolate lock contention through batched command and callback execution

### Security

- **Rate Limiting**: 100 commands/second per agent (sliding 1-second window, configurable via GameConfig.xml)
- **Input Validation**: JavaScript-side schema validation before C++ submission
- **Audit Trail**: All commands logged with agentId, timestamp, type, and execution result to `.spec-workflow/command-audit-log.json`
- **Sandboxing**: Handlers execute in isolated try-catch blocks; failures do not propagate
- **Memory Safety**: V8 handles properly disposed; no dangling references to JavaScript objects

### Reliability

- **Error Isolation**: Handler exceptions logged but do not crash engine or affect other commands
- **Backpressure Handling**: Queue full conditions logged and returned to JavaScript as failure
- **Thread Safety**: Mutex-protected handler registry; lock-free command queue (SPSC)
- **Graceful Degradation**: Unknown command types logged as warnings; system continues processing
- **Memory Leak Prevention**: V8 persistent handles tracked and disposed on queue destruction

### Usability

- **JavaScript API Simplicity**: Single-function submission: `CommandQueue.submit(type, payload, agentId)`
- **Schema Validation**: Optional client-side validation with clear error messages
- **Debugging Support**:
  - Chrome DevTools integration for JavaScript command inspection
  - Audit log export for command history analysis
  - Dashboard visualization of command flow and statistics (integrated with existing spec-workflow dashboard at `http://localhost:5000`)
- **Documentation**:
  - JSDoc comments for all JavaScript APIs
  - C++ Doxygen comments for engine integration
  - Migration guide from typed to generic commands
- **Hot-Reload Compatibility**: Command handlers can be re-registered after JavaScript file reload without losing state

### Dashboard Integration

The GenericCommand monitoring features integrate with the existing **spec-workflow dashboard** (http://localhost:5000):

- **Real-time Statistics**: WebSocket notifications push command queue metrics to dashboard
- **Command History**: Stored in `.spec-workflow/command-audit-log.json` for retrospective analysis
- **Visualization Components**:
  - Command submission rate graph (commands/second per agent)
  - Queue depth gauge (current utilization percentage)
  - Top command types bar chart (most frequently used commands)
  - Agent activity heatmap (which agents are most active)
- **Integration Points**:
  - Reuses existing WebSocket infrastructure from spec-workflow MCP server
  - Command audit log follows same JSON format as implementation logs
  - Dashboard UI extends existing React components for consistency
