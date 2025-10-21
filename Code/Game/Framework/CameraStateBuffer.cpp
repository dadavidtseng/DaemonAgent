//----------------------------------------------------------------------------------------------------
// CameraStateBuffer.cpp
// Phase 2b: Camera API - Double-Buffered Camera State Implementation
//----------------------------------------------------------------------------------------------------

#include "Game/Framework/CameraStateBuffer.hpp"

//----------------------------------------------------------------------------------------------------
// Construction / Destruction
//----------------------------------------------------------------------------------------------------

CameraStateBuffer::CameraStateBuffer()
    : m_frontBuffer(&m_bufferA)
    , m_backBuffer(&m_bufferB)
    , m_activeCameraID(0)  // No active camera initially
    , m_totalSwaps(0)
{
	// Double-buffered storage initialized
	// Front buffer starts empty, back buffer ready for writes
}

//----------------------------------------------------------------------------------------------------
CameraStateBuffer::~CameraStateBuffer()
{
	// std::unordered_map cleanup handled automatically
	// No manual resource deallocation needed
}

//----------------------------------------------------------------------------------------------------
// Buffer Access (Thread-Safe)
//----------------------------------------------------------------------------------------------------

CameraStateMap const* CameraStateBuffer::GetFrontBuffer() const
{
	// Lock-free read from main thread
	// Returns const pointer to prevent accidental modification
	return m_frontBuffer;
}

//----------------------------------------------------------------------------------------------------
CameraStateMap* CameraStateBuffer::GetBackBuffer()
{
	// Lock-free write from worker thread
	// Returns mutable pointer for single-writer pattern
	return m_backBuffer;
}

//----------------------------------------------------------------------------------------------------
// Buffer Swap (Frame Boundary, Main Thread Only)
//----------------------------------------------------------------------------------------------------

void CameraStateBuffer::SwapBuffers()
{
	// Lock during swap to prevent concurrent access
	std::lock_guard<std::mutex> lock(m_swapMutex);

	// Full copy: back buffer → new front buffer
	// Phase 2b: Simple strategy, optimize in Phase 4 if profiling shows need
	*m_frontBuffer = *m_backBuffer;

	// Swap pointers (cheap, no data copy)
	std::swap(m_frontBuffer, m_backBuffer);

	// Rebuild camera cache from new front buffer
	// Convert CameraState structs to Camera objects for rendering
	m_cameraCache.clear();
	for (auto const& [cameraId, cameraState] : *m_frontBuffer)
	{
		// Create Camera object from CameraState
		Camera camera;

		// Set camera position and orientation
		camera.SetPosition(cameraState.position);
		camera.SetOrientation(cameraState.orientation);

		// Configure camera mode (perspective vs orthographic)
		if (cameraState.type == "world")
		{
			// Perspective camera for 3D world rendering
			camera.SetPerspectiveGraphicView(
			    cameraState.perspectiveAspect,
			    cameraState.perspectiveFOV,
			    cameraState.perspectiveNear,
			    cameraState.perspectiveFar
			);
		}
		else if (cameraState.type == "screen")
		{
			// Orthographic camera for 2D UI/HUD rendering
			camera.SetOrthoGraphicView(
			    Vec2(cameraState.orthoLeft, cameraState.orthoBottom),
			    Vec2(cameraState.orthoRight, cameraState.orthoTop),
			    cameraState.orthoNear,
			    cameraState.orthoFar
			);
		}

		// Set viewport (normalized coordinates)
		// Phase 2b: Fix for rendering issue - viewport must be set for proper rendering
		camera.SetNormalizedViewport(cameraState.viewport);

		// Store in cache
		m_cameraCache[cameraId] = camera;
	}

	// Increment swap counter for profiling
	++m_totalSwaps;
}

//----------------------------------------------------------------------------------------------------
// Camera Lookup (Main Thread, for rendering)
//----------------------------------------------------------------------------------------------------

Camera const* CameraStateBuffer::GetCameraById(EntityID cameraId) const
{
	// Look up camera in cache (lock-free, main thread only)
	// Cache is updated during SwapBuffers(), so it matches front buffer
	auto it = m_cameraCache.find(cameraId);
	if (it != m_cameraCache.end())
	{
		return &it->second;  // Return pointer to cached Camera object
	}

	// Camera not found
	return nullptr;
}

//----------------------------------------------------------------------------------------------------
// Monitoring / Debugging
//----------------------------------------------------------------------------------------------------

size_t CameraStateBuffer::GetCameraCount() const
{
	// Lock-free read (approximate count, may be stale)
	// Used for monitoring only, not critical for correctness
	return m_frontBuffer->size();
}
