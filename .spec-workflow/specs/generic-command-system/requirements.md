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
│  │   payload: v8::Persistent<v8::Object> (ref to JS object)  │ │
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
│  │ std::unordered_map<string, HandlerFunc>                   │ │
│  │   "SpawnEntity"    → [](v8::Local<v8::Object> payload) { │ │
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
CommandQueue.submit(
  "SpawnEntity",                    // Command type (string)
  { position: { x: 5, y: 0, z: 3 }, // Payload (JavaScript object)
    prefabName: "Enemy" },
  "planner-agent"                   // Agent ID for tracking
);
```

**Step 2: C++ Queue Storage**
```cpp
// GenericCommand stored in lock-free ring buffer
struct GenericCommand {
  String type;                       // "SpawnEntity"
  v8::Persistent<v8::Object> payload; // Persistent handle to JS object
  String agentId;                    // "planner-agent"
  uint64_t timestamp;                // 1234567890
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
    handler(payload);  // Execute registered C++ handler
  }
});
```

### Key Benefits

1. **Runtime Extensibility**: New command types added from JavaScript without C++ recompilation
2. **AI Agent Autonomy**: Agents define custom commands for new game mechanics on-the-fly
3. **Thread-Safe**: Lock-free SPSC queue + V8 Persistent handles prevent race conditions
4. **Type Flexibility**: V8 object payloads support arbitrary JSON-like structures
5. **Performance**: Less than 10µs submission latency, 1000+ commands/frame throughput

### Comparison: Typed vs Generic Commands

| Aspect | Typed Commands (Current) | GenericCommand (New) |
|--------|-------------------------|----------------------|
| **Adding New Command** | Edit C++ enum + struct + recompile | JavaScript: `CommandQueue.submit("NewType", {...})` |
| **Handler Registration** | Hardcoded in C++ switch statement | Runtime: `RegisterHandler("NewType", handler)` |
| **Payload Type** | Fixed C++ struct per command | Flexible V8 object (JSON-like) |
| **AI Agent Autonomy** | ❌ Requires C++ developer | ✅ Fully autonomous |
| **Hot-Reload Support** | ❌ C++ recompilation breaks | ✅ Handlers re-register on reload |

### Example Use Case: AI Agent Creating Custom Command

```javascript
// AI Planner Agent: "I need a command to spawn waves of enemies"
// WITHOUT GenericCommand: Ask human to write C++, wait hours/days
// WITH GenericCommand: Define and use immediately

