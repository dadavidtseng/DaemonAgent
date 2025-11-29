# ProtogameJS3D: Dual-Language Game Engine Architecture
## PowerPoint Presentation Guide (10 Slides)

**Presentation Duration**: 10-15 minutes
**Audience**: Thesis Professor
**Date**: November 5, 2025
**Based on**: cpp-js-architecture.md (Comprehensive Technical Analysis)

---

## Slide 1: Title Slide

### Visual Content
```
Title: ProtogameJS3D
Subtitle: Dual-Language Game Engine Architecture
         C++ Performance + JavaScript Flexibility

Author: [Your Name]
Date: November 5, 2025
Thesis Advisor: [Professor Name]
```

### Presenter Notes (150-200 words)

Good morning, Professor. Today I'll present ProtogameJS3D, a research project demonstrating dual-language game engine architecture. This system addresses a fundamental challenge in modern game development: how to balance performance-critical operations with rapid iteration requirements.

Traditional game engines face a dilemma—pure C++ provides excellent performance but slow iteration times, while pure scripting languages enable fast prototyping but sacrifice performance. ProtogameJS3D solves this by integrating Google V8 JavaScript Engine with a high-performance C++ rendering foundation called DaemonEngine.

The key innovation is our fault-tolerant async execution model, where C++ maintains stable 60 FPS rendering on the main thread while JavaScript executes game logic asynchronously on a worker thread. This architecture ensures that even if JavaScript code runs slowly or crashes, the rendering pipeline continues operating smoothly.

This presentation will cover six main topics: our dual-thread architecture, async execution flow, thread synchronization mechanisms, state management patterns, the script interface layer, and performance characteristics. I'll demonstrate how this architecture achieves both high performance and rapid development iteration.

---

## Slide 2: Architecture Overview

### Visual Content
```
[Reference: System Architecture Diagram from Section 1.1]

Main Components:
• Main Thread (C++) - 60 FPS Rendering
• Worker Thread (JavaScript) - Game Logic
• Communication Layer - Lock-Free Queues
• V8 Integration - Script Runtime
• Hot-Reload System - Live Updates

Key Metrics:
• Main Thread: 16.67ms ± 2ms (Stable)
• JavaScript: 0-30ms (Variable, Non-blocking)
• Buffer Swap: <1ms
```

### Presenter Notes (180-200 words)

This diagram shows the complete system architecture. The design follows a strict separation of concerns across two threads.

On the main thread, we have the C++ game loop in App.cpp, which maintains a stable 60 FPS rendering rate regardless of JavaScript execution time. This thread handles DirectX rendering, processes render commands, and reads state from front buffers. The rendering pipeline never blocks waiting for JavaScript.

On the worker thread, we run a V8 JavaScript isolate managed by JSGameLogicJob. This executes all game logic asynchronously—JSEngine.js coordinates game systems, which write state to back buffers and submit rendering commands to a lock-free queue.

The communication layer connects these threads using three mechanisms: EntityStateBuffer and CameraStateBuffer for state management, and RenderCommandQueue for graphics commands. All three use lock-free or minimally-locked patterns to avoid thread contention.

The V8 integration layer includes the ScriptSubsystem for runtime management, a hot-reload system with file watching, and Chrome DevTools integration for debugging. This architecture achieves fault tolerance—JavaScript errors are isolated and don't crash the C++ rendering pipeline. Performance metrics show main thread stability at 16.67ms per frame while JavaScript execution varies from 0-30ms without affecting rendering.

---

## Slide 3: Dual-Language Integration Model

### Visual Content
```
C++ Domain (Performance-Critical):
✓ DirectX Rendering Pipeline
✓ Physics Calculations
✓ Memory Management
✓ File I/O and Resource Loading

JavaScript Domain (Rapid Iteration):
✓ Game Logic and AI
✓ Entity Behaviors
✓ UI Systems
✓ Gameplay Scripts

Bridge Layer:
→ Type-Safe Interfaces
→ Async Callbacks
→ State Synchronization
```

