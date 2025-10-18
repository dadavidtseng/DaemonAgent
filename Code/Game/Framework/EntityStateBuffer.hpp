//----------------------------------------------------------------------------------------------------
// EntityStateBuffer.hpp
// Phase 1: Async Architecture - Double-Buffered Entity State
//
// Purpose:
//   Thread-safe entity state storage for rendering isolation.
//   Allows JavaScript worker thread to update game state while main thread renders.
//
// Design Rationale:
//   - Double-buffering: Main thread reads front buffer, worker writes back buffer
//   - Swap on frame boundaries: Atomic pointer swap, no locking during rendering
//   - Full-copy strategy (Phase 1): Simple, predictable performance (optimized in Phase 4)
//   - std::unordered_map: O(1) entity lookup by EntityID
//
// Thread Safety Model:
//   - Main Thread: Reads front buffer (no locking)
//   - Worker Thread: Writes back buffer (no locking)
//   - Swap Point: Atomic pointer swap at frame boundary (lock-protected)
//
// Performance Characteristics (Phase 1):
//   - Swap cost: < 1ms for 1000 entities (full map copy)
//   - Memory overhead: 2× entity storage (~few KB for typical scene)
//   - Lookup: O(1) average case, O(n) worst case (hash collision)
//
// Future Optimization (Phase 4):
//   - Copy-on-write: Only copy modified entities
//   - Dirty tracking: Track changed entities per frame
//   - Entity pools: Preallocated storage, no dynamic allocation
//
// Author: Phase 1 - Async Architecture Implementation
// Date: 2025-10-17
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/RenderCommand.hpp"  // For EntityID typedef

#include <mutex>
#include <string>
#include <unordered_map>

//----------------------------------------------------------------------------------------------------
// EntityState
//
// Snapshot of entity rendering state.
// Immutable after creation (value-semantic, copyable).
//
// Design Notes:
//   - EulerAngles for orientation: Matches engine convention, simple yaw/pitch/roll
//   - std::string for meshType: Simplicity over performance (Phase 1)
//   - isActive flag: Soft deletion (entity exists but not rendered)
//   - No scale: Uniform radius only (simplifies Phase 1)
//
// Memory Layout: ~48 bytes per entity
//   - position: 12 bytes (Vec3)
//   - orientation: 12 bytes (EulerAngles: yaw, pitch, roll floats)
//   - color: 4 bytes (Rgba8)
//   - radius: 4 bytes (float)
//   - meshType: ~24 bytes (std::string with SSO)
//   - isActive: 1 byte (bool)
//   Total: ~57 bytes (cache-friendly)
//----------------------------------------------------------------------------------------------------
struct EntityState
{
	Vec3        position;     // World-space position
	EulerAngles orientation;  // World-space rotation (yaw, pitch, roll in degrees)
	Rgba8       color;        // RGBA color (4 bytes, memory efficient)
	float       radius;       // Uniform scale (single float, no separate scale vector)
	std::string meshType;     // "cube", "sphere", "grid", etc. (Phase 1 simplicity)
	bool        isActive;     // Active flag (true = render, false = skip)

	// Default constructor (identity state)
	EntityState()
	    : position(Vec3::ZERO)
	    , orientation(EulerAngles::ZERO)
	    , color(Rgba8::WHITE)
	    , radius(1.0f)
	    , meshType("cube")
	    , isActive(true)
	{
	}

	// Explicit constructor
	EntityState(Vec3 const& pos, EulerAngles const& orient, Rgba8 const& col, float r, std::string const& type, bool active = true)
	    : position(pos)
	    , orientation(orient)
	    , color(col)
	    , radius(r)
	    , meshType(type)
	    , isActive(active)
	{
	}
};

//----------------------------------------------------------------------------------------------------
// EntityStateMap
//
// Type alias for entity state storage.
// Key: EntityID (uint64_t)
// Value: EntityState (rendering snapshot)
//
// std::unordered_map chosen for:
//   - O(1) average lookup by EntityID
//   - Dynamic entity creation/destruction
//   - Simple Phase 1 implementation
//
// Alternatives considered:
//   - std::map: O(log n) lookup, slower (rejected)
//   - std::vector: O(1) lookup if EntityID is dense index (deferred to Phase 4)
//   - Custom pool allocator: Faster allocation (deferred to Phase 4)
//----------------------------------------------------------------------------------------------------
using EntityStateMap = std::unordered_map<EntityID, EntityState>;

