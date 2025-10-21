//----------------------------------------------------------------------------------------------------
// CameraStateBuffer.hpp
// Phase 2b: Camera API - Double-Buffered Camera State
//
// Purpose:
//   Thread-safe camera state storage for rendering isolation.
//   Allows JavaScript worker thread to create/update cameras while main thread renders.
//
// Design Rationale:
//   - Double-buffering: Main thread reads front buffer, worker writes back buffer
//   - Separate from EntityStateBuffer: Cameras have different properties and lifecycle
//   - Active camera tracking: Supports switching between multiple cameras
//   - Async creation: Callbacks notify JavaScript when camera is ready
//
// Thread Safety Model:
//   - Main Thread: Reads front buffer (no locking)
//   - Worker Thread: Writes back buffer (no locking)
//   - Swap Point: Atomic pointer swap at frame boundary (lock-protected)
//
// Author: Phase 2b - Camera API Implementation
// Date: 2025-10-20
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/RenderCommand.hpp"  // For EntityID typedef (used as CameraID)
#include "Engine/Renderer/Camera.hpp"

#include <mutex>
#include <string>
#include <unordered_map>

//----------------------------------------------------------------------------------------------------
// CameraState
//
// Snapshot of camera rendering state.
// Immutable after creation (value-semantic, copyable).
//
// Design Notes:
//   - Stores Camera configuration, not Camera* pointer (value semantics)
//   - type: "world" (perspective) or "screen" (orthographic)
//   - isActive: Whether this camera is currently rendering
//   - Camera mode auto-configured based on type during creation
//----------------------------------------------------------------------------------------------------
struct CameraState
{
	Vec3        position;     // World-space position
	EulerAngles orientation;  // World-space rotation (yaw, pitch, roll in degrees)
	std::string type;         // "world" (3D perspective) or "screen" (2D orthographic)
	bool        isActive;     // Active flag (true = rendering, false = inactive)

	// Camera mode configuration (auto-set based on type)
	Camera::Mode mode;        // eMode_Perspective or eMode_Orthographic

	// Perspective camera properties (for "world" type)
	float perspectiveFOV;     // Field of view in degrees (default: 60.0f)
	float perspectiveAspect;  // Aspect ratio (default: 16/9)
	float perspectiveNear;    // Near plane (default: 0.1f)
	float perspectiveFar;     // Far plane (default: 100.0f)

	// Orthographic camera properties (for "screen" type)
	float orthoLeft;          // Left bound (default: 0.0f)
	float orthoBottom;        // Bottom bound (default: 0.0f)
	float orthoRight;         // Right bound (default: 1920.0f)
	float orthoTop;           // Top bound (default: 1080.0f)
	float orthoNear;          // Near plane (default: 0.0f)
	float orthoFar;           // Far plane (default: 1.0f)

	// Viewport configuration (normalized 0-1 coordinates)
	AABB2 viewport;           // Viewport bounds (default: full screen AABB2(0, 0, 1, 1))

	// Default constructor (perspective camera at origin)
	CameraState()
	    : position(Vec3::ZERO)
	    , orientation(EulerAngles::ZERO)
	    , type("world")
	    , isActive(false)
	    , mode(Camera::eMode_Perspective)
	    , perspectiveFOV(60.0f)
	    , perspectiveAspect(16.0f / 9.0f)
	    , perspectiveNear(0.1f)
	    , perspectiveFar(100.0f)
	    , orthoLeft(0.0f)
	    , orthoBottom(0.0f)
	    , orthoRight(1920.0f)
	    , orthoTop(1080.0f)
	    , orthoNear(0.0f)
	    , orthoFar(1.0f)
	    , viewport(0.0f, 0.0f, 1.0f, 1.0f)  // Full screen viewport (normalized)
	{
	}

	// Explicit constructor
	CameraState(Vec3 const& pos, EulerAngles const& orient, std::string const& camType)
	    : position(pos)
	    , orientation(orient)
	    , type(camType)
	    , isActive(false)
	    , viewport(0.0f, 0.0f, 1.0f, 1.0f)  // Full screen viewport (normalized)
	{
		// Auto-configure camera mode and properties based on type
		if (camType == "world")
		{
			mode = Camera::eMode_Perspective;
			perspectiveFOV = 60.0f;
			perspectiveAspect = 16.0f / 9.0f;
			perspectiveNear = 0.1f;
			perspectiveFar = 100.0f;
		}
		else if (camType == "screen")
		{
			mode = Camera::eMode_Orthographic;
			orthoLeft = 0.0f;
			orthoBottom = 0.0f;
			orthoRight = 1920.0f;
			orthoTop = 1080.0f;
			orthoNear = 0.0f;
			orthoFar = 1.0f;
		}
	}
};

//----------------------------------------------------------------------------------------------------
// CameraStateMap
//
// Type alias for camera state storage.
// Key: CameraID (EntityID reused as CameraID)
// Value: CameraState (rendering snapshot)
//----------------------------------------------------------------------------------------------------
using CameraStateMap = std::unordered_map<EntityID, CameraState>;