// Step 1: Register handler (in C++ or via script binding)
GenericCommandExecutor.RegisterHandler("SpawnWave", (payload) => {
  const { count, enemyType, formation } = payload;
  for (let i = 0; i < count; i++) {
    const pos = CalculateFormationPos(i, formation);
    EntitySystem.CreateEntity(enemyType, pos);
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
│  Ring buffer with V8::Persistent<V8::Object> payloads         │
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

## Alignment with Product Vision

This feature directly supports DaemonAgent's core vision as outlined in the project documentation:

- **Dual-Language Architecture**: Strengthens JavaScript's role as the "Agent API Surface" by enabling runtime command extensibility
- **AI-Driven Development**: Empowers AI agents to autonomously extend game functionality through custom commands
- **Hot-Reload Support**: Aligns with existing hot-reload philosophy—no C++ recompilation for agent-driven features
- **Multi-Agent Collaboration**: Provides shared command vocabulary for KĀDI protocol agent communication
- **Research Project Goals**: Demonstrates cutting-edge runtime extensibility for game engine architectures

## Requirements

### Requirement 1: Generic Command Structure

**User Story:** As an AI agent developer, I want to submit commands with dynamic payloads to the C++ engine, so that I can prototype new game mechanics without C++ code changes.

#### Acceptance Criteria

1. WHEN an AI agent creates a command with type "SpawnDynamicEntity" and payload `{position: {x: 10, y: 0, z: 5}, prefabName: "Enemy"}` THEN the system SHALL store the command with V8 object payload intact
2. WHEN the command queue capacity is reached THEN the system SHALL reject new submissions and return false
3. WHEN a command is submitted with agentId "calculator-agent" THEN the system SHALL record the agentId for audit tracking
4. IF the V8 isolate is locked by another thread THEN the system SHALL safely handle the lock contention without corruption
5. WHEN a command's V8 object payload is accessed THEN the system SHALL use proper V8 handle scopes to prevent memory leaks

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
2. WHEN calling `CommandQueue.registerSchema("UpdateCamera", {position: isVector3, rotation: isEulerAngles})` THEN the system SHALL validate subsequent commands against the schema
3. IF a command payload fails schema validation THEN the system SHALL throw a descriptive JavaScript error without submitting to C++
4. WHEN JavaScript queries `CommandQueue.getRegisteredTypes()` THEN the system SHALL return an array of all registered command types
5. WHEN the C++ command queue is full THEN `CommandQueue.submit()` SHALL return false and log a backpressure warning

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

1. WHEN an agent submits more than 100 commands per second THEN the system SHALL enforce rate limiting and reject excess commands
2. WHEN a command is submitted THEN the system SHALL log it to an audit trail with timestamp, agentId, command type, and success/failure status
3. IF an agent submits 10 consecutive invalid commands THEN the system SHALL log a warning about potential agent malfunction
4. WHEN querying `GenericCommandQueue.GetStatistics()` THEN the system SHALL return total submitted, executed, failed, and dropped command counts per agent
5. WHEN the dashboard is opened THEN it SHALL display real-time command submission rates, queue depth, and top command types

### Requirement 6: Performance Characteristics

**User Story:** As an engine performance engineer, I want GenericCommand to maintain acceptable latency, so that AI agent commands do not degrade frame rate.

#### Acceptance Criteria

1. WHEN submitting a GenericCommand THEN the submission latency SHALL be less than 10 microseconds (excluding V8 overhead)
2. WHEN executing 100 GenericCommands per frame THEN the frame time overhead SHALL be less than 1 millisecond
3. IF the command queue reaches 80% capacity THEN the system SHALL log a performance warning
4. WHEN V8 object payload size exceeds 1 KB THEN the system SHALL log a warning about potential performance impact
5. WHEN GenericCommand execution is profiled THEN handler lookup latency SHALL be less than 0.1 microseconds per command

### Requirement 7: Migration Path from Typed Commands

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
  - `GenericCommand.hpp` - Command structure definition
  - `GenericCommandQueue.hpp` - Queue implementation using CommandQueueBase template
  - `GenericCommandExecutor.hpp` - Handler registry and execution logic
  - `GenericCommandScriptInterface.hpp` - JavaScript V8 bindings
  - `CommandQueue.js` - JavaScript API facade
- **Dependency Management**:
  - GenericCommand depends on V8 API only
  - GenericCommandQueue inherits from CommandQueueBase (reuses existing SPSC lock-free implementation)
  - GenericCommandExecutor has no dependencies on specific game systems
- **Clear Interfaces**:
  - C++ handler registry uses `std::function<void(v8::Local<v8::Object>)>` for handler signatures
  - JavaScript API uses promises for async command submission
  - V8 integration uses standard v8::Persistent and v8::Local handle patterns

### Performance

- **Submission Latency**: < 10µs per command (excluding V8 object copy overhead)
- **Execution Throughput**: 1000+ commands per frame without frame rate impact (target: 60 FPS)
- **Memory Overhead**:
  - GenericCommand size: ~320 bytes (v8::Persistent + metadata)
  - Queue capacity: 500 commands (160 KB total memory)
  - Handler registry: O(1) hash map lookup
- **V8 Integration**: Minimize isolate lock contention through batched command execution

### Security

- **Rate Limiting**: 100 commands/second per agent (configurable)
- **Input Validation**: JavaScript-side schema validation before C++ submission
- **Audit Trail**: All commands logged with agentId, timestamp, type, and execution result
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
  - Dashboard visualization of command flow and statistics
- **Documentation**:
  - JSDoc comments for all JavaScript APIs
  - C++ Doxygen comments for engine integration
  - Migration guide from typed to generic commands
- **Hot-Reload Compatibility**: Command handlers can be re-registered after JavaScript file reload