### Presenter Notes (170-190 words)

The dual-language model follows a clear separation of responsibilities based on performance requirements and iteration speed needs.

The C++ domain handles all performance-critical systems. DirectX rendering operations require tight control over GPU resources and must complete within strict frame budgets. Physics calculations benefit from C++ optimization and SIMD instructions. Memory management uses RAII patterns for deterministic cleanup, which JavaScript's garbage collector can't provide. File I/O operations are synchronous and need direct OS API access.

The JavaScript domain focuses on game logic where iteration speed matters more than raw performance. Entity behaviors can be modified and hot-reloaded without recompiling C++. AI systems use JavaScript's flexibility for complex state machines. UI systems benefit from rapid prototyping and designer-friendly scripting.

The bridge layer provides type-safe interfaces between these domains. We've implemented EntityAPI and CameraAPI as C++ classes with JavaScript bindings. The async callback system allows C++ to trigger JavaScript functions without blocking. State synchronization uses double-buffered containers, which I'll explain in detail shortly. This architecture achieves the best of both worlds—C++ performance where needed, JavaScript flexibility for game logic.

---

## Slide 4: Async Execution Flow (M4-T8 Achievement)

### Visual Content
```
[Reference: Execution Flow Sequence Diagram from Section 2.2]

Frame Timeline (16.67ms):
0ms:     Main Thread starts Frame N
         Worker Thread triggered

0-8ms:   Main Thread renders (Front Buffer)
         Worker Thread executes JavaScript (Back Buffer)

8ms:     Worker signals completion

16.67ms: Main Thread starts Frame N+1
         Swap buffers (brief lock ~1ms)
         Process render commands

Key Benefit: JavaScript can take >16ms without dropping frames!
```

### Presenter Notes (190-210 words)

This is one of our most significant achievements, completed in Milestone 4 Task 8, where we refactored the architecture for true async execution.

Let me walk through a typical frame. At time zero, the main thread begins Frame N and immediately triggers the worker thread to start JavaScript execution. The main thread then reads from the front buffer—this contains entity positions and camera state from the previous frame—and proceeds with rendering.

Simultaneously, the worker thread executes JSEngine.update(), which runs all game systems. These systems write updated state to the back buffer and submit rendering commands to a queue. In this example, JavaScript completes in 8 milliseconds.

At 16.67 milliseconds, the main thread begins Frame N+1. It checks if the worker thread has signaled completion. If yes, we swap buffers with a brief mutex lock—typically under 1 millisecond—and process the queued render commands. If JavaScript hasn't finished, that's okay—we skip the swap and continue rendering with the previous frame's state.

This frame skip tolerance is crucial. If JavaScript takes 30 milliseconds due to complex AI calculations, the main thread simply renders two frames with the same state. Users see smooth 60 FPS rendering even during JavaScript computation spikes. This is the core innovation that makes dual-language architecture practical for real-time games.

---

## Slide 5: Thread Synchronization Mechanisms

### Visual Content
```
[Reference: Synchronization Primitives Diagram from Section 4.1]

Lock-Free Mechanisms:
✓ Double-Buffering (EntityStateBuffer)
✓ Double-Buffering (CameraStateBuffer)
✓ Command Queue (Lock-Free MPSC)

Minimal Locking:
• Buffer Swap (~1ms with std::mutex)
• Callback Storage (std::mutex)
• Hot-Reload Events (std::mutex)

Thread Coordination:
• Frame Trigger (std::condition_variable)
• Frame Complete (std::atomic<bool>)
```

### Presenter Notes (180-200 words)

Thread synchronization is critical for our async architecture's performance and correctness.

We use lock-free mechanisms wherever possible. The double-buffering pattern for EntityStateBuffer and CameraStateBuffer allows simultaneous read and write operations without locks. The main thread reads the front buffer while the worker thread writes to the back buffer. Only the buffer swap operation requires a brief mutex lock, typically under 1 millisecond.

