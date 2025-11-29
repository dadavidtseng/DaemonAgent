# Requirements: C++/JavaScript Decoupling Architecture (Phase 1-4 Completion)

## Spec Metadata
- **Spec Name**: cpp-js-decoupling
- **Created**: 2025-01-18
- **Priority**: HIGH
- **Type**: Architecture Implementation / Refactoring
- **Estimated Hours**: 24-32h (across Phase 2-4)
- **Dependencies**: Phase 1 already complete (M4-T8 async architecture)

## User Story

**As a** game engine developer,
**I want to** complete the Phase 1-4 roadmap for C++/JavaScript decoupling,
**So that** C++ and JavaScript can run at independent speeds in their own game loops with proper async communication.

## Background and Architecture Goals

### Current State (Phase 1 - Complete ✅)
- Async infrastructure in place (EntityStateBuffer, CameraStateBuffer, RenderCommandQueue)
- JavaScript → C++ communication uses lock-free SPSC queue
- Double-buffering prevents corruption when entity counts change
- M4-T8 refactoring: Entity/Camera systems moved to Engine repository

### Problems with Current Implementation
1. **JavaScript executes on main thread** via `UpdateJS()`, not worker thread via `UpdateJSWorkerThread()`
2. **C++ → JavaScript callbacks are synchronous**, blocking C++ thread (no JavaScript callback queue)
3. **Full O(n) copy every frame** in `SwapBuffers()` regardless of dirty entities
4. **vertexBufferHandle pollutes EntityState** - C++ render resource in shared state structure
5. **No error recovery** in SwapBuffers() or callback execution
6. **No formal phase documentation** - phases exist only as scattered code comments

### Architecture Goal: Three Core Independence Principles

Achieve true decoupling where:

1. **No Waiting (Lock-Free Communication)**
   - C++ and JavaScript don't wait for each other when accessing queues
   - Lock-free SPSC queues guarantee non-blocking operations
   - Both threads continue execution regardless of the other's state

2. **Different Speeds (Independent Execution)**
   - **C++ rendering thread** runs at stable 60+ FPS
   - **JavaScript worker thread** runs at variable game logic speed (30-60 FPS)
   - C++ maintains frame rate even if JavaScript is slow (50ms+)

3. **Crash Isolation (Fault Tolerance)**
   - C++ continues rendering last good frame if JavaScript crashes
   - JavaScript errors don't crash C++ rendering
   - Double-buffering enables graceful degradation

**Communication Architecture**:
- **Async bidirectional**: JS→C++ via RenderCommandQueue, C++→JS via CallbackQueue
- **Optimized synchronization**: Dirty tracking instead of full O(n) copy
- **Robust error handling**: Try-catch, validation, timeout detection

## Requirements

### FR-1: Phase 2 - Worker Thread JavaScript Execution
**Given** JavaScript currently executes on main thread,
**When** implementing Phase 2,
**Then** JavaScript shall execute on dedicated worker thread via `UpdateJSWorkerThread()`.

**Acceptance Criteria**:
- Create `JavaScriptWorkerThread` class managing V8 isolate on worker thread
- Replace `Game::UpdateJS()` call with `Game::UpdateJSWorkerThread()` invocation
- Pass `EntityStateBuffer::GetFrontBuffer()` read-only access to worker thread
- Pass `RenderCommandQueue` write access to worker thread for JS→C++ commands
- Worker thread runs independent game loop (can be 30 FPS while C++ renders at 60 FPS)
- Main thread never blocks on JavaScript execution
- JavaScript execution time doesn't affect C++ frame rate

**Technical Details**:
```cpp
// Current (Phase 1) - Main thread execution
void Game::Update() {
    UpdateJS();  // ← Blocks main thread
    ProcessRenderCommands();
}

// Target (Phase 2) - Worker thread execution
void Game::Update() {
    // JavaScript runs independently on worker thread
    ProcessRenderCommands();  // ← Never blocks on JS
}

void Game::UpdateJSWorkerThread() {
    // Runs on worker thread
    auto* frontBuffer = m_entityStateBuffer->GetFrontBuffer();  // Read-only
    float deltaTime = ...;
    ExecuteJavaScriptCommand(...);
}
```

