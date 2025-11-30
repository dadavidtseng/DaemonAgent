# Requirements: Unified Command Queue Architecture & SOLID Module Separation

## Spec Metadata
- **Spec Name**: command-queue-refactoring
- **Created**: 2025-01-30
- **Priority**: HIGH
- **Type**: Architecture Refactoring
- **Estimated Hours**: 32-48h
- **Dependencies**: None (builds on existing async infrastructure)

## Introduction

This specification defines a comprehensive refactoring of the project's async communication infrastructure to achieve three goals:

1. **Unified Queue Architecture**: Create a reusable `CommandQueueBase` template to eliminate code duplication across `RenderCommandQueue`, `CallbackQueue`, `AudioCommandQueue`, `ResourceCommandQueue`, and `DebugRenderCommandQueue`.

2. **Complete Async Coverage**: Add missing command queues for Audio and Resource modules to align with established async patterns used by Entity, Camera, and Render systems.

3. **SOLID Module Separation**: Complete the migration of resource loading responsibilities from Renderer to Resource module, ensuring each module has a single, well-defined purpose.

## Alignment with Product Vision

**Architecture Goal**: Achieve professional-grade, scalable async architecture that:
- Eliminates code duplication (DRY principle)
- Provides consistent async patterns across all subsystems
- Enables independent threading for C++, JavaScript, Audio, and Resource loading
- Maintains clear module boundaries following SOLID principles

This refactoring establishes ProtogameJS3D as a proven, reusable template for future game projects built on DaemonEngine.

## Requirements

### Requirement 1: Unified Command Queue Infrastructure

**User Story:** As an engine developer, I want a reusable command queue template, so that I can add new async subsystems without duplicating lock-free SPSC queue implementation code.

#### Acceptance Criteria

1. WHEN creating `CommandQueueBase<TCommand>` template THEN the system SHALL provide:
   - Lock-free SPSC ring buffer implementation
   - Cache-line aligned atomic indices (64-byte separation)
   - Configurable capacity via template parameter
   - `Submit(command)` producer API (non-blocking)
   - `ConsumeAll(processor)` consumer API (batch processing)
   - Virtual hooks for `OnSubmit()`, `OnConsume()`, `OnQueueFull()`

2. WHEN a new queue type is needed THEN the developer SHALL:
   - Define command structure (e.g., `AudioCommand`, `ResourceCommand`)
   - Create queue class inheriting from `CommandQueueBase<TCommand>`
   - Optionally override virtual hooks for custom behavior
   - Total implementation SHALL be <30 lines of code

3. WHEN the template is instantiated THEN the system SHALL:
   - Prevent code duplication (DRY principle)
   - Maintain identical performance to handwritten queues
   - Use memory ordering: acquire/release for synchronization, relaxed for statistics

### Requirement 2: Audio Command Queue Integration

**User Story:** As a game developer using JavaScript, I want async audio loading and playback, so that audio file I/O (10-200ms) doesn't block the JavaScript game loop.

#### Acceptance Criteria

1. WHEN JavaScript calls `audio.loadSound(path, callback)` THEN the system SHALL:
   - Submit `AudioCommand{LOAD_SOUND}` to `AudioCommandQueue` (non-blocking)
   - Return immediately to JavaScript (no blocking)
   - Process loading on Audio worker thread
   - Return result via `CallbackQueue` with `{ callbackId, soundId }`

2. WHEN JavaScript calls `audio.playSound(soundId)` THEN the system SHALL:
   - Submit `AudioCommand{PLAY_SOUND}` to `AudioCommandQueue` (non-blocking)
   - Process on Audio thread via FMOD
   - Execute without blocking JavaScript or C++ render thread

3. WHEN `AudioCommandQueue` processes commands THEN the system SHALL:
   - Support command types: `LOAD_SOUND`, `PLAY_SOUND`, `STOP_SOUND`, `SET_VOLUME`, `UPDATE_3D_POSITION`
   - Use default capacity of 200 commands (~8-12 KB memory)
   - Log queue full warnings (backpressure detection)

