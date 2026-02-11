//----------------------------------------------------------------------------------------------------
// EntityAPI.js
// Phase 2: High-Level Entity API Wrapper for C++ entity interface (globalThis.entity)
// Part of the interface wrapper layer for C++/JavaScript bridge
//----------------------------------------------------------------------------------------------------

/**
 * EntityAPI - High-level abstraction over C++ entity system (Phase 2)
 *
 * This wrapper provides a JavaScript-friendly interface to the C++ high-level entity API,
 * replacing low-level rendering calls with user-friendly entity management methods.
 *
 * Design Principles:
 * - Single Responsibility: Only wraps C++ entity interface
 * - Safe Fallbacks: Returns sensible defaults if C++ interface unavailable
 * - Type Safety: Clear return types and parameter documentation
 * - Async Callbacks: All creation methods use callbacks for async results
 * - Error Resilience: JavaScript errors should never crash C++ rendering
 *
 * Coordinate System: X-forward, Y-left, Z-up (right-handed)
 * - +X = forward
 * - +Y = left
 * - +Z = up
 *
 * C++ Interface Methods (exposed via globalThis.entity):
 * - createMesh(meshType, properties, callback): callbackId
 * - updatePosition(entityId, position): void
 * - moveBy(entityId, delta): void
 * - updateOrientation(entityId, orientation): void
 * - updateColor(entityId, color): void
 * - destroyEntity(entityId): void
 *
 * Usage Example:
 * ```javascript
 * const entityAPI = new EntityAPI();
 *
 * // Create a cube mesh
 * entityAPI.createMesh('cube', {
 *     position: [0, 0, 0],
 *     scale: 1.0,
 *     color: [255, 255, 255, 255]
 * }, (entityId) => {
 *     console.log('Cube created with ID:', entityId);
 *
 *     // Move the entity
 *     entityAPI.updatePosition(entityId, [10, 0, 0]);
 * });
 * ```
 */
export class EntityAPI
{
    // Version tracking for hot-reload detection
    static version = 6; // FULLY FLATTENED API - All methods now use individual primitive arguments (createMesh, updatePosition, updateOrientation, updateColor, moveBy)

    constructor()
    {
        // Singleton pattern: Return existing instance if available
        if (globalThis.EntityAPI && globalThis.EntityAPI instanceof EntityAPI)
        {
            console.log('EntityAPI: Returning existing singleton instance');
            return globalThis.EntityAPI;
        }

        this.cppEntity = globalThis.entity; // C++ entity interface reference

        // Phase 2.4: Callback registry for C++ → JavaScript callbacks
        this.callbackRegistry = new Map();  // Maps callbackId → callback function

        // Make instance globally accessible for JSEngine callback routing
        globalThis.EntityAPI = this;

        if (!this.cppEntity)
        {
            console.log('EntityAPI: C++ entity interface (globalThis.entity) not available');
        }
        else
        {
            console.log(`EntityAPI: Successfully connected to C++ high-level entity interface (Phase 2.4, version ${EntityAPI.version})`);
        }
    }

    /**
     * Handle callback from C++ (Phase 2.4)
     * Called by JSEngine.executeCallback() when callback dequeued from CallbackQueue
     *
     * @param {number} callbackId - Callback ID from C++
     * @param {number} resultId - Entity ID (or 0 if failed)
     * @param {string} errorMessage - Error message (empty if success)
     */
    handleCallback(callbackId, resultId, errorMessage)
    {
        // Look up callback function in registry
        const callback = this.callbackRegistry.get(callbackId);

        if (!callback)
        {
            // Callback already executed or not registered (hot-reload or duplicate enqueue)
            // This is expected behavior - silently skip
            return;
        }

        // Remove callback from registry (one-time use)
        this.callbackRegistry.delete(callbackId);

        // Invoke JavaScript callback
        try
        {
            if (errorMessage && errorMessage.length > 0)
            {
                console.log(`EntityAPI: Callback ${callbackId} failed: ${errorMessage}`);
                callback(0);  // Signal failure with entityId = 0
            }
            else
            {
                callback(resultId);  // Success - pass entityId
            }
        }
        catch (error)
        {
            console.log(`EntityAPI: Error executing callback ${callbackId}: ${error.message}`);
        }
    }

    //----------------------------------------------------------------------------------------------------
    // Entity Creation and Management
    //----------------------------------------------------------------------------------------------------

