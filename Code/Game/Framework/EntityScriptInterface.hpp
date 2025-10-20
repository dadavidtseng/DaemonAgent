//----------------------------------------------------------------------------------------------------
// EntityScriptInterface.hpp
// Phase 2: JavaScript Interface for High-Level Entity/Camera API
//
// Purpose:
//   Exposes HighLevelEntityAPI to JavaScript runtime through IScriptableObject interface.
//   Provides user-friendly JavaScript APIs for entity, camera, and light management.
//
// Design Philosophy:
//   - Clean separation from RendererScriptInterface (rendering internals)
//   - High-level, user-friendly API for JavaScript developers
//   - Error-resilient (JavaScript errors don't crash C++ rendering)
//   - Async callbacks for creation operations
//
// JavaScript API (exposed methods):
//   Entity Management:
//     - entity.createMesh(type, properties, callback)
//     - entity.updatePosition(entityId, {x, y, z})
//     - entity.moveBy(entityId, {dx, dy, dz})
//     - entity.updateOrientation(entityId, {yaw, pitch, roll})
//     - entity.updateColor(entityId, {r, g, b, a})
//     - entity.destroy(entityId)
//
//   Camera Management:
//     - camera.create(properties, callback)
//     - camera.move(cameraId, {x, y, z})
//     - camera.moveBy(cameraId, {dx, dy, dz})
//     - camera.lookAt(cameraId, {x, y, z})
//
// Usage Example (from JavaScript):
//   // Create a cube entity
//   entity.createMesh('cube', {
//       position: {x: 5, y: 0, z: 0},
//       scale: 1.0,
//       color: {r: 255, g: 0, b: 0, a: 255}
//   }, (entityId) => {
//       console.log('Entity created:', entityId);
//       // Update entity position
//       entity.updatePosition(entityId, {x: 10, y: 0, z: 0});
//   });
//
//   // Create a camera
//   camera.create({
//       position: {x: -10, y: 0, z: 5},
//       lookAt: {x: 0, y: 0, z: 0},
//       type: 'world'
//   }, (cameraId) => {
//       console.log('Camera created:', cameraId);
//   });
//
// Thread Safety:
//   - All methods submit commands to RenderCommandQueue (lock-free)
//   - Callbacks executed on JavaScript worker thread
//   - V8 locking handled internally by ExecutePendingCallbacks()
//
// Author: Phase 2 - High-Level API Implementation
// Date: 2025-10-18
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"
#include "Game/Framework/HighLevelEntityAPI.hpp"

#include <memory>

//----------------------------------------------------------------------------------------------------
// Forward Declarations
class HighLevelEntityAPI;

//----------------------------------------------------------------------------------------------------
// EntityScriptInterface
//
// JavaScript interface for high-level entity/camera management.
// Wraps HighLevelEntityAPI and exposes methods to V8 JavaScript runtime.
//
// Registration:
//   - Registered in ScriptSubsystem as "entity" global object
//   - Camera methods may be exposed as separate "camera" global (TBD)
//
// Method Naming Convention:
//   - JavaScript methods use camelCase (e.g., createMesh, moveBy)
//   - C++ methods map to HighLevelEntityAPI (e.g., CreateMesh, MoveBy)
//
// Error Handling:
//   - Invalid parameters return ScriptMethodResult::Error()
//   - Errors logged to console, don't crash C++ rendering
//   - Callbacks with error status notify JavaScript of failures
//----------------------------------------------------------------------------------------------------
class EntityScriptInterface : public IScriptableObject
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	explicit EntityScriptInterface(HighLevelEntityAPI* entityAPI);
	~EntityScriptInterface() override = default;

	//------------------------------------------------------------------------------------------------
	// IScriptableObject Interface
	//------------------------------------------------------------------------------------------------
	void                          InitializeMethodRegistry() override;
	ScriptMethodResult            CallMethod(String const& methodName, ScriptArgs const& args) override;
	std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
	std::vector<String>           GetAvailableProperties() const override;
	std::any                      GetProperty(String const& propertyName) const override;
	bool                          SetProperty(String const& propertyName, std::any const& value) override;