**Files to Modify**:
- `C:\p4\Personal\SD\ProtogameJS3D\Code\Game\Gameplay\Game.cpp` - Connect UpdateJSWorkerThread()
- `C:\p4\Personal\SD\ProtogameJS3D\Code\Game\Gameplay\Game.hpp` - Add JavaScriptWorkerThread member
- `C:\p4\Personal\SD\ProtogameJS3D\Code\Game\Framework\App.cpp` - Remove UpdateJS() call
- `C:\p4\Personal\SD\Engine\Code\Engine\Core\V8Subsystem.cpp` - Thread-safe V8 isolate management

### FR-2: JavaScript Callback Queue (Async C++→JS Communication)
**Given** C++ callbacks currently execute synchronously and block C++ thread,
**When** implementing async callback mechanism,
**Then** C++ shall enqueue callbacks into JavaScript's CallbackQueue for async processing.

**Acceptance Criteria**:
- Create `CallbackQueue` class using lock-free SPSC queue (similar to RenderCommandQueue)
- C++ enqueues callbacks via `EnqueueCallback(callbackId, resultData)`
- JavaScript dequeues and executes callbacks during its update loop
- C++ never blocks waiting for callback execution
- Callbacks execute on JavaScript worker thread, not main thread
- Support for multiple callback types: entity creation, camera creation, resource loading
- Error in callback doesn't crash C++ thread

**Technical Details**:
```cpp
// Current (Phase 1) - Synchronous blocking
void EntityAPI::ExecutePendingCallbacks() {
    for (auto& [callbackId, pending] : m_pendingCallbacks) {
        if (pending.ready) {
            ExecuteCallback(callbackId, pending.resultId);  // ← Blocks C++ thread
        }
    }
}

// Target (Phase 2) - Async queue
void EntityAPI::ExecutePendingCallbacks() {
    for (auto& [callbackId, pending] : m_pendingCallbacks) {
        if (pending.ready) {
            CallbackData data{callbackId, pending.resultId, pending.errorMsg};
            m_callbackQueue->Enqueue(data);  // ← Never blocks
        }
    }
}

// JavaScript side
JSEngine.update = function(deltaTime) {
    // Process callbacks from C++
    while (let callback = CallbackQueue.dequeue()) {
        executeCallback(callback.callbackId, callback.resultId);
    }

    // Normal game logic
    // ...
};
```

**Files to Create**:
- `C:\p4\Personal\SD\Engine\Code\Engine\Core\CallbackQueue.hpp` - Lock-free SPSC queue for callbacks
- `C:\p4\Personal\SD\Engine\Code\Engine\Core\CallbackQueue.cpp` - Implementation

**Files to Modify**:
- `C:\p4\Personal\SD\Engine\Code\Engine\Entity\EntityAPI.cpp` - Use CallbackQueue instead of direct execution
- `C:\p4\Personal\SD\Engine\Code\Engine\Renderer\CameraAPI.cpp` - Use CallbackQueue instead of direct execution
- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\interfaces\EntityAPI.js` - Dequeue and process callbacks
- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\interfaces\CameraAPI.js` - Dequeue and process callbacks
- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\JSEngine.js` - Add callback processing in update loop

### FR-3: Phase 3 - Error Recovery and Exception Handling
**Given** current implementation has no error recovery,
**When** implementing Phase 3,
**Then** errors in one thread shall not crash the other thread.

**Acceptance Criteria**:
- Implement `Game::HandleJSException()` to catch and log JavaScript errors
- Wrap `SwapBuffers()` with exception handling and validation
- Add `ValidateStateBuffer()` to detect corruption before swap
- JavaScript errors logged but don't crash C++ thread
- C++ exceptions logged but don't corrupt JavaScript state
- Failed callbacks reported back to C++ with error messages
- Add mutex timeout detection to prevent deadlocks

**Technical Details**:
```cpp
// Phase 3 - Error recovery
void StateBuffer::SwapBuffers() {
    try {
        std::lock_guard lock(m_swapMutex);

        // Validate before swap
        if (!ValidateStateBuffer(m_backBuffer)) {
            LogError("Back buffer validation failed - skipping swap");
            return;
        }

        *m_frontBuffer = *m_backBuffer;
        std::swap(m_frontBuffer, m_backBuffer);
        ++m_totalSwaps;
    }
    catch (std::exception const& e) {
        LogError("SwapBuffers failed: %s", e.what());
        // Don't propagate - renderer can use stale front buffer
    }
}

