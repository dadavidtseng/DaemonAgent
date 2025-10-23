//----------------------------------------------------------------------------------------------------
// HighLevelEntityAPI.hpp
// Phase 2: High-Level Entity/Camera/Light API for JavaScript
//
// Purpose:
//   Provides user-friendly, high-level APIs for JavaScript to create and manipulate entities,
//   cameras, and lights. Replaces low-level rendering methods (setModelConstants, etc.) with
//   coarse-grained operations that are easy to use and error-resilient.
//
// Design Philosophy:
//   - High-level, user-friendly API (10-12 methods total)
//   - Async callbacks for creation operations (error resilience)
//   - C++ auto-manages render state (no JavaScript control over blend/depth modes)
//   - JavaScript errors must NOT crash C++ rendering
//
// API Surface (as per Phase 2 specification):
//   Entity API (5 methods):
//     - createMesh(type, properties, callback) - Async, returns entityId via callback
//     - updatePosition(entityId, {x, y, z}) - Absolute positioning
//     - moveBy(entityId, {dx, dy, dz}) - Relative movement
//     - updateOrientation(entityId, {yaw, pitch, roll}) - Euler angles
//     - updateColor(entityId, {r, g, b, a}) - RGBA color
//     - destroy(entityId) - Remove entity
//
//   Camera API (4 methods):
//     - createCamera(properties, callback) - Async, returns cameraId via callback
//     - moveCamera(cameraId, {x, y, z}) - Absolute positioning
//     - moveCameraBy(cameraId, {dx, dy, dz}) - Relative movement
//     - lookAtCamera(cameraId, {x, y, z}) - Point camera at target
//
//   Light API (3 methods - deferred to Phase 2b):
//     - createLight(properties, callback)
//     - updateLight(lightId, properties)
//     - destroyLight(lightId)
//
// Coordinate System:
//   X-forward, Y-left, Z-up (right-handed)
//   +X = forward, +Y = left, +Z = up
//
// Thread Safety:
//   - Methods submit RenderCommands to RenderCommandQueue (lock-free)
//   - Callbacks executed on JavaScript worker thread (V8 isolation required)
//   - C++ rendering continues even if JavaScript callbacks throw errors
//
// Author: Phase 2 - High-Level API Implementation
// Date: 2025-10-18
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/RenderCommand.hpp"
#include "Engine/Script/ScriptCommon.hpp"

#include <functional>
#include <memory>

//----------------------------------------------------------------------------------------------------
// Forward Declarations
class RenderCommandQueue;
class ScriptSubsystem;
class Renderer;
class CameraStateBuffer;

//----------------------------------------------------------------------------------------------------
// CallbackID Type Definition
// - Unique identifier for async callbacks
// - Maps to JavaScript callback functions stored in V8 runtime
//----------------------------------------------------------------------------------------------------
using CallbackID = uint64_t;

//----------------------------------------------------------------------------------------------------
// ScriptCallback Type Definition
// - Type-erased JavaScript callback function (stored as std::any from V8)
// - Will be converted back to V8 function when executed
//----------------------------------------------------------------------------------------------------
using ScriptCallback = std::any;