### Requirement 3: Resource Command Queue Integration

**User Story:** As a game developer using JavaScript, I want async resource loading (textures, models, shaders), so that file I/O doesn't block the JavaScript game loop and resources can load in the background.

#### Acceptance Criteria

1. WHEN JavaScript calls `resource.loadTexture(path, callback)` THEN the system SHALL:
   - Submit `ResourceCommand{LOAD_TEXTURE}` to `ResourceCommandQueue` (non-blocking)
   - Return immediately to JavaScript
   - Process loading on Resource worker thread pool
   - Return result via `CallbackQueue` with `{ callbackId, resourceId }`

2. WHEN Resource worker thread processes commands THEN the system SHALL:
   - Support command types: `LOAD_TEXTURE`, `LOAD_MODEL`, `LOAD_AUDIO`, `LOAD_SHADER`, `UNLOAD_RESOURCE`
   - Replace `std::async` with unified command queue pattern
   - Maintain existing worker thread pool architecture
   - Use default capacity of 200 commands (~280 bytes each = ~56 KB memory)

3. WHEN C++ code needs resource loading THEN the system SHALL:
   - Preserve existing synchronous API: `LoadResource<T>(path)` unchanged
   - Preserve existing async API: `LoadResourceAsync<T>(path)` returns `std::future`
   - Allow optional command queue injection via `SetCommandQueue(queue)`
   - Support compilation WITHOUT script module (`#if ENGINE_SCRIPTING_ENABLED`)

### Requirement 4: Debug Render Command Queue (Optional)

**User Story:** As a developer debugging game logic, I want debug rendering to be non-blocking, so that extensive debug visualization (100+ lines/spheres per frame) doesn't impact main rendering performance.

#### Acceptance Criteria

1. WHEN JavaScript calls `debug.addWorldLine(...)` THEN the system SHALL:
   - Submit `DebugRenderCommand{ADD_WORLD_LINE}` to `DebugRenderCommandQueue` (non-blocking)
   - Process on render thread during debug render pass

2. WHEN compiled in DEBUG mode THEN the system SHALL:
   - Include full `DebugRenderCommandQueue` implementation
   - Use capacity of 500 commands (~280 bytes each = ~140 KB)
   - Support all debug primitives: lines, spheres, arrows, text, billboards

3. WHEN compiled in RELEASE mode THEN the system SHALL:
   - Provide stub implementation (no-op)
   - Compile out all debug queue overhead (`#if defined(_DEBUG)`)

### Requirement 5: SOLID Module Separation - Renderer/Resource

**User Story:** As an engine architect, I want Renderer and Resource modules to follow Single Responsibility Principle, so that each module has a clear, focused purpose and can be maintained independently.

#### Acceptance Criteria

1. WHEN migrating Shader loading THEN the system SHALL:
   - Move `CreateOrGetShaderFromFile()` from Renderer to ResourceSubsystem
   - Move `GetShaderForFileName()` from Renderer to ResourceCache
   - Move `CreateShader()` logic to `ShaderLoader` class
   - Keep DirectX shader compilation in Renderer (render API responsibility)

2. WHEN migrating BitmapFont loading THEN the system SHALL:
   - Move `CreateOrGetBitmapFontFromFile()` from Renderer to ResourceSubsystem
   - Move `GetBitMapFontForFileName()` from Renderer to ResourceCache
   - Move font loading logic to `FontLoader` class

3. WHEN migration is complete THEN Renderer SHALL:
   - Have ZERO file I/O operations (no `fopen`, `std::ifstream`)
   - Have ZERO resource caching logic (no `std::vector<Texture*>`, etc.)
   - Focus exclusively on: DirectX API wrapping, rendering pipeline, shader binding, state management

4. WHEN migration is complete THEN ResourceSubsystem SHALL:
   - Own ALL file loading operations (textures, shaders, fonts, models, audio)
   - Own ALL resource caching and lifetime management
   - Provide unified API: `CreateOrGetResource<T>(path)` for all resource types

### Requirement 6: Visual Studio Filter Organization (IDE View Only)