RenderCommandQueue implements a lock-free Multi-Producer Single-Consumer pattern. JavaScript systems submit rendering commands without blocking, and the main thread processes them in batch during the update phase.

We do use locks for three specific operations. Buffer swapping requires a std::mutex to safely exchange front and back pointers—this is brief and happens once per frame. Callback storage uses a mutex because C++ needs to modify the callback list while JavaScript might be executing callbacks. Hot-reload events use a mutex since file watching happens on a separate thread.

For thread coordination, we use a condition variable to trigger frame execution—the main thread signals the worker thread to start. Frame completion uses an atomic boolean for wait-free checking—the main thread can query completion status without blocking.

This combination of lock-free and minimally-locked mechanisms achieves high performance while maintaining thread safety guarantees.

---

## Slide 6: State Management & Double Buffering

### Visual Content
```
StateBuffer<T> Template Pattern:

[Front Buffer] ← Main Thread (Read Only)
     ↕ Swap
[Back Buffer]  ← Worker Thread (Write Only)

Instantiations:
• EntityStateBuffer - Position, Rotation, Scale
• CameraStateBuffer - View Matrix, Projection

Swap Operation (App.cpp):
1. Lock mutex
2. std::swap(frontPtr, backPtr)
3. Unlock mutex
4. Duration: <1ms
```

### Presenter Notes (170-190 words)

Double-buffering is the foundation of our thread-safe state management, implemented as a generic StateBuffer template in the Engine repository.

The pattern is elegantly simple. We maintain two complete copies of all game state—one front buffer and one back buffer. The main thread has read-only access to the front buffer for rendering. The worker thread has write-only access to the back buffer for game logic updates. These operations happen simultaneously without any locks.

We have two instantiations. EntityStateBuffer stores the position, rotation, and scale for every game entity. CameraStateBuffer stores the view matrix, projection matrix, and camera parameters. Both follow the same double-buffer pattern.

Once per frame, if JavaScript has finished executing, we perform a swap operation in App.cpp. This acquires a mutex, calls std::swap to exchange the front and back pointers, and releases the mutex. The swap itself is just a pointer exchange—extremely fast, typically well under 1 millisecond even with thousands of entities.

This pattern was moved to the Engine repository during M4-T8 refactoring, improving code reusability across projects. The key trade-off is memory—we need 2× state storage—but we gain complete thread safety without runtime locks during normal operation.

---

## Slide 7: Script Interface Layer

### Visual Content
```
IScriptableObject Pattern:

C++ Side (EntityAPI.cpp):
class EntityAPI : public IScriptableObject {
    void ExposeToScript(ScriptSubsystem*) override;
    // Binds C++ functions to JavaScript names
};

JavaScript Side (EntityAPI.js):
const entity = EntityAPI.create();
entity.setPosition(x, y, z);
const pos = entity.getPosition();

Type Conversion:
C++ Vec3 ↔ JavaScript {x, y, z}
C++ String ↔ JavaScript string
C++ Callbacks ↔ JavaScript Functions
```

### Presenter Notes (185-205 words)

The script interface layer provides type-safe communication between C++ and JavaScript, implemented through the IScriptableObject pattern.

On the C++ side, EntityAPI and CameraAPI inherit from IScriptableObject. Each implements ExposeToScript, which registers C++ member functions with the V8 runtime. For example, EntityAPI exposes create, destroy, setPosition, getPosition, and other entity manipulation functions. The V8 binding system handles marshaling between C++ and JavaScript calling conventions.

On the JavaScript side, these appear as native JavaScript objects. EntityAPI.js wraps the low-level bindings with a cleaner API. Game code can call entity.setPosition(x, y, z) naturally, and this triggers the C++ implementation through V8 function templates.

Type conversion is handled automatically by our binding layer. C++ Vec3 structures convert to JavaScript objects with x, y, z properties. C++ std::string maps to JavaScript string. Most interesting is callback support—C++ can store JavaScript function references and invoke them later, enabling event-driven communication.