void Game::HandleJSException(v8::TryCatch& tryCatch) {
    v8::String::Utf8Value exception(isolate, tryCatch.Exception());
    v8::Local<v8::Message> message = tryCatch.Message();

    // Log detailed error info
    LogError("JavaScript Exception: %s", *exception);
    LogError("  at %s:%d", *filename, linenum);

    // Don't crash - continue game loop
}
```

**Files to Modify**:
- `C:\p4\Personal\SD\Engine\Code\Engine\Core\StateBuffer.hpp` - Add validation and error handling
- `C:\p4\Personal\SD\ProtogameJS3D\Code\Game\Gameplay\Game.cpp` - Implement HandleJSException()
- `C:\p4\Personal\SD\Engine\Code\Engine\Core\V8Subsystem.cpp` - Wrap all V8 calls with try-catch
- `C:\p4\Personal\SD\Engine\Code\Engine\Core\CallbackQueue.cpp` - Add error reporting

### FR-4: Phase 4 - Dirty Tracking Optimization
**Given** current implementation copies all entities every frame (O(n)),
**When** implementing Phase 4 optimization,
**Then** only dirty entities shall be copied during SwapBuffers().

**Acceptance Criteria**:
- Add `std::unordered_set<EntityID> m_dirtyEntities` to track modified entities
- `UpdateEntity()` operations mark entity as dirty
- `SwapBuffers()` only copies dirty entities from back to front buffer
- Reduce average swap time from O(n) to O(d) where d = dirty count
- Maintain correctness - no stale data in front buffer
- Add metrics: dirty entity count, swap time, copy count

**Technical Details**:
```cpp
// Current (Phase 1) - Full copy O(n)
void StateBuffer::SwapBuffers() {
    std::lock_guard lock(m_swapMutex);
    *m_frontBuffer = *m_backBuffer;  // Copies ALL entities
    std::swap(m_frontBuffer, m_backBuffer);
}

// Target (Phase 4) - Dirty tracking O(d)
void StateBuffer::SwapBuffers() {
    std::lock_guard lock(m_swapMutex);

    // Only copy dirty entities
    for (EntityID dirtyId : m_dirtyEntities) {
        auto it = m_backBuffer->find(dirtyId);
        if (it != m_backBuffer->end()) {
            (*m_frontBuffer)[dirtyId] = it->second;  // Copy single entity
        } else {
            m_frontBuffer->erase(dirtyId);  // Entity deleted
        }
    }

    m_dirtyEntities.clear();
    std::swap(m_frontBuffer, m_backBuffer);

    // Metrics
    m_avgDirtyCount.Add(m_dirtyEntities.size());
}

void StateBuffer::MarkDirty(EntityID id) {
    m_dirtyEntities.insert(id);
}
```

**Files to Modify**:
- `C:\p4\Personal\SD\Engine\Code\Engine\Core\StateBuffer.hpp` - Add dirty tracking data structures
- `C:\p4\Personal\SD\Engine\Code\Engine\Core\StateBuffer.cpp` - Implement dirty-only swap
- `C:\p4\Personal\SD\ProtogameJS3D\Code\Game\Framework\App.cpp` - Mark entities dirty in ProcessRenderCommands()

### FR-5: Remove vertexBufferHandle from EntityState
**Given** vertexBufferHandle is C++ render resource in shared state,
**When** refactoring entity state,
**Then** render resources shall be separated from game state.

**Acceptance Criteria**:
- Create `RenderResourceManager` to store vertexBufferHandle separately
- Map EntityID → vertexBufferHandle in C++ only
- Remove `vertexBufferHandle` field from `EntityState` struct
- JavaScript never sees or accesses render resources
- `EntityState` contains only game-relevant data: position, orientation, color, radius, meshType

**Technical Details**:
```cpp
// Current - Mixed concerns
struct EntityState {
    Vec3 position;
    EulerAngles orientation;
    Rgba8 color;
    float radius;
    std::string meshType;
    bool isActive;
    int vertexBufferHandle;  // ❌ C++ render resource polluting shared state
    std::string cameraType;
};

