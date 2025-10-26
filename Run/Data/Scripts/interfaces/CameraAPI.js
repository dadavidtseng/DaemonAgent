//----------------------------------------------------------------------------------------------------
// CameraAPI.js
// M4-T8: High-Level Camera API Wrapper for C++ camera interface (globalThis.camera)
// Part of the interface wrapper layer for C++/JavaScript bridge
//----------------------------------------------------------------------------------------------------

/**
 * CameraAPI - High-level abstraction over C++ camera system (M4-T8)
 *
 * This wrapper provides a JavaScript-friendly interface to the C++ high-level camera API,
 * enabling easy camera creation and management with support for multiple cameras and overlays.
 *
 * Design Principles:
 * - Single Responsibility: Only wraps C++ camera interface
 * - Safe Fallbacks: Returns sensible defaults if C++ interface unavailable
 * - Type Safety: Clear return types and parameter documentation
 * - Async Callbacks: Lifecycle methods use callbacks for async results
 * - Error Resilience: JavaScript errors should never crash C++ rendering
 *
 * Coordinate System: X-forward, Y-left, Z-up (right-handed)
 * - +X = forward
 * - +Y = left
 * - +Z = up
 *
 * Camera Types:
 * - 'world': 3D perspective camera for world rendering
 * - 'screen': 2D orthographic camera for UI/HUD overlay
 *
 * M4-T8 CHANGE: C++ Interface Methods (exposed via globalThis.camera - SPLIT FROM globalThis.entity):
 * Lifecycle (async with callback):
 * - create(posX, posY, posZ, yaw, pitch, roll, type, callback): callbackId (FLATTENED API)
 * - setActive(cameraId, callback): callbackId
 * - updateType(cameraId, type, callback): callbackId
 * - destroy(cameraId, callback): callbackId
 *
 * Updates (fire-and-forget, FLATTENED API):
 * - update(cameraId, posX, posY, posZ, yaw, pitch, roll): void [RECOMMENDED - atomic update]
 * - updatePosition(cameraId, posX, posY, posZ): void [DEPRECATED - use update]
 * - updateOrientation(cameraId, yaw, pitch, roll): void [DEPRECATED - use update]
 * - moveBy(cameraId, dx, dy, dz): void
 * - lookAt(cameraId, targetX, targetY, targetZ): void
 * - getHandle(cameraId): number (for debug rendering)
 *
 * Usage Example:
 * ```javascript
 * const cameraAPI = new CameraAPI();
 *
 * // Create a world camera (JavaScript wrapper uses array format for convenience)
 * // Internally converts to flattened C++ call: create(0, -10, 5, 0, 0, 0, 'world', callback)
 * cameraAPI.createCamera([0, -10, 5], [0, 0, 0], 'world', (cameraId) => {
 *     console.log('Camera created with ID:', cameraId);
 *
 *     // Move the camera
 *     cameraAPI.updatePosition(cameraId, [0, -15, 5]);
 *
 *     // Rotate the camera
 *     cameraAPI.updateOrientation(cameraId, [45, 0, 0]);
 *
 *     // Set as active camera
 *     cameraAPI.setActive(cameraId, (result) => {
 *         console.log('Camera activated:', result);
 *     });
 * });
 * ```
 */
export class CameraAPI
{
    constructor()
    {
        this.cppCamera = globalThis.camera; // M4-T8: C++ camera interface (separated from entity system)

        if (!this.cppCamera)
        {
            console.log('CameraAPI: C++ camera interface (globalThis.camera) not available');
        }
        else
        {
            console.log('CameraAPI: Successfully connected to C++ high-level camera interface (M4-T8)');
        }
    }

    //----------------------------------------------------------------------------------------------------
    // Camera Lifecycle Methods (Async with Callbacks)
    //----------------------------------------------------------------------------------------------------