This pattern was also refactored in M4-T8. Previously, we had a HighLevelEntityAPI facade that violated SOLID principles. Now App.cpp directly uses EntityAPI and CameraAPI from the Engine repository, following the Dependency Inversion Principle. This improves both code clarity and reusability.

---

## Slide 8: Performance Analysis

### Visual Content
```
Frame Timing Breakdown (60 FPS = 16.67ms):

Main Thread Budget:
• BeginFrame:     0.2ms  (1.2%)
• Update:         2.5ms  (15%)
  ├ SwapBuffers:  1.1ms  (7%)
  └ ProcessCmds:  1.0ms  (6%)
• Render:         8.0ms  (48%)
• EndFrame:       1.5ms  (9%)
• Total:         12.2ms  (73%)
• Slack:          4.5ms  (27%)

JavaScript Thread (Variable):
• Best Case:      3ms
• Typical:       8-15ms
• Worst Case:    30ms+
→ No impact on main thread!
```

### Presenter Notes (175-195 words)

Let me show you the actual performance characteristics measured from our implementation.

The main thread operates with a strict 16.67-millisecond budget for 60 FPS. BeginFrame takes 0.2 milliseconds for initialization. Update phase is 2.5 milliseconds—this includes swapping buffers at 1.1 milliseconds and processing render commands at 1 millisecond. Rendering is our most expensive operation at 8 milliseconds, dominated by DirectX draw calls for entities. EndFrame takes 1.5 milliseconds for presentation and buffer swaps.

Total main thread time is 12.2 milliseconds, which is 73% of our budget. We have 4.5 milliseconds of slack—this is important for handling frame spikes.

JavaScript execution on the worker thread is highly variable. Best case is 3 milliseconds for simple game logic. Typical frame with entity updates, camera movement, and input processing takes 8-15 milliseconds. Worst case, during complex AI decisions or pathfinding, can exceed 30 milliseconds.

The critical point is that JavaScript timing has zero impact on main thread frame rate. When JavaScript takes 30 milliseconds, the main thread simply renders two frames with the same game state. Users see smooth 60 FPS animation even during computation spikes. This is the practical benefit of our async architecture.

---

## Slide 9: Hot-Reload System & Development Workflow

### Visual Content
```
Hot-Reload Architecture:

File Watcher Thread:
1. Monitors Run/Data/Scripts/*.js
2. Detects file changes
3. Signals ScriptSubsystem

Main Thread Processing:
4. Pauses worker thread
5. V8 re-compiles changed scripts
6. Resumes worker thread

Latency: 10-50ms (mostly V8 compilation)

Developer Benefit:
Edit → Save → Instant Update
No C++ recompilation needed!
```

### Presenter Notes (165-185 words)

The hot-reload system dramatically improves development iteration speed and was a key motivation for choosing JavaScript.

A dedicated file watcher thread monitors all JavaScript files in Run/Data/Scripts. When you save changes to any .js file, the watcher detects the modification and signals the ScriptSubsystem on the main thread.

During the next frame, the main thread briefly pauses the worker thread to ensure no JavaScript code is executing. V8 then recompiles the modified scripts. Once compilation completes—typically 10-50 milliseconds depending on file size—the worker thread resumes with the new code. The next frame executes the updated logic.

This enables immediate feedback during development. You can edit entity behaviors in Player.js, save the file, and instantly see the changes in the running game. No waiting for C++ recompilation, no relaunching the application. This is particularly valuable during gameplay tuning—designers can tweak parameters and immediately test them.

We also integrated Chrome DevTools protocol support, providing full JavaScript debugging capability with breakpoints, stepping, and variable inspection. This combination of hot-reload and professional debugging tools creates a development experience comparable to web development while maintaining game engine performance.

---

## Slide 10: Conclusion & Research Contributions