**User Story:** As a developer navigating the codebase in Visual Studio, I want all async infrastructure grouped under a logical "Async" filter in Solution Explorer, so that I can quickly find related files without physically restructuring directories or updating include paths.

#### Acceptance Criteria

1. WHEN organizing async infrastructure THEN the system SHALL keep physical files in their module directories:
   ```
   Physical Directory Structure (UNCHANGED):
   Engine/Core/
   ├── CommandQueueBase.hpp          ← Template base (new)
   ├── CommandQueueBase.cpp          ← Common implementation (new)
   ├── CallbackQueue.hpp             ← Existing (unchanged location)
   └── CallbackQueue.cpp             ← Existing (unchanged location)

   Engine/Renderer/
   ├── RenderCommand.hpp             ← Existing (unchanged location)
   ├── RenderCommand.cpp             ← Existing (unchanged location)
   ├── RenderCommandQueue.hpp        ← Existing (unchanged location)
   ├── RenderCommandQueue.cpp        ← Existing (unchanged location)
   └── DebugRenderCommand.hpp        ← New (stays in Renderer)

   Engine/Audio/
   ├── AudioCommand.hpp              ← New (stays in Audio)
   └── AudioCommandQueue.hpp         ← New (stays in Audio)

   Engine/Resource/
   ├── ResourceCommand.hpp           ← New (stays in Resource)
   └── ResourceCommandQueue.hpp      ← New (stays in Resource)
   ```

2. WHEN configuring Visual Studio filters THEN the system SHALL create logical grouping in `.vcxproj.filters`:
   ```xml
   <ItemGroup>
     <Filter Include="Engine\Core\Async">
       <UniqueIdentifier>{GUID}</UniqueIdentifier>
     </Filter>
     <Filter Include="Engine\Core\Async\Base">
       <UniqueIdentifier>{GUID}</UniqueIdentifier>
     </Filter>
     <Filter Include="Engine\Core\Async\Queues">
       <UniqueIdentifier>{GUID}</UniqueIdentifier>
     </Filter>
     <Filter Include="Engine\Core\Async\Commands">
       <UniqueIdentifier>{GUID}</UniqueIdentifier>
     </Filter>
   </ItemGroup>

   <ItemGroup>
     <!-- Base Template -->
     <ClInclude Include="..\..\Engine\Code\Engine\Core\CommandQueueBase.hpp">
       <Filter>Engine\Core\Async\Base</Filter>
     </ClInclude>

     <!-- Commands (grouped logically, but stay in original directories) -->
     <ClInclude Include="..\..\Engine\Code\Engine\Renderer\RenderCommand.hpp">
       <Filter>Engine\Core\Async\Commands</Filter>
     </ClInclude>
     <ClInclude Include="..\..\Engine\Code\Engine\Audio\AudioCommand.hpp">
       <Filter>Engine\Core\Async\Commands</Filter>
     </ClInclude>
     <ClInclude Include="..\..\Engine\Code\Engine\Resource\ResourceCommand.hpp">
       <Filter>Engine\Core\Async\Commands</Filter>
     </ClInclude>

     <!-- Queues (grouped logically, but stay in original directories) -->
     <ClInclude Include="..\..\Engine\Code\Engine\Renderer\RenderCommandQueue.hpp">
       <Filter>Engine\Core\Async\Queues</Filter>
     </ClInclude>
     <ClInclude Include="..\..\Engine\Code\Engine\Audio\AudioCommandQueue.hpp">
       <Filter>Engine\Core\Async\Queues</Filter>
     </ClInclude>
     <ClInclude Include="..\..\Engine\Code\Engine\Resource\ResourceCommandQueue.hpp">
       <Filter>Engine\Core\Async\Queues</Filter>
     </ClInclude>
     <ClInclude Include="..\..\Engine\Code\Engine\Core\CallbackQueue.hpp">
       <Filter>Engine\Core\Async\Queues</Filter>
     </ClInclude>
   </ItemGroup>
   ```