//----------------------------------------------------------------------------------------------------
// CameraStateBuffer
//
// Double-buffered camera state storage for lock-free rendering.
// Follows same pattern as EntityStateBuffer for consistency.
//
// Usage Pattern:
//
// Worker Thread (JavaScript):
//   CameraStateMap* backBuffer = buffer->GetBackBuffer();
//   (*backBuffer)[cameraId].position = newPosition;  // Update back buffer
//
// Main Thread (Rendering):
//   CameraStateMap const* frontBuffer = buffer->GetFrontBuffer();
//   EntityID activeCameraId = buffer->GetActiveCameraID();
//   auto it = frontBuffer->find(activeCameraId);
//   if (it != frontBuffer->end() && it->second.isActive) {
//       RenderWithCamera(it->second);  // Read front buffer (no locking)
//   }
//
// Frame Boundary (Main Thread):
//   buffer->SwapBuffers();  // Atomic swap, brief lock
//
// Thread Safety:
//   - GetFrontBuffer(): Returns const pointer, safe for concurrent reads
//   - GetBackBuffer(): Returns mutable pointer, single-writer guarantee
//   - SwapBuffers(): Atomically swaps buffers, brief lock during copy
//----------------------------------------------------------------------------------------------------
class CameraStateBuffer
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	CameraStateBuffer();
	~CameraStateBuffer();

	// Non-copyable, non-movable (contains mutex)
	CameraStateBuffer(CameraStateBuffer const&)            = delete;
	CameraStateBuffer& operator=(CameraStateBuffer const&) = delete;
	CameraStateBuffer(CameraStateBuffer&&)                 = delete;
	CameraStateBuffer& operator=(CameraStateBuffer&&)      = delete;

	//------------------------------------------------------------------------------------------------
	// Buffer Access (Thread-Safe)
	//------------------------------------------------------------------------------------------------

	// Get front buffer for rendering (Main Thread, lock-free read)
	CameraStateMap const* GetFrontBuffer() const;

	// Get back buffer for writing (Worker Thread, lock-free write)
	CameraStateMap* GetBackBuffer();

	//------------------------------------------------------------------------------------------------
	// Active Camera Management
	//------------------------------------------------------------------------------------------------

	// Get active camera ID (Main Thread, for rendering)
	EntityID GetActiveCameraID() const { return m_activeCameraID; }

	// Set active camera ID (Worker Thread, via command processing)
	void SetActiveCameraID(EntityID cameraId) { m_activeCameraID = cameraId; }

	//------------------------------------------------------------------------------------------------
	// Buffer Swap (Frame Boundary, Main Thread Only)
	//------------------------------------------------------------------------------------------------

	// Swap front/back buffers and copy data (locked operation)
	void SwapBuffers();

	//------------------------------------------------------------------------------------------------
	// Camera Lookup (Main Thread, for rendering)
	//------------------------------------------------------------------------------------------------

	// Get camera by ID from front buffer (returns nullptr if not found)
	// Converts CameraState to Camera object for rendering
	// Returns pointer to cached Camera object (valid until next SwapBuffers())
	Camera const* GetCameraById(EntityID cameraId) const;

	//------------------------------------------------------------------------------------------------
	// Monitoring / Debugging
	//------------------------------------------------------------------------------------------------

	// Get camera count in front buffer (approximate, for monitoring only)
	size_t GetCameraCount() const;

	// Get total swaps performed (for profiling)
	uint64_t GetTotalSwaps() const { return m_totalSwaps; }

private:
	//------------------------------------------------------------------------------------------------
	// Double-Buffer Storage
	//------------------------------------------------------------------------------------------------
	CameraStateMap m_bufferA;  // Buffer A (front or back)
	CameraStateMap m_bufferB;  // Buffer B (front or back)

	CameraStateMap* m_frontBuffer;  // Pointer to current front buffer (read by main thread)
	CameraStateMap* m_backBuffer;   // Pointer to current back buffer (written by worker thread)

	//------------------------------------------------------------------------------------------------
	// Active Camera Tracking
	//------------------------------------------------------------------------------------------------
	EntityID m_activeCameraID;  // Currently active camera for rendering (0 = no active camera)

	//------------------------------------------------------------------------------------------------
	// Camera Object Cache (for rendering)
	//------------------------------------------------------------------------------------------------
	// Cache of Camera objects converted from CameraState for rendering
	// Updated during SwapBuffers() to match front buffer
	mutable std::unordered_map<EntityID, Camera> m_cameraCache;

	//------------------------------------------------------------------------------------------------
	// Synchronization
	//------------------------------------------------------------------------------------------------
	mutable std::mutex m_swapMutex;  // Protects buffer swap operation

	//------------------------------------------------------------------------------------------------
	// Statistics
	//------------------------------------------------------------------------------------------------
	uint64_t m_totalSwaps;  // Total buffer swaps performed (profiling counter)
};