//----------------------------------------------------------------------------------------------------
// HighLevelEntityAPI
//
// High-level entity/camera management API for JavaScript.
// Replaces low-level rendering methods with user-friendly operations.
//
// Usage Pattern (from JavaScript):
//
// Entity Creation (Async):
//   entity.createMesh('cube', {
//       position: {x: 5, y: 0, z: 0},  // X-forward, Y-left, Z-up
//       scale: 1.0,
//       color: {r: 255, g: 0, b: 0, a: 255}
//   }, (entityId) => {
//       console.log('Entity created:', entityId);
//       // Use entityId for future updates
//   });
//
// Entity Update (Sync):
//   entity.updatePosition(entityId, {x: 10, y: 0, z: 0});  // Absolute
//   entity.moveBy(entityId, {dx: 1, dy: 0, dz: 0});        // Relative (+X = forward)
//   entity.updateOrientation(entityId, {yaw: 45, pitch: 0, roll: 0});
//   entity.updateColor(entityId, {r: 0, g: 255, b: 0, a: 255});
//
// Camera Creation (Async):
//   camera.createCamera({
//       position: {x: -10, y: 0, z: 5},   // 10 units back, 5 units up
//       lookAt: {x: 0, y: 0, z: 0},        // Looking at origin (forward +X)
//       type: 'world'
//   }, (cameraId) => {
//       console.log('Camera created:', cameraId);
//   });
//
// Camera Update (Sync):
//   camera.moveCamera(cameraId, {x: -5, y: 0, z: 3});      // Absolute
//   camera.moveCameraBy(cameraId, {dx: 1, dy: 0, dz: 0});  // Relative (+X = forward)
//   camera.lookAtCamera(cameraId, {x: 0, y: 0, z: 0});     // Point at target
//
// Error Resilience:
//   - JavaScript callback errors are caught and logged
//   - C++ rendering continues with last valid state
//   - Invalid entityIds are ignored with warning logs
//----------------------------------------------------------------------------------------------------
class HighLevelEntityAPI
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	explicit HighLevelEntityAPI(RenderCommandQueue* commandQueue,
	                             ScriptSubsystem* scriptSubsystem,
	                             Renderer* renderer,
	                             CameraStateBuffer* cameraBuffer);
	~HighLevelEntityAPI();

	// Non-copyable (manages callback state)
	HighLevelEntityAPI(HighLevelEntityAPI const&)            = delete;
	HighLevelEntityAPI& operator=(HighLevelEntityAPI const&) = delete;

	//------------------------------------------------------------------------------------------------
	// Entity API (5 methods)
	//------------------------------------------------------------------------------------------------

	// Create a mesh entity (async, returns entityId via callback)
	// Parameters:
	//   - meshType: "cube", "sphere", "grid", "plane"
	//   - position: {x, y, z} world-space position (X-forward, Y-left, Z-up)
	//   - scale: Uniform scale (float)
	//   - color: {r, g, b, a} RGBA color (0-255)
	//   - callback: JavaScript function (entityId) => {...}
	// Returns: CallbackID (for internal tracking, not exposed to JavaScript)
	CallbackID CreateMesh(std::string const& meshType,
	                      Vec3 const& position,
	                      float scale,
	                      Rgba8 const& color,
	                      ScriptCallback const& callback);

	// Update entity position (absolute, world-space)
	void UpdatePosition(EntityID entityId, Vec3 const& position);

	// Move entity by delta (relative movement)
	// Delta convention: +X = forward, +Y = left, +Z = up
	void MoveBy(EntityID entityId, Vec3 const& delta);

	// Update entity orientation (Euler angles in degrees)
	void UpdateOrientation(EntityID entityId, EulerAngles const& orientation);

	// Update entity color (RGBA)
	void UpdateColor(EntityID entityId, Rgba8 const& color);

	// Destroy entity (remove from rendering)
	void DestroyEntity(EntityID entityId);

	//------------------------------------------------------------------------------------------------
	// Camera API (Phase 2b: 8 methods)
	//------------------------------------------------------------------------------------------------

	// Create camera with specified properties (async, returns cameraId via callback)
	// Parameters:
	//   - position: {x, y, z} world-space position (X-forward, Y-left, Z-up)
	//   - orientation: {yaw, pitch, roll} in degrees
	//   - type: "world" (3D perspective) or "screen" (2D orthographic)
	//   - callback: JavaScript function (cameraId) => {...}
	// Returns: CallbackID (for internal tracking)
	// FOV, aspect ratio, near/far planes auto-configured based on type
	CallbackID CreateCamera(Vec3 const& position,
	                        EulerAngles const& orientation,
	                        std::string const& type,
	                        ScriptCallback const& callback);

	// RECOMMENDED: Update camera position AND orientation atomically (eliminates race conditions)
	// This is the preferred method - sends both position and orientation in a single command
	// No callback - command queued and processed asynchronously
	void UpdateCamera(EntityID cameraId, Vec3 const& position, EulerAngles const& orientation);

	// DEPRECATED: Update camera position only (may cause race conditions with orientation updates)
	// Use UpdateCamera() instead for atomic updates
	// No callback - command queued and processed asynchronously
	void UpdateCameraPosition(EntityID cameraId, Vec3 const& position);

	// DEPRECATED: Update camera orientation only (may cause race conditions with position updates)
	// Use UpdateCamera() instead for atomic updates
	// No callback - command queued and processed asynchronously
	void UpdateCameraOrientation(EntityID cameraId, EulerAngles const& orientation);

	// Move camera by delta (relative movement, fire-and-forget)
	// Delta convention: +X = forward, +Y = left, +Z = up
	// No callback - command queued and processed asynchronously
	void MoveCameraBy(EntityID cameraId, Vec3 const& delta);

	// Point camera at target position (calculates orientation, fire-and-forget)
	// Target: world-space position to look at
	// No callback - command queued and processed asynchronously
	void LookAtCamera(EntityID cameraId, Vec3 const& target);

	// Set active camera (async with callback - rendering switches immediately)
	// Callback notifies JavaScript when camera switch is confirmed
	CallbackID SetActiveCamera(EntityID cameraId, ScriptCallback const& callback);

	// Update camera type (async with callback - requires reconfiguring FOV/etc)
	// Type: "world" (3D perspective) or "screen" (2D orthographic)
	// Callback notifies when reconfiguration is complete
	CallbackID UpdateCameraType(EntityID cameraId, std::string const& type, ScriptCallback const& callback);

	// Destroy camera (async with callback)
	CallbackID DestroyCamera(EntityID cameraId, ScriptCallback const& callback);

	// Get camera pointer by ID (for debug rendering)
	// Returns: Camera pointer as uintptr_t (to pass to JavaScript as number)
	// Returns 0 if camera not found
	// Note: Pointer valid until next SwapBuffers() call
	uintptr_t GetCameraHandle(EntityID cameraId) const;

	//------------------------------------------------------------------------------------------------
	// Light API (3 methods - Phase 2b, deferred)
	//------------------------------------------------------------------------------------------------

	// Create a light source (async, returns lightId via callback)
	// PHASE 2b: Not implemented yet
	CallbackID CreateLight(Vec3 const& position,
	                       Rgba8 const& color,
	                       float intensity,
	                       ScriptCallback const& callback);

	// Update light properties
	// PHASE 2b: Not implemented yet
	void UpdateLight(EntityID lightId, Vec3 const& position, Rgba8 const& color, float intensity);

	// Destroy light source
	// PHASE 2b: Not implemented yet
	void DestroyLight(EntityID lightId);

	//------------------------------------------------------------------------------------------------
	// Callback Execution (called by C++ main thread after command processing)
	//------------------------------------------------------------------------------------------------

	// Execute pending callbacks with results
	// Called by App::Update() after processing render commands
	// Executes callbacks on JavaScript worker thread with V8 locking
	void ExecutePendingCallbacks();

	// Register a callback completion (called by command processor)
	void NotifyCallbackReady(CallbackID callbackId, EntityID resultId);