// Target - Clean separation
struct EntityState {
    Vec3 position;           // ✅ Game state
    EulerAngles orientation; // ✅ Game state
    Rgba8 color;             // ✅ Game state
    float radius;            // ✅ Game state
    std::string meshType;    // ✅ Configuration
    bool isActive;           // ✅ Game state
    std::string cameraType;  // ✅ Configuration
};

class RenderResourceManager {
    std::unordered_map<EntityID, int> m_vertexBuffers;  // C++ only
    std::unordered_map<EntityID, int> m_textures;       // C++ only
public:
    void AssignVertexBuffer(EntityID id, int handle);
    int GetVertexBuffer(EntityID id) const;
};
```

**Files to Modify**:
- `C:\p4\Personal\SD\Engine\Code\Engine\Entity\EntityState.hpp` - Remove vertexBufferHandle
- `C:\p4\Personal\SD\ProtogameJS3D\Code\Game\Framework\App.hpp` - Add RenderResourceManager
- `C:\p4\Personal\SD\ProtogameJS3D\Code\Game\Framework\App.cpp` - Use RenderResourceManager in ProcessRenderCommands()

### FR-6: Formal Phase Documentation
**Given** phases exist only as scattered code comments,
**When** documenting the architecture,
**Then** create formal phase documentation explaining the roadmap.

**Acceptance Criteria**:
- Create `Docs/architecture/AsyncArchitecturePhases.md` document
- Document Phase 1: Async Infrastructure (✅ Complete)
- Document Phase 2: Worker Thread Execution
- Document Phase 3: Error Recovery
- Document Phase 4: Performance Optimization
- Include diagrams showing thread relationships
- Include code examples for each phase
- Explain trade-offs and design decisions

**Files to Create**:
- `C:\p4\Personal\SD\ProtogameJS3D\Docs\architecture\AsyncArchitecturePhases.md` - Complete phase documentation

## Non-Functional Requirements

### NFR-1: Thread Safety
**Given** C++ and JavaScript run on separate threads,
**When** implementing decoupled architecture,
**Then** all shared data structures shall be thread-safe.

**Acceptance Criteria**:
- No data races detected by ThreadSanitizer
- Lock-free queues properly synchronized
- StateBuffer swap uses std::mutex
- Front buffer read-only for JavaScript thread
- Back buffer write-only for C++ thread

### NFR-2: Performance
**Given** the goal is independent thread speeds,
**When** measuring performance,
**Then** C++ rendering shall maintain 60+ FPS regardless of JavaScript performance.

**Acceptance Criteria**:
- C++ rendering: 60+ FPS
- JavaScript logic: 30-60 FPS (independent of C++)
- Queue operations: < 1µs per operation
- SwapBuffers with dirty tracking: < 100µs for 100 dirty entities
- No frame drops when JavaScript slows down

### NFR-3: Backward Compatibility
**Given** existing JavaScript game code,
**When** implementing Phase 2-4,
**Then** existing JavaScript APIs shall remain compatible.

**Acceptance Criteria**:
- EntityAPI.js methods work unchanged
- CameraAPI.js methods work unchanged
- Callbacks work identically from JavaScript perspective
- No breaking changes to game logic code

### NFR-4: Testability
**Given** complex multi-threaded architecture,
**When** implementing Phase 2-4,
**Then** components shall be testable in isolation.

**Acceptance Criteria**:
- CallbackQueue testable without V8
- StateBuffer testable without JavaScript
- RenderCommandQueue testable without rendering
- Mock JavaScript worker for C++ testing
- Mock C++ thread for JavaScript testing

## Success Criteria

### Phase 2 Success Criteria
- [ ] JavaScript executes on worker thread via `UpdateJSWorkerThread()`
- [ ] Main thread never calls `UpdateJS()`
- [ ] JavaScript worker thread has independent game loop
- [ ] C++ rendering maintains 60 FPS while JavaScript runs at 30 FPS
- [ ] CallbackQueue implemented and used for all C++→JS callbacks
- [ ] No synchronous callback execution blocking C++ thread

### Phase 3 Success Criteria
- [ ] `HandleJSException()` catches and logs JavaScript errors without crashing
- [ ] `SwapBuffers()` has exception handling and validation
- [ ] `ValidateStateBuffer()` detects corruption
- [ ] Failed callbacks report errors back to C++
- [ ] No deadlocks or mutex timeouts
- [ ] Error recovery tested with intentional failures

### Phase 4 Success Criteria
- [ ] Dirty tracking implemented in StateBuffer
- [ ] SwapBuffers() only copies dirty entities
- [ ] Metrics show average dirty count << total entity count
- [ ] SwapBuffers() time reduced by 50%+ for typical workloads
- [ ] Correctness maintained - no stale data bugs

### Phase 5 (vertexBufferHandle) Success Criteria
- [ ] `vertexBufferHandle` removed from EntityState
- [ ] RenderResourceManager created and used
- [ ] JavaScript never accesses render resources
- [ ] EntityState contains only game-relevant data
- [ ] All rendering code uses RenderResourceManager

### Documentation Success Criteria
- [ ] `AsyncArchitecturePhases.md` document created
- [ ] All 4 phases documented with code examples
- [ ] Thread relationship diagrams included
- [ ] Design trade-offs explained

## Out of Scope

- JavaScript multi-threading (JavaScript remains single-threaded)
- C++ multi-threaded rendering (rendering remains on main thread)
- Entity Component System (ECS) architecture
- Network synchronization
- Save/load system modifications
- GPU-side optimizations

## Dependencies

**Required Systems** (already exist):
- EntityStateBuffer, CameraStateBuffer (Phase 1 ✅)
- RenderCommandQueue (Phase 1 ✅)
- V8Subsystem with V8 isolate management (Phase 1 ✅)
- EntityAPI, CameraAPI (Phase 1 ✅)

**New Dependencies**:
- `std::thread` support for worker thread
- Lock-free queue library (already used for RenderCommandQueue)
- C++20 features for thread safety

## Risks and Mitigation

**Risk 1: V8 Thread Safety (HIGH)**
- **Description**: V8 isolates are not thread-safe by default
- **Probability**: HIGH
- **Mitigation**: Use V8 Locker API, one isolate per thread, no shared V8 objects
- **Contingency**: If Locker causes performance issues, use message passing instead of shared isolates

**Risk 2: Deadlock on Queues (MEDIUM)**
- **Description**: Circular dependencies between RenderCommandQueue and CallbackQueue could deadlock
- **Probability**: MEDIUM
- **Mitigation**: Careful lock ordering, timeout detection, deadlock testing
- **Contingency**: Add queue size limits and overflow handling

**Risk 3: Phase 4 Complexity (MEDIUM)**
- **Description**: Dirty tracking bugs could cause rendering artifacts
- **Probability**: MEDIUM
- **Mitigation**: Extensive testing, add debug mode with full copy for validation
- **Contingency**: Keep full-copy mode as fallback option

**Risk 4: Performance Regression (LOW)**
- **Description**: Overhead of worker thread and queues could slow down system
- **Probability**: LOW
- **Mitigation**: Profile at each phase, benchmark against Phase 1 baseline
- **Contingency**: Optimize queue operations, reduce lock contention

## Implementation Order

1. **Phase 2.1**: Implement CallbackQueue (independent, testable)
2. **Phase 2.2**: Integrate CallbackQueue into EntityAPI/CameraAPI
3. **Phase 2.3**: Create JavaScriptWorkerThread class
4. **Phase 2.4**: Connect UpdateJSWorkerThread() and test worker execution
5. **Phase 3.1**: Add SwapBuffers() error handling
6. **Phase 3.2**: Implement HandleJSException()
7. **Phase 3.3**: Add validation and recovery mechanisms
8. **Phase 4.1**: Implement dirty tracking in StateBuffer
9. **Phase 4.2**: Optimize SwapBuffers() to use dirty tracking
10. **Phase 4.3**: Add metrics and validation
11. **Phase 5**: Remove vertexBufferHandle and create RenderResourceManager
12. **Documentation**: Create AsyncArchitecturePhases.md

## Notes

- This spec completes the architectural vision started in M4-T8
- Phase 1 infrastructure is already complete and working
- Phases 2-4 build incrementally on Phase 1
- Each phase is independently testable
- Worker thread execution (Phase 2) is the most critical change
- Dirty tracking (Phase 4) provides the largest performance win
- Error recovery (Phase 3) ensures production robustness
