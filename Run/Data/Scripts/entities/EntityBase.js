//----------------------------------------------------------------------------------------------------
// EntityBase.js - Base Entity Class (Matches C++ Entity structure)
//----------------------------------------------------------------------------------------------------

/**
 * EntityBase - Base class for all game entities
 *
 * Matches C++ Entity.hpp structure:
 * - m_position: Vec3
 * - m_orientation: EulerAngles
 * - m_velocity: Vec3
 * - m_angularVelocity: EulerAngles
 *
 * Note: This is NOT the EntitySystem. This is the base entity class.
 */
export class EntityBase {
    constructor() {
        // Transform (matches C++ Entity)
        this.m_position = { x: 0, y: 0, z: 0 };
        this.m_orientation = { yaw: 0, pitch: 0, roll: 0 };

        // Physics (matches C++ Entity)
        this.m_velocity = { x: 0, y: 0, z: 0 };
        this.m_angularVelocity = { yaw: 0, pitch: 0, roll: 0 };
    }

    /**
     * Update entity (called every frame)
     * Virtual method - override in derived classes
     */
    update(deltaSeconds) {
        // Update position based on velocity
        this.m_position.x += this.m_velocity.x * deltaSeconds;
        this.m_position.y += this.m_velocity.y * deltaSeconds;
        this.m_position.z += this.m_velocity.z * deltaSeconds;

        // Update orientation based on angular velocity
        this.m_orientation.yaw += this.m_angularVelocity.yaw * deltaSeconds;
        this.m_orientation.pitch += this.m_angularVelocity.pitch * deltaSeconds;
        this.m_orientation.roll += this.m_angularVelocity.roll * deltaSeconds;
    }

    /**
     * Render entity (called every frame)
     * Virtual method - override in derived classes
     */
    render() {
        // Override in derived classes
    }

    /**
     * Get model-to-world transform matrix
     * Simplified version - just returns position for now
     */
    getModelToWorldTransform() {
        return this.m_position;
    }
}

console.log('EntityBase: Module loaded');