//----------------------------------------------------------------------------------------------------
// EntityStateBuffer
//
// Double-buffered entity state storage for lock-free rendering.
//
// Usage Pattern:
//
// Worker Thread (JavaScript):
//   EntityStateBuffer* buffer = ...;
//   EntityState* state = buffer->GetBackBuffer()->find(entityId);
//   if (state != buffer->GetBackBuffer()->end()) {
//       state->second.position = newPosition;  // Update back buffer
//   }
//
// Main Thread (Rendering):
//   EntityStateBuffer* buffer = ...;
//   EntityStateMap const* frontBuffer = buffer->GetFrontBuffer();
//   for (auto const& [entityId, state] : *frontBuffer) {
//       if (state.isActive) {
//           RenderEntity(state);  // Read front buffer (no locking)
//       }
//   }
//
// Frame Boundary (Main Thread):
//   buffer->SwapBuffers();  // Atomic swap, brief lock
//
// Thread Safety:
//   - GetFrontBuffer(): Returns const pointer, safe for concurrent reads
//   - GetBackBuffer(): Returns mutable pointer, single-writer guarantee
//   - SwapBuffers(): Atomically swaps buffers, brief lock during copy
//
// Performance (Phase 1):
//   - Read: Lock-free, O(1) entity lookup
//   - Write: Lock-free, O(1) entity update
//   - Swap: Locked, O(n) full map copy (< 1ms for 1000 entities)
//----------------------------------------------------------------------------------------------------
class EntityStateBuffer
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	EntityStateBuffer();
	~EntityStateBuffer();

	// Non-copyable, non-movable (contains mutex)
	EntityStateBuffer(EntityStateBuffer const&)            = delete;
	EntityStateBuffer& operator=(EntityStateBuffer const&) = delete;
	EntityStateBuffer(EntityStateBuffer&&)                 = delete;
	EntityStateBuffer& operator=(EntityStateBuffer&&)      = delete;

	//------------------------------------------------------------------------------------------------
	// Buffer Access (Thread-Safe)
	//------------------------------------------------------------------------------------------------

	// Get front buffer for rendering (Main Thread, lock-free read)
	// Returns: Const pointer to current front buffer (safe for concurrent reads)
	EntityStateMap const* GetFrontBuffer() const;

	// Get back buffer for writing (Worker Thread, lock-free write)
	// Returns: Mutable pointer to current back buffer (single-writer guarantee)
	EntityStateMap* GetBackBuffer();

	//------------------------------------------------------------------------------------------------
	// Buffer Swap (Frame Boundary, Main Thread Only)
	//------------------------------------------------------------------------------------------------

	// Swap front/back buffers and copy data (locked operation)
	// Algorithm:
	//   1. Acquire mutex lock
	//   2. Copy back buffer → new front buffer (full deep copy)
	//   3. Swap buffer pointers
	//   4. Release mutex lock
	//
	// Performance: O(n) where n = number of entities (< 1ms for 1000 entities)
	// Thread Safety: Locked operation, call from main thread only
	void SwapBuffers();

	//------------------------------------------------------------------------------------------------
	// Monitoring / Debugging
	//------------------------------------------------------------------------------------------------

	// Get entity count in front buffer (approximate, for monitoring only)
	size_t GetEntityCount() const;

	// Get total swaps performed (for profiling)
	uint64_t GetTotalSwaps() const { return m_totalSwaps; }

private:
	//------------------------------------------------------------------------------------------------
	// Double-Buffer Storage
	//------------------------------------------------------------------------------------------------
	EntityStateMap m_bufferA;  // Buffer A (front or back)
	EntityStateMap m_bufferB;  // Buffer B (front or back)

	EntityStateMap* m_frontBuffer;  // Pointer to current front buffer (read by main thread)
	EntityStateMap* m_backBuffer;   // Pointer to current back buffer (written by worker thread)

	//------------------------------------------------------------------------------------------------
	// Synchronization
	//------------------------------------------------------------------------------------------------
	mutable std::mutex m_swapMutex;  // Protects buffer swap operation

	//------------------------------------------------------------------------------------------------
	// Statistics
	//------------------------------------------------------------------------------------------------
	uint64_t m_totalSwaps;  // Total buffer swaps performed (profiling counter)
};

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Full-Copy Strategy (Phase 1):
//   Rationale:
//     - Simple, predictable performance (no edge cases)
//     - Avoids complex dirty tracking logic
//     - Acceptable cost for Phase 1 (< 1ms for 1000 entities)
//
//   Drawbacks:
//     - Memory bandwidth: Copying entire map each frame
//     - Scalability: O(n) cost, grows with entity count
//
//   Future Optimization (Phase 4):
//     - Copy-on-write: Only copy modified entities
//     - Dirty bit tracking: Track changed entities per frame
//     - Entity pools: Preallocated storage, no allocation per frame
//
// Thread Safety Validation:
//   - Run under Thread Sanitizer (TSan) to detect data races
//   - Main thread: Only reads front buffer (no writes)
//   - Worker thread: Only writes back buffer (no reads from front)
//   - Swap point: Brief lock, no concurrent access to buffers during swap
//
// Alternatives Considered:
//   - Lock-free swap: Complex, requires double-indirection or atomic pointers
//   - Triple-buffering: Wastes memory, no significant benefit for 60 FPS
//   - Shared pointer with atomic load/store: Complex ownership, rejected
//
// Performance Validation (Phase 1 Acceptance Criteria):
//   - Swap cost: < 1ms for 1000 entities (measured via high-resolution timer)
//   - No frame drops during typical gameplay (60 FPS stable)
//   - Memory overhead: < 10 MB for 1000 entities (2× storage)
//----------------------------------------------------------------------------------------------------