    /**
     * Create a mesh entity with specified type and properties (async)
     * @param {string} meshType - Type of mesh: 'cube', 'sphere', 'grid', 'plane'
     * @param {Object} properties - Mesh properties
     * @param {Array<number>} properties.position - [x, y, z] position (X-forward, Y-left, Z-up)
     * @param {number} properties.scale - Uniform scale factor
     * @param {Array<number>} properties.color - [r, g, b, a] color (0-255)
     * @param {Function} callback - Callback function(entityId) called when mesh is created
     * @returns {number} callbackId - ID for tracking the callback
     */
    createMesh(meshType, properties, callback)
    {
        if (!this.cppEntity || !this.cppEntity.createMesh)
        {
            console.log('EntityAPI: ERROR - createMesh not available');
            if (callback) callback(0); // 0 = creation failed
            return 0;
        }

        // Validate parameters
        if (!meshType || typeof meshType !== 'string')
        {
            console.log('EntityAPI: ERROR - createMesh requires valid meshType string');
            if (callback) callback(0);
            return 0;
        }

        if (!properties || typeof properties !== 'object')
        {
            console.log('EntityAPI: ERROR - createMesh requires valid properties object');
            if (callback) callback(0);
            return 0;
        }

        // Default values
        const position = properties.position || [0, 0, 0];
        const scale = properties.scale || 1.0;
        const color = properties.color || [255, 255, 255, 255];

        // Validate array lengths
        if (!Array.isArray(position) || position.length !== 3)
        {
            console.log('EntityAPI: ERROR - position must be [x, y, z] array');
            if (callback) callback(0);
            return 0;
        }

        if (!Array.isArray(color) || color.length !== 4)
        {
            console.log('EntityAPI: ERROR - color must be [r, g, b, a] array');
            if (callback) callback(0);
            return 0;
        }

        // FLATTENED API: V8 binding cannot handle nested objects
        // Call C++ with individual primitive arguments instead
        // Signature: createMesh(meshType, posX, posY, posZ, scale, colorR, colorG, colorB, colorA, callback)

        console.log(`EntityAPI: Calling flattened createMesh API with 10 arguments`);
        console.log(`  meshType: ${meshType}`);
        console.log(`  position: [${position[0]}, ${position[1]}, ${position[2]}]`);
        console.log(`  scale: ${scale}`);
        console.log(`  color: [${color[0]}, ${color[1]}, ${color[2]}, ${color[3]}]`);

        // Call C++ interface with flattened arguments
        try
        {
            const result = this.cppEntity.createMesh(
                meshType,           // arg 0: string
                position[0],        // arg 1: number (posX)
                position[1],        // arg 2: number (posY)
                position[2],        // arg 3: number (posZ)
                scale,              // arg 4: number
                color[0],           // arg 5: number (colorR)
                color[1],           // arg 6: number (colorG)
                color[2],           // arg 7: number (colorB)
                color[3],           // arg 8: number (colorA)
                0                   // arg 9: 0 sentinel (Phase 2.4 - callback stored in JavaScript, V8 drops trailing null)
            );
            console.log(`EntityAPI: createMesh returned callbackId: ${result}`);

            // Phase 2.4: Store callback in registry instead of passing to C++
            if (callback && result !== 0)
            {
                this.callbackRegistry.set(result, callback);
                console.log(`EntityAPI: Registered callback ${result} for mesh creation`);
            }

            return result;
        }
        catch (error)
        {
            console.log('EntityAPI: ERROR - createMesh exception:', error);
            console.log('EntityAPI: ERROR - Exception message:', error.message);
            if (callback) callback(0);
            return 0;
        }
    }

    /**
     * Update entity position (absolute)
     * @param {number} entityId - Entity ID to update
     * @param {Array<number>} position - [x, y, z] new position (X-forward, Y-left, Z-up)
     */
    updatePosition(entityId, position)
    {
        if (!this.cppEntity || !this.cppEntity.updatePosition)
        {
            console.log('EntityAPI: ERROR - updatePosition not available');
            return;
        }

        if (!Array.isArray(position) || position.length !== 3)
        {
            console.log('EntityAPI: ERROR - position must be [x, y, z] array');
            return;
        }

        try
        {
            // FLATTENED API: V8 binding cannot handle nested objects
            // Call C++ with individual primitive arguments instead of object
            // Signature: updatePosition(entityId, posX, posY, posZ)
            this.cppEntity.updatePosition(entityId, position[0], position[1], position[2]);
        }
        catch (error)
        {
            console.log('EntityAPI: ERROR - updatePosition exception:', error);
        }
    }

    /**
     * Move entity by relative delta
     * @param {number} entityId - Entity ID to move
     * @param {Array<number>} delta - [dx, dy, dz] movement delta (X-forward, Y-left, Z-up)
     */
    moveBy(entityId, delta)
    {
        if (!this.cppEntity || !this.cppEntity.moveBy)
        {
            console.log('EntityAPI: ERROR - moveBy not available');
            return;
        }

        if (!Array.isArray(delta) || delta.length !== 3)
        {
            console.log('EntityAPI: ERROR - delta must be [dx, dy, dz] array');
            return;
        }

        try
        {
            // FLATTENED API: V8 binding cannot handle nested objects
            // Call C++ with individual primitive arguments instead of object
            // Signature: moveBy(entityId, dx, dy, dz)
            this.cppEntity.moveBy(entityId, delta[0], delta[1], delta[2]);
        }
        catch (error)
        {
            console.log('EntityAPI: ERROR - moveBy exception:', error);
        }
    }