    /**
     * Create a camera with specified position, orientation, and type (async)
     * @param {Array<number>} position - [x, y, z] camera position (X-forward, Y-left, Z-up)
     * @param {Array<number>} orientation - [yaw, pitch, roll] rotation in degrees
     * @param {string} type - Camera type: 'world' (3D perspective) or 'screen' (2D orthographic)
     * @param {Function} callback - Callback function(cameraId) called when camera is created
     * @returns {number} callbackId - ID for tracking the callback
     */
    createCamera(position, orientation, type, callback)
    {
        if (!this.cppCamera || !this.cppCamera.create)
        {
            console.log('CameraAPI: ERROR - create not available');
            if (callback) callback(0); // 0 = creation failed
            return 0;
        }

        // Validate parameters
        if (!Array.isArray(position) || position.length !== 3)
        {
            console.log('CameraAPI: ERROR - position must be [x, y, z] array');
            if (callback) callback(0);
            return 0;
        }

        if (!Array.isArray(orientation) || orientation.length !== 3)
        {
            console.log('CameraAPI: ERROR - orientation must be [yaw, pitch, roll] array');
            if (callback) callback(0);
            return 0;
        }

        if (type !== 'world' && type !== 'screen')
        {
            console.log('CameraAPI: ERROR - type must be "world" or "screen"');
            if (callback) callback(0);
            return 0;
        }

        // FLATTENED API: V8 binding cannot handle nested objects
        // Call C++ with individual primitive arguments instead
        // M4-T8: Signature changed to camera.create(posX, posY, posZ, yaw, pitch, roll, type, callback)
        try
        {
            console.log('CameraAPI: Calling flattened create API with 8 arguments (M4-T8)');
            console.log(`  position: [${position[0]}, ${position[1]}, ${position[2]}]`);
            console.log(`  orientation: [${orientation[0]}, ${orientation[1]}, ${orientation[2]}]`);
            console.log(`  type: ${type}`);

            const result = this.cppCamera.create(
                position[0],        // arg 0: double (posX)
                position[1],        // arg 1: double (posY)
                position[2],        // arg 2: double (posZ)
                orientation[0],     // arg 3: double (yaw)
                orientation[1],     // arg 4: double (pitch)
                orientation[2],     // arg 5: double (roll)
                type,               // arg 6: string (type)
                callback            // arg 7: function
            );

            console.log(`CameraAPI: create returned callbackId: ${result}`);
            return result;
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - create exception:', error);
            if (callback) callback(0);
            return 0;
        }
    }

    /**
     * Set camera as active for rendering (async with callback)
     * @param {number} cameraId - Camera ID to activate
     * @param {Function} callback - Callback function(result) called when operation completes
     * @returns {number} callbackId - ID for tracking the callback
     */
    setActive(cameraId, callback)
    {
        if (!this.cppCamera || !this.cppCamera.setActive)
        {
            console.log('CameraAPI: ERROR - setActive not available');
            if (callback) callback(0);
            return 0;
        }

        try
        {
            return this.cppCamera.setActive(cameraId, callback);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - setActive exception:', error);
            if (callback) callback(0);
            return 0;
        }
    }

    /**
     * Update camera type (async with callback)
     * @param {number} cameraId - Camera ID to update
     * @param {string} type - New camera type: 'world' or 'screen'
     * @param {Function} callback - Callback function(result) called when operation completes
     * @returns {number} callbackId - ID for tracking the callback
     */
    updateType(cameraId, type, callback)
    {
        if (!this.cppCamera || !this.cppCamera.updateType)
        {
            console.log('CameraAPI: ERROR - updateType not available');
            if (callback) callback(0);
            return 0;
        }

        if (type !== 'world' && type !== 'screen')
        {
            console.log('CameraAPI: ERROR - type must be "world" or "screen"');
            if (callback) callback(0);
            return 0;
        }

        try
        {
            return this.cppCamera.updateType(cameraId, type, callback);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - updateType exception:', error);
            if (callback) callback(0);
            return 0;
        }
    }

