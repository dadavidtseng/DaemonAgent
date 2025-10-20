//----------------------------------------------------------------------------------------------------
// CameraAPI.js
// Phase 2: High-Level Camera API Wrapper for C++ camera interface (globalThis.entity)
// Part of the interface wrapper layer for C++/JavaScript bridge
//----------------------------------------------------------------------------------------------------

/**
 * CameraAPI - High-level abstraction over C++ camera system (Phase 2)
 *
 * This wrapper provides a JavaScript-friendly interface to the C++ high-level camera API,
 * enabling easy camera creation and management with support for multiple cameras and overlays.
 *
 * Design Principles:
 * - Single Responsibility: Only wraps C++ camera interface
 * - Safe Fallbacks: Returns sensible defaults if C++ interface unavailable
 * - Type Safety: Clear return types and parameter documentation
 * - Async Callbacks: Camera creation uses callbacks for async results
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
 * C++ Interface Methods (exposed via globalThis.entity):
 * - createCamera(position, lookAt, type, callback): callbackId
 * - moveCamera(cameraId, position): void
 * - moveCameraBy(cameraId, delta): void
 * - lookAtCamera(cameraId, target): void
 *
 * Usage Example:
 * ```javascript
 * const cameraAPI = new CameraAPI();
 *
 * // Create a world camera
 * cameraAPI.createCamera([0, -10, 5], [0, 0, 0], 'world', (cameraId) => {
 *     console.log('Camera created with ID:', cameraId);
 *
 *     // Move the camera
 *     cameraAPI.moveCamera(cameraId, [0, -15, 5]);
 *
 *     // Look at a new target
 *     cameraAPI.lookAt(cameraId, [10, 0, 0]);
 * });
 * ```
 */
export class CameraAPI
{
    constructor()
    {
        this.cppEntity = globalThis.entity; // C++ entity interface (cameras are part of entity system)

        if (!this.cppEntity)
        {
            console.warn('CameraAPI: C++ entity interface (globalThis.entity) not available');
        }
        else
        {
            console.log('CameraAPI: Successfully connected to C++ high-level camera interface (Phase 2)');
        }
    }

    //----------------------------------------------------------------------------------------------------
    // Camera Creation and Management
    //----------------------------------------------------------------------------------------------------

    /**
     * Create a camera with specified position, target, and type (async)
     * @param {Array<number>} position - [x, y, z] camera position (X-forward, Y-left, Z-up)
     * @param {Array<number>} lookAt - [x, y, z] target position to look at
     * @param {string} type - Camera type: 'world' (3D perspective) or 'screen' (2D orthographic)
     * @param {Function} callback - Callback function(cameraId) called when camera is created
     * @returns {number} callbackId - ID for tracking the callback
     */
    createCamera(position, lookAt, type, callback)
    {
        if (!this.cppEntity || !this.cppEntity.createCamera)
        {
            console.log('CameraAPI: ERROR - createCamera not available');
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

        if (!Array.isArray(lookAt) || lookAt.length !== 3)
        {
            console.log('CameraAPI: ERROR - lookAt must be [x, y, z] array');
            if (callback) callback(0);
            return 0;
        }

        if (type !== 'world' && type !== 'screen')
        {
            console.log('CameraAPI: ERROR - type must be "world" or "screen"');
            if (callback) callback(0);
            return 0;
        }

        // Call C++ interface
        try
        {
            return this.cppEntity.createCamera(position, lookAt, type, callback);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - createCamera exception:', error);
            if (callback) callback(0);
            return 0;
        }
    }

    /**
     * Move camera to absolute position
     * @param {number} cameraId - Camera ID to move
     * @param {Array<number>} position - [x, y, z] new position (X-forward, Y-left, Z-up)
     */
    moveCamera(cameraId, position)
    {
        if (!this.cppEntity || !this.cppEntity.moveCamera)
        {
            console.log('CameraAPI: ERROR - moveCamera not available');
            return;
        }

        if (!Array.isArray(position) || position.length !== 3)
        {
            console.log('CameraAPI: ERROR - position must be [x, y, z] array');
            return;
        }

        try
        {
            this.cppEntity.moveCamera(cameraId, position);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - moveCamera exception:', error);
        }
    }

    /**
     * Move camera by relative delta
     * @param {number} cameraId - Camera ID to move
     * @param {Array<number>} delta - [dx, dy, dz] movement delta (X-forward, Y-left, Z-up)
     */
    moveCameraBy(cameraId, delta)
    {
        if (!this.cppEntity || !this.cppEntity.moveCameraBy)
        {
            console.log('CameraAPI: ERROR - moveCameraBy not available');
            return;
        }

        if (!Array.isArray(delta) || delta.length !== 3)
        {
            console.log('CameraAPI: ERROR - delta must be [dx, dy, dz] array');
            return;
        }

        try
        {
            this.cppEntity.moveCameraBy(cameraId, delta);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - moveCameraBy exception:', error);
        }
    }

    /**
     * Make camera look at a target position
     * @param {number} cameraId - Camera ID to update
     * @param {Array<number>} target - [x, y, z] target position to look at (X-forward, Y-left, Z-up)
     */
    lookAt(cameraId, target)
    {
        if (!this.cppEntity || !this.cppEntity.lookAtCamera)
        {
            console.log('CameraAPI: ERROR - lookAtCamera not available');
            return;
        }

        if (!Array.isArray(target) || target.length !== 3)
        {
            console.log('CameraAPI: ERROR - target must be [x, y, z] array');
            return;
        }

        try
        {
            this.cppEntity.lookAtCamera(cameraId, target);
        }
        catch (error)
        {
            console.log('CameraAPI: ERROR - lookAtCamera exception:', error);
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
        return this.cppEntity !== undefined && this.cppEntity !== null;
    }

    /**
     * Get interface status for debugging
     * @returns {Object} Status object with availability and method information
     */
    getStatus()
    {
        return {
            available: this.isAvailable(),
            cppInterfaceType: typeof this.cppEntity,
            hasMethods: this.cppEntity ? {
                createCamera: typeof this.cppEntity.createCamera === 'function',
                moveCamera: typeof this.cppEntity.moveCamera === 'function',
                moveCameraBy: typeof this.cppEntity.moveCameraBy === 'function',
                lookAtCamera: typeof this.cppEntity.lookAtCamera === 'function'
            } : null
        };
    }
}

// Export for ES6 module system
export default CameraAPI;

// Export to globalThis for hot-reload detection
globalThis.CameraAPI = CameraAPI;

console.log('CameraAPI: High-level camera wrapper loaded (Phase 2 Interface Layer)');