    /**
     * Update entity orientation
     * @param {number} entityId - Entity ID to update
     * @param {Array<number>} orientation - [yaw, pitch, roll] in degrees
     */
    updateOrientation(entityId, orientation)
    {
        if (!this.cppEntity || !this.cppEntity.updateOrientation)
        {
            console.log('EntityAPI: ERROR - updateOrientation not available');
            return;
        }

        if (!Array.isArray(orientation) || orientation.length !== 3)
        {
            console.log('EntityAPI: ERROR - orientation must be [yaw, pitch, roll] array');
            return;
        }

        try
        {
            // FLATTENED API: V8 binding cannot handle nested objects
            // Call C++ with individual primitive arguments instead of object
            // Signature: updateOrientation(entityId, yaw, pitch, roll)
            this.cppEntity.updateOrientation(entityId, orientation[0], orientation[1], orientation[2]);
        }
        catch (error)
        {
            console.log('EntityAPI: ERROR - updateOrientation exception:', error);
        }
    }

    /**
     * Update entity color
     * @param {number} entityId - Entity ID to update
     * @param {Array<number>} color - [r, g, b, a] color (0-255)
     */
    updateColor(entityId, color)
    {
        if (!this.cppEntity || !this.cppEntity.updateColor)
        {
            console.log('EntityAPI: ERROR - updateColor not available');
            return;
        }

        if (!Array.isArray(color) || color.length !== 4)
        {
            console.log('EntityAPI: ERROR - color must be [r, g, b, a] array');
            return;
        }

        try
        {
            // FLATTENED API: V8 binding cannot handle nested objects
            // Call C++ with individual primitive arguments instead of object
            // Signature: updateColor(entityId, r, g, b, a)
            this.cppEntity.updateColor(entityId, color[0], color[1], color[2], color[3]);
        }
        catch (error)
        {
            console.log('EntityAPI: ERROR - updateColor exception:', error);
        }
    }

    /**
     * Destroy an entity
     * @param {number} entityId - Entity ID to destroy
     */
    destroyEntity(entityId)
    {
        if (!this.cppEntity || !this.cppEntity.destroy)
        {
            console.log('EntityAPI: ERROR - destroy method not available (C++ method is named destroy not destroyEntity)');
            return;
        }

        try
        {
            this.cppEntity.destroy(entityId);
        }
        catch (error)
        {
            console.log('EntityAPI: ERROR - destroyEntity exception:', error);
        }
    }

    //----------------------------------------------------------------------------------------------------
    // GenericCommand-based Entity Creation (Task 8.3 Migration)
    //----------------------------------------------------------------------------------------------------

    /**
     * Create a mesh entity via GenericCommand pipeline (Task 8.3)
     *
     * Uses CommandQueue.submit("create_mesh", ...) instead of direct C++ ScriptInterface call.
     * Same parameters and callback signature as createMesh() for drop-in replacement.
     *
     * @param {string} meshType - Type of mesh: 'cube', 'sphere', 'grid', 'plane'
     * @param {Object} properties - Mesh properties
     * @param {Array<number>} properties.position - [x, y, z] position
     * @param {number} properties.scale - Uniform scale factor
     * @param {Array<number>} properties.color - [r, g, b, a] color (0-255)
     * @param {Function} callback - Callback function(entityId) called when mesh is created
     * @returns {number} callbackId (0 if submission failed)
     */
    createMeshViaCommand(meshType, properties, callback)
    {
        // Get CommandQueue singleton
        const commandQueue = globalThis.CommandQueueAPI;
        if (!commandQueue || !commandQueue.isAvailable())
        {
            console.log('EntityAPI: ERROR - createMeshViaCommand requires CommandQueue');
            if (callback) callback(0);
            return 0;
        }

        if (!meshType || typeof meshType !== 'string')
        {
            console.log('EntityAPI: ERROR - createMeshViaCommand requires valid meshType string');
            if (callback) callback(0);
            return 0;
        }

        const position = (properties && properties.position) || [0, 0, 0];
        const scale    = (properties && properties.scale) || 1.0;
        const color    = (properties && properties.color) || [255, 255, 255, 255];

        // Submit via GenericCommand pipeline
        const callbackId = commandQueue.submit(
            'create_mesh',
            { meshType, position, scale, color },
            'entity-api',
            (result) =>
            {
                if (callback)
                {
                    // Adapt GenericCommand result format to EntityAPI callback format
                    // GenericCommand: { success, resultId, error }
                    // EntityAPI callback: (entityId) where 0 = failure
                    callback(result.success ? result.resultId : 0);
                }
            }
        );

        return callbackId;
    }

    //----------------------------------------------------------------------------------------------------
    // Utility Methods
    //----------------------------------------------------------------------------------------------------

    /**
     * Check if C++ entity interface is available
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
                createMesh: typeof this.cppEntity.createMesh === 'function',
                updatePosition: typeof this.cppEntity.updatePosition === 'function',
                moveBy: typeof this.cppEntity.moveBy === 'function',
                updateOrientation: typeof this.cppEntity.updateOrientation === 'function',
                updateColor: typeof this.cppEntity.updateColor === 'function',
                destroyEntity: typeof this.cppEntity.destroy === 'function'
            } : null
        };
    }
}

// Export for ES6 module system
export default EntityAPI;

// Export to globalThis for hot-reload detection
globalThis.EntityAPI = EntityAPI;

console.log('EntityAPI: High-level entity wrapper loaded (Phase 2 Interface Layer)');