private:
	//------------------------------------------------------------------------------------------------
	// Internal State
	//------------------------------------------------------------------------------------------------

	RenderCommandQueue* m_commandQueue;     // Queue for submitting render commands
	ScriptSubsystem*    m_scriptSubsystem;  // Script subsystem for callback execution
	Renderer*           m_renderer;         // Renderer for geometry creation
	CameraStateBuffer*  m_cameraBuffer;     // Camera state buffer for camera lookups

	// Entity ID generation
	EntityID m_nextEntityId;   // Auto-incremented entity ID counter
	EntityID m_nextCameraId;   // Auto-incremented camera ID counter
	EntityID m_nextLightId;    // Auto-incremented light ID counter

	// Callback ID generation
	CallbackID m_nextCallbackId;  // Auto-incremented callback ID counter

	// Callback storage (CallbackID → {ScriptFunction, resultId})
	struct PendingCallback
	{
		ScriptCallback callback;
		EntityID       resultId;
		bool           ready;  // True when C++ has processed command and resultId is available
	};
	std::unordered_map<CallbackID, PendingCallback> m_pendingCallbacks;

	//------------------------------------------------------------------------------------------------
	// Helper Methods
	//------------------------------------------------------------------------------------------------

	// Generate next entity ID (thread-safe, called on worker thread)
	EntityID GenerateEntityID();

	// Generate next camera ID
	EntityID GenerateCameraID();

	// Generate next light ID
	EntityID GenerateLightID();

	// Generate next callback ID
	CallbackID GenerateCallbackID();

	// Submit render command to queue (with error handling)
	bool SubmitCommand(RenderCommand const& command);

	// Execute a single callback (with error handling to prevent C++ crash)
	void ExecuteCallback(CallbackID callbackId, EntityID resultId);
};

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Callback Execution Flow:
//   1. JavaScript calls entity.createMesh(..., callback)
//   2. C++ stores callback in m_pendingCallbacks with ready=false
//   3. C++ submits CREATE_MESH command to RenderCommandQueue
//   4. Main thread processes CREATE_MESH, creates entity, calls NotifyCallbackReady()
//   5. Worker thread calls ExecutePendingCallbacks() each frame
//   6. Callback executed with entityId, then removed from m_pendingCallbacks
//
// Error Resilience Strategy:
//   - JavaScript callback errors caught with V8 TryCatch blocks
//   - C++ continues rendering even if callback throws
//   - Invalid entityIds logged as warnings, commands ignored
//   - Queue overflow logged, creation requests dropped (user notified via callback failure)
//
// Thread Safety:
//   - CreateMesh/CreateCamera called on JavaScript worker thread
//   - NotifyCallbackReady called on main thread (command processor)
//   - ExecutePendingCallbacks called on worker thread (requires mutex for m_pendingCallbacks)
//   - V8 locking required for callback execution (v8::Locker)
//
// Coordinate System Conventions:
//   - X-forward (+X points forward in world space)
//   - Y-left (+Y points left in world space)
//   - Z-up (+Z points up in world space)
//   - Right-handed coordinate system
//   - All positions/deltas use this convention
//
// Future Optimizations (Phase 4):
//   - Batch callback execution (reduce V8 locking overhead)
//   - Callback timeout mechanism (prevent stuck callbacks)
//   - Priority queue for camera callbacks (camera updates processed first)
//----------------------------------------------------------------------------------------------------