    /**
     * Destroy camera (async with callback)
     * @param {number} cameraId - Camera ID to destroy
     * @param {Function} callback - Callback function(result) called when operation completes
     * @returns {number} callbackId - ID for tracking the callback
     */
    destroy(cameraId, callback)
    {
        if (!this.cppCamera || !this.cppCamera.destroy)
        {
            console.log('CameraAPI: ERROR - destroy not available');
            if (callback) callback(0);
            return 0;
        }

        try
        {
            return this.cppCamera.destroy(cameraId, callback);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - destroy exception:', error);
            if (callback) callback(0);
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------------------
    // Camera Update Methods (Fire-and-Forget)
    //----------------------------------------------------------------------------------------------------

    /**
     * RECOMMENDED: Update camera position AND orientation atomically (eliminates race conditions)
     * @param {number} cameraId - Camera ID to update
     * @param {number} posX - X position (forward direction)
     * @param {number} posY - Y position (left direction)
     * @param {number} posZ - Z position (up direction)
     * @param {number} yaw - Yaw rotation in degrees
     * @param {number} pitch - Pitch rotation in degrees
     * @param {number} roll - Roll rotation in degrees
     */
    update(cameraId, posX, posY, posZ, yaw, pitch, roll)
    {
        if (!this.cppCamera || !this.cppCamera.update)
        {
            console.log('CameraAPI: ERROR - update not available');
            return;
        }

        try
        {
            // FLATTENED API: V8 binding cannot handle nested objects
            // Call C++ with individual primitive arguments
            // M4-T8: Signature changed to camera.update(cameraId, posX, posY, posZ, yaw, pitch, roll)
            this.cppCamera.update(cameraId, posX, posY, posZ, yaw, pitch, roll);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - update exception:', error);
        }
    }

    /**
     * DEPRECATED: Update camera position only (use update() instead to avoid race conditions)
     * @param {number} cameraId - Camera ID to update
     * @param {Array<number>} position - [x, y, z] new position (X-forward, Y-left, Z-up)
     */
    updatePosition(cameraId, position)
    {
        if (!this.cppCamera || !this.cppCamera.updatePosition)
        {
            console.log('CameraAPI: ERROR - updatePosition not available');
            return;
        }

        if (!Array.isArray(position) || position.length !== 3)
        {
            console.log('CameraAPI: ERROR - position must be [x, y, z] array');
            return;
        }

        try
        {
            // FLATTENED API: V8 binding cannot handle nested objects
            // Call C++ with individual primitive arguments instead
            // M4-T8: Signature changed to camera.updatePosition(cameraId, posX, posY, posZ)
            this.cppCamera.updatePosition(cameraId, position[0], position[1], position[2]);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - updatePosition exception:', error);
        }
    }

    /**
     * Update camera orientation (absolute rotation)
     * @param {number} cameraId - Camera ID to update
     * @param {Array<number>} orientation - [yaw, pitch, roll] rotation in degrees
     */
    updateOrientation(cameraId, orientation)
    {
        if (!this.cppCamera || !this.cppCamera.updateOrientation)
        {
            console.log('CameraAPI: ERROR - updateOrientation not available');
            return;
        }

        if (!Array.isArray(orientation) || orientation.length !== 3)
        {
            console.log('CameraAPI: ERROR - orientation must be [yaw, pitch, roll] array');
            return;
        }

        try
        {
            // FLATTENED API: V8 binding cannot handle nested objects
            // Call C++ with individual primitive arguments instead
            // M4-T8: Signature changed to camera.updateOrientation(cameraId, yaw, pitch, roll)
            this.cppCamera.updateOrientation(cameraId, orientation[0], orientation[1], orientation[2]);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - updateOrientation exception:', error);
        }
    }

    /**
     * Move camera by relative delta
     * @param {number} cameraId - Camera ID to move
     * @param {Array<number>} delta - [dx, dy, dz] movement delta (X-forward, Y-left, Z-up)
     */
    moveBy(cameraId, delta)
    {
        if (!this.cppCamera || !this.cppCamera.moveBy)
        {
            console.log('CameraAPI: ERROR - moveBy not available');
            return;
        }

        if (!Array.isArray(delta) || delta.length !== 3)
        {
            console.log('CameraAPI: ERROR - delta must be [dx, dy, dz] array');
            return;
        }

        try
        {
            // FLATTENED API: V8 binding cannot handle nested objects
            // Call C++ with individual primitive arguments instead
            // M4-T8: Signature changed to camera.moveBy(cameraId, dx, dy, dz)
            this.cppCamera.moveBy(cameraId, delta[0], delta[1], delta[2]);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - moveBy exception:', error);
        }
    }

    /**
     * Make camera look at a target position
     * @param {number} cameraId - Camera ID to update
     * @param {Array<number>} target - [x, y, z] target position to look at (X-forward, Y-left, Z-up)
     */
    lookAt(cameraId, target)
    {
        if (!this.cppCamera || !this.cppCamera.lookAt)
        {
            console.log('CameraAPI: ERROR - lookAt not available');
            return;
        }

        if (!Array.isArray(target) || target.length !== 3)
        {
            console.log('CameraAPI: ERROR - target must be [x, y, z] array');
            return;
        }

        try
        {
            // FLATTENED API: V8 binding cannot handle nested objects
            // Call C++ with individual primitive arguments instead
            // M4-T8: Signature changed to camera.lookAt(cameraId, targetX, targetY, targetZ)
            this.cppCamera.lookAt(cameraId, target[0], target[1], target[2]);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - lookAt exception:', error);
        }
    }

    //----------------------------------------------------------------------------------------------------
    // Utility Methods
    //----------------------------------------------------------------------------------------------------

    /**
     * Check if C++ camera interface is available
     * @returns {boolean} True if C++ interface is connected
     */
    isAvailable()
    {
        return this.cppCamera !== undefined && this.cppCamera !== null;
    }

    /**
     * Get interface status for debugging
     * @returns {Object} Status object with availability and method information
     */
    getStatus()
    {
        return {
            available: this.isAvailable(),
            cppInterfaceType: typeof this.cppCamera,
            hasMethods: this.cppCamera ? {
                // Lifecycle methods (M4-T8: method names updated)
                create: typeof this.cppCamera.create === 'function',
                setActive: typeof this.cppCamera.setActive === 'function',
                updateType: typeof this.cppCamera.updateType === 'function',
                destroy: typeof this.cppCamera.destroy === 'function',
                // Update methods (RECOMMENDED: use update for atomic updates)
                update: typeof this.cppCamera.update === 'function',
                updatePosition: typeof this.cppCamera.updatePosition === 'function',
                updateOrientation: typeof this.cppCamera.updateOrientation === 'function',
                moveBy: typeof this.cppCamera.moveBy === 'function',
                lookAt: typeof this.cppCamera.lookAt === 'function',
                getHandle: typeof this.cppCamera.getHandle === 'function'
            } : null
        };
    }
}

// Export for ES6 module system
export default CameraAPI;

// Export to globalThis for hot-reload detection
globalThis.CameraAPI = CameraAPI;

console.log('CameraAPI: High-level camera wrapper loaded (M4-T8 Interface Layer)');