3. WHEN reviewing in Visual Studio Solution Explorer THEN the developer SHALL:
   - See virtual "Engine\Core\Async" filter with subfolders (Base, Commands, Queues)
   - Navigate to files grouped by async role, not physical location
   - Retain original include paths (no breaking changes to `#include` statements)
   - Understand async architecture at a glance via IDE organization

## Non-Functional Requirements

### Code Architecture and Modularity

#### Single Responsibility Principle (SOLID - S)
- **Renderer Module**: DirectX API wrapping, rendering pipeline, shader binding, draw calls
- **Resource Module**: File I/O, resource loading, caching, lifetime management
- **Core Module**: Lock-free queue templates, shared async infrastructure (CommandQueueBase)
- **Each Module's Queue**: Module-specific commands and queue specializations remain with their module

#### Open/Closed Principle (SOLID - O)
- **CommandQueueBase**: Extensible via inheritance, closed for modification
- **Virtual Hooks**: Subclasses extend behavior without modifying base template

#### Dependency Inversion Principle (SOLID - D)
- **ResourceSubsystem**: Core module has ZERO dependency on Script module
- **Optional Injection**: Command queues injected via `SetCommandQueue(queue)` if scripting enabled
- **Conditional Compilation**: `#if ENGINE_SCRIPTING_ENABLED` prevents hard dependencies

### Performance

1. **Queue Operations**:
   - Submit latency: <0.5ms (lock-free, no blocking)
   - Consume latency: O(n) where n = commands per frame
   - Memory overhead: <200 KB total for all queues

2. **Template Instantiation**:
   - Zero runtime overhead vs handwritten queues
   - Compiler optimization SHALL inline template code
   - Binary size increase: <50 KB total

3. **Backward Compatibility**:
   - Existing C++ APIs SHALL maintain identical performance
   - ResourceSubsystem sync/async APIs unchanged
   - Renderer rendering performance unaffected

### Reliability

1. **Backpressure Handling**:
   - Full queue SHALL return `false` from `Submit()` (non-blocking)
   - System SHALL log warnings when queues are >80% full
   - Commands SHALL be dropped gracefully with error logging

2. **Thread Safety**:
   - SPSC queues guarantee thread safety (single producer, single consumer)
   - Memory ordering SHALL prevent data races
   - No locks, no deadlocks, no priority inversion

3. **Crash Isolation**:
   - JavaScript resource loading errors SHALL NOT crash C++
   - Audio loading failures SHALL return error via callback
   - Render thread SHALL continue if resource loading fails

### Maintainability

1. **Code Duplication Elimination**:
   - Eliminate ~800 lines of duplicate queue code
   - Reduce to 1 base template + 5 specializations (<150 lines total)

2. **Documentation**:
   - `Engine/Core/Async/README.md` SHALL document architecture
   - Each queue SHALL have usage examples in header comments
   - CLAUDE.md files SHALL be updated with new structure

3. **Testing**:
   - Validate queue refactoring preserves behavior (no regressions)
   - Test async resource loading from JavaScript
   - Test async audio playback from JavaScript
   - Benchmark performance (ensure <5% variance)

### Usability

1. **Developer Experience**:
   - Adding new queue type SHALL require <30 lines of code
   - Clear error messages when queues are full
   - Consistent API across all async subsystems

2. **Debugging**:
   - Queue statistics exposed: `GetTotalSubmitted()`, `GetTotalConsumed()`
   - Approximate queue size for monitoring
   - Virtual `OnQueueFull()` hook for custom logging

## Success Metrics

1. **Code Quality**:
   - Eliminate 800+ lines of duplicate queue code ✅
   - All async subsystems use unified pattern ✅
   - SOLID principles validated (no Renderer file I/O) ✅

2. **Performance**:
   - Zero regression in existing benchmarks ✅
   - Audio/Resource async operations non-blocking ✅
   - Queue operations <0.5ms latency ✅

3. **Architecture**:
   - ProtogameJS3D becomes proven template ✅
   - DaemonAgent can start from clean architecture ✅
   - Future queue types trivial to add (<30 lines) ✅