### Visual Content
```
Architecture Strengths:
✓ Fault Tolerance - JS errors isolated from C++
✓ Performance - Stable 60 FPS async execution
✓ Developer Experience - Hot-reload iteration
✓ Thread Safety - Lock-free + double-buffering
✓ Modularity - Clean engine/game separation

Research Contributions:
1. Async dual-thread execution model for V8
2. Lock-free state synchronization patterns
3. Frame skip tolerance mechanism
4. Hot-reload with zero downtime

Future Work:
• TypeScript integration for type safety
• Multi-threaded JavaScript workers
• Network synchronization layer
```

### Presenter Notes (190-210 words)

Let me summarize the key strengths and research contributions of this architecture.

We achieved fault tolerance through complete isolation—JavaScript errors are caught by V8 and logged without crashing the C++ rendering pipeline. I can demonstrate this by intentionally throwing errors in game scripts while the engine continues running smoothly.

Performance is proven—we maintain stable 60 FPS with async JavaScript execution that varies from 3 to 30+ milliseconds. The main thread never blocks waiting for JavaScript. Developer experience is exceptional with hot-reload enabling sub-second iteration cycles and Chrome DevTools providing professional debugging.

Thread safety is guaranteed through our combination of lock-free mechanisms and minimal locking. Double-buffering eliminates locks during normal operation. Modularity is achieved by separating the Engine repository (reusable C++ systems) from the Game repository (project-specific logic).

Our research contributions include: First, the async dual-thread execution model showing that V8 can be safely integrated with game engines without compromising frame rate. Second, our lock-free state synchronization patterns using double-buffering and command queues. Third, the frame skip tolerance mechanism that gracefully handles JavaScript slowdowns. Fourth, hot-reload implementation with zero application downtime.

Future work includes TypeScript integration for compile-time type safety, exploring multi-threaded JavaScript workers for parallelism, and adding network synchronization for multiplayer games. Thank you for your attention. I'm happy to answer any questions about the architecture.

---

## Presentation Tips

### Timing Guide (10-15 minutes total)
- **Slide 1**: 1 minute (Introduction)
- **Slide 2**: 1.5 minutes (Architecture overview)
- **Slide 3**: 1.5 minutes (Dual-language model)
- **Slide 4**: 2 minutes (Async flow - key achievement)
- **Slide 5**: 1.5 minutes (Synchronization)
- **Slide 6**: 1.5 minutes (State management)
- **Slide 7**: 1.5 minutes (Script interface)
- **Slide 8**: 1.5 minutes (Performance)
- **Slide 9**: 1.5 minutes (Hot-reload)
- **Slide 10**: 1.5-2 minutes (Conclusion)
- **Q&A Buffer**: 3-5 minutes

### Key Points to Emphasize
1. **M4-T8 Achievement**: Highlight async architecture refactoring
2. **Performance Numbers**: Concrete metrics validate design
3. **Practical Benefits**: Hot-reload and fault tolerance
4. **Research Novelty**: V8 integration patterns for game engines

### Diagram References
- Include diagram references from cpp-js-architecture.md
- Have backup slides with detailed code examples if professor asks
- Prepare to explain any mermaid diagram in detail

### Potential Professor Questions
1. "Why not use existing engines like Unity/Unreal?"
   - Answer: Research project exploring dual-language patterns

2. "What's the memory overhead of double-buffering?"
   - Answer: 2× state storage (~5MB for 10K entities)

3. "How do you handle JavaScript errors?"
   - Answer: V8 TryCatch blocks, error logging, graceful degradation

4. "Can this scale to AAA game complexity?"
   - Answer: Currently tested to 10K entities; scaling requires optimizations discussed in Section 8

### Backup Content
Have ready if time permits or professor requests:
- Detailed code walkthrough of App.cpp Update() loop
- EntityStateBuffer implementation details
- V8 binding example with actual code
- Performance profiling methodology

---

**Document Version**: 1.0
**Created**: November 3, 2025
**Based on**: cpp-js-architecture.md (27,919 tokens)
**Presentation Target**: M5-T10 (Due November 5, 2025)
**Status**: Ready for PowerPoint creation