private:
	//------------------------------------------------------------------------------------------------
	// Entity Management Methods (exposed to JavaScript)
	//------------------------------------------------------------------------------------------------

	// Create a mesh entity (async with callback)
	// JavaScript signature: createMesh(type, properties, callback)
	// Parameters:
	//   - type: string ("cube", "sphere", "grid", "plane")
	//   - properties: object {position: {x, y, z}, scale: number, color: {r, g, b, a}}
	//   - callback: function (entityId) => {...}
	ScriptMethodResult ExecuteCreateMesh(ScriptArgs const& args);

	// Update entity position (absolute, world-space)
	// JavaScript signature: updatePosition(entityId, position)
	// Parameters:
	//   - entityId: number
	//   - position: object {x, y, z}
	ScriptMethodResult ExecuteUpdatePosition(ScriptArgs const& args);

	// Move entity by delta (relative movement)
	// JavaScript signature: moveBy(entityId, delta)
	// Parameters:
	//   - entityId: number
	//   - delta: object {dx, dy, dz}
	ScriptMethodResult ExecuteMoveBy(ScriptArgs const& args);

	// Update entity orientation (Euler angles in degrees)
	// JavaScript signature: updateOrientation(entityId, orientation)
	// Parameters:
	//   - entityId: number
	//   - orientation: object {yaw, pitch, roll}
	ScriptMethodResult ExecuteUpdateOrientation(ScriptArgs const& args);

	// Update entity color
	// JavaScript signature: updateColor(entityId, color)
	// Parameters:
	//   - entityId: number
	//   - color: object {r, g, b, a}
	ScriptMethodResult ExecuteUpdateColor(ScriptArgs const& args);

	// Destroy entity
	// JavaScript signature: destroy(entityId)
	// Parameters:
	//   - entityId: number
	ScriptMethodResult ExecuteDestroyEntity(ScriptArgs const& args);

	//------------------------------------------------------------------------------------------------
	// Camera Management Methods (exposed to JavaScript)
	//------------------------------------------------------------------------------------------------

	// Create a camera (async with callback)
	// JavaScript signature: createCamera(properties, callback)
	// Parameters:
	//   - properties: object {position: {x, y, z}, lookAt: {x, y, z}, type: string}
	//   - callback: function (cameraId) => {...}
	ScriptMethodResult ExecuteCreateCamera(ScriptArgs const& args);

	// Move camera to absolute position
	// JavaScript signature: moveCamera(cameraId, position)
	// Parameters:
	//   - cameraId: number
	//   - position: object {x, y, z}
	ScriptMethodResult ExecuteMoveCamera(ScriptArgs const& args);

	// Move camera by delta (relative movement)
	// JavaScript signature: moveCameraBy(cameraId, delta)
	// Parameters:
	//   - cameraId: number
	//   - delta: object {dx, dy, dz}
	ScriptMethodResult ExecuteMoveCameraBy(ScriptArgs const& args);

	// Point camera at target position
	// JavaScript signature: lookAtCamera(cameraId, target)
	// Parameters:
	//   - cameraId: number
	//   - target: object {x, y, z}
	ScriptMethodResult ExecuteLookAtCamera(ScriptArgs const& args);

	//------------------------------------------------------------------------------------------------
	// Helper Methods
	//------------------------------------------------------------------------------------------------

	// Extract Vec3 from JavaScript object {x, y, z}
	// Returns std::nullopt if extraction fails
	std::optional<Vec3> ExtractVec3(std::any const& value) const;

	// Extract Rgba8 from JavaScript object {r, g, b, a}
	// Returns std::nullopt if extraction fails
	std::optional<Rgba8> ExtractRgba8(std::any const& value) const;

	// Extract EulerAngles from JavaScript object {yaw, pitch, roll}
	// Returns std::nullopt if extraction fails
	std::optional<EulerAngles> ExtractEulerAngles(std::any const& value) const;

	// Extract EntityID (uint64_t) from JavaScript number
	// Returns std::nullopt if extraction fails
	std::optional<EntityID> ExtractEntityID(std::any const& value) const;

	// Extract callback function from std::any
	// Returns std::nullopt if extraction fails
	std::optional<ScriptCallback> ExtractCallback(std::any const& value) const;

private:
	//------------------------------------------------------------------------------------------------
	// Internal State
	//------------------------------------------------------------------------------------------------
	HighLevelEntityAPI* m_entityAPI;  // Pointer to high-level entity API (owned by App/Game)
};

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Method Naming Convention:
//   - JavaScript uses camelCase: createMesh, updatePosition, moveBy
//   - C++ uses PascalCase: ExecuteCreateMesh, ExecuteUpdatePosition, ExecuteMoveBy
//
// Parameter Extraction Strategy:
//   - All JavaScript objects passed as std::any (type-erased)
//   - Helper methods (ExtractVec3, ExtractRgba8, etc.) validate and extract types
//   - Extraction failures return ScriptMethodResult::Error() with descriptive message
//   - No crashes on invalid JavaScript input (defensive programming)
//
// Callback Handling:
//   - Callbacks stored in HighLevelEntityAPI::m_pendingCallbacks
//   - Executed by HighLevelEntityAPI::ExecutePendingCallbacks() on worker thread
//   - V8 locking handled by HighLevelEntityAPI (not this interface)
//
// Error Resilience:
//   - All parameter extraction wrapped in try-catch (std::bad_any_cast protection)
//   - Invalid parameters → ScriptMethodResult::Error() → JavaScript receives error
//   - C++ rendering continues regardless of JavaScript errors
//
// Future Extensions (Phase 2b):
//   - Add light management methods (createLight, updateLight, destroyLight)
//   - Add batch entity creation (createMeshBatch for multiple entities)
//   - Add entity query methods (getEntityPosition, getEntityCount)
//----------------------------------------------------------------------------------------------------
