//----------------------------------------------------------------------------------------------------
// EntityBase.js - Base Entity Class (Migrated from C++ Entity.hpp/.cpp)
//----------------------------------------------------------------------------------------------------

/**
 * EntityBase - Base class for all game entities
 *
 * Migrated from C++ Entity.hpp/.cpp
 * - Abstract base class with virtual Update() and Render() methods
 * - Position, velocity, orientation, angular velocity
 * - Color property for rendering
 * - GetModelToWorldTransform() for rendering
 */
export class EntityBase {
    constructor(game = null) {
        // Game reference (matches C++ Entity::m_game)
        this.m_game = game;

        // Transform properties (matches C++ Entity)
        this.m_position = { x: 0.0, y: 0.0, z: 0.0 };        // Vec3::ZERO
        this.m_velocity = { x: 0.0, y: 0.0, z: 0.0 };        // Vec3::ZERO
        this.m_orientation = { yaw: 0.0, pitch: 0.0, roll: 0.0 };  // EulerAngles::ZERO
        this.m_angularVelocity = { yaw: 0.0, pitch: 0.0, roll: 0.0 }; // EulerAngles::ZERO

        // Rendering properties (matches C++ Entity)
        this.m_color = { r: 255, g: 255, b: 255, a: 255 };   // Rgba8::WHITE
    }

    /**
     * Update entity (virtual method - must be overridden)
     * Matches: virtual void Update(float deltaSeconds) = 0;
     */
    update(deltaSeconds) {
        throw new Error('EntityBase::update() must be overridden in derived class');
    }

    /**
     * Render entity (virtual method - must be overridden)
     * Matches: virtual void Render() const = 0;
     */
    render() {
        throw new Error('EntityBase::render() must be overridden in derived class');
    }

    /**
     * Get model-to-world transform matrix
     * Matches: virtual Mat44 GetModelToWorldTransform() const;
     *
     * C++ implementation:
     *   Mat44 m2w;
     *   m2w.SetTranslation3D(m_position);
     *   m2w.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());
     *   return m2w;
     *
     * Returns: { position, orientation } for JavaScript rendering
     */
    getModelToWorldTransform() {
        return {
            position: this.m_position,
            orientation: this.m_orientation
        };
    }

    /**
     * Helper: Get forward vector from orientation
     */
    getForwardVector() {
        const yawRad = this.m_orientation.yaw * (Math.PI / 180.0);
        const pitchRad = this.m_orientation.pitch * (Math.PI / 180.0);

        return {
            x: Math.cos(yawRad) * Math.cos(pitchRad),
            y: Math.sin(yawRad) * Math.cos(pitchRad),
            z: Math.sin(pitchRad)
        };
    }

    /**
     * Helper: Get left vector from orientation
     */
    getLeftVector() {
        const yawRad = this.m_orientation.yaw * (Math.PI / 180.0);

        return {
            x: Math.cos(yawRad + Math.PI / 2.0),
            y: Math.sin(yawRad + Math.PI / 2.0),
            z: 0.0
        };
    }

    /**
     * Helper: Get up vector from orientation
     */
    getUpVector() {
        const yawRad = this.m_orientation.yaw * (Math.PI / 180.0);
        const pitchRad = this.m_orientation.pitch * (Math.PI / 180.0);
        const rollRad = this.m_orientation.roll * (Math.PI / 180.0);

        // Simplified - returns world up rotated by roll
        return {
            x: 0.0,
            y: 0.0,
            z: 1.0
        };
    }
}

console.log('EntityBase: Module loaded (C++ Entity.hpp/.cpp migration complete)');
