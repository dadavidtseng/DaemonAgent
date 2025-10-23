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
        this.cppEntity = globalThis.entity; // C++ entity interface reference

        if (!this.cppEntity)
        {
            console.log('EntityAPI: C++ entity interface (globalThis.entity) not available');
        }
        else
        {
            console.log(`EntityAPI: Successfully connected to C++ high-level entity interface (Phase 2, version ${EntityAPI.version})`);
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
                callback            // arg 9: function
            );
            console.log(`EntityAPI: createMesh returned callbackId: ${result}`);
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
        if (!this.cppEntity || !this.cppEntity.destroyEntity)
        {
            console.log('EntityAPI: ERROR - destroyEntity not available');
            return;
        }

        try
        {
            this.cppEntity.destroyEntity(entityId);
        }
        catch (error)
        {
            console.log('EntityAPI: ERROR - destroyEntity exception:', error);
        }
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
                destroyEntity: typeof this.cppEntity.destroyEntity === 'function'
            } : null
        };
    }
}

// Export for ES6 module system
export default EntityAPI;

// Export to globalThis for hot-reload detection
globalThis.EntityAPI = EntityAPI;

console.log('EntityAPI: High-level entity wrapper loaded (Phase 2 Interface Layer)');
