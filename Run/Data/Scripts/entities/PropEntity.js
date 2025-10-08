//----------------------------------------------------------------------------------------------------
// PropEntity.js - JavaScript Prop Entity (Matches C++ Prop behavior exactly)
//----------------------------------------------------------------------------------------------------
import {EntityBase} from './EntityBase.js';

/**
 * PropEntity - Recreates exact C++ Prop behavior with JavaScript rendering
 *
 * Extends EntityBase (like C++ Prop extends Entity)
 * Manages a single prop with geometry and rendering
 *
 * C++ Prop behaviors (when managing multiple props in JSGame):
 * - Prop[0]: Cube at (2, 2, 0) - pitch+roll += 30°/s
 * - Prop[1]: Cube at (-2, -2, 0) - pulsing color (sin wave)
 * - Prop[2]: Sphere at (10, -5, 1) - yaw += 45°/s
 * - Prop[3]: Grid at (0, 0, 0) - static
 *
 * This entity represents a single prop with geometry and custom behavior.
 * JSGame creates and manages multiple PropEntity instances (like C++ Game's std::vector<Prop*>).
 */
export class PropEntity extends EntityBase
{
    constructor(game, propType, position, rendererSystem)
    {
        super(game);

        this.propType = propType; // 'cube', 'sphere', 'grid'
        this.rendererSystem = rendererSystem;

        // Set position from constructor parameter
        this.m_position = position || {x: 0.0, y: 0.0, z: 0.0};
        this.m_color = {r: 255, g: 255, b: 255, a: 255};

        // Geometry handle for rendering
        this.vertexArrayHandle = null;

        // Behavior-specific data
        this.behaviorType = null; // 'rotate-pitch-roll', 'pulse-color', 'rotate-yaw', 'static'
        this.startTime = Date.now() / 1000.0;

        // Create geometry based on type
        this.initializeGeometry();

        console.log(`PropEntity: Created ${propType} at (${this.m_position.x}, ${this.m_position.y}, ${this.m_position.z})`);
    }

    /**
     * Initialize geometry based on prop type (matches C++ Prop::InitializeLocalVerts*)
     */
    initializeGeometry()
    {
        if (typeof renderer === 'undefined')
        {
            console.log('PropEntity: renderer not available!');
            return;
        }

        switch (this.propType)
        {
            case 'cube':
                this.vertexArrayHandle = this.rendererSystem.createCubeVertexArray(1.0);
                break;
            case 'sphere':
                this.vertexArrayHandle = this.rendererSystem.createSphereVertexArray(0.5, 32, 16);
                break;
            case 'grid':
                this.vertexArrayHandle = this.rendererSystem.createGridVertexArray(100.0);
                break;
            default:
                console.log(`PropEntity: Unknown prop type '${this.propType}'`);
                break;
        }
    }

    /**
     * Set behavior type for this prop
     */
    setBehavior(behaviorType)
    {
        this.behaviorType = behaviorType;
    }

    /**
     * Update - Override EntityBase::update()
     * Matches C++ Prop::Update(float deltaSeconds)
     */
    update(deltaSeconds)
    {
        // Apply behavior-specific updates
        switch (this.behaviorType)
        {
            case 'rotate-pitch-roll':
                // Prop 0: Rotate pitch and roll (30°/s each)
                this.m_orientation.pitch += 30.0 * deltaSeconds;
                this.m_orientation.roll += 30.0 * deltaSeconds;
                break;

            case 'pulse-color':
                // Prop 1: Pulsing color (sin wave)
                // C++ code: float const colorValue = (sinf(time) + 1.0f) * 0.5f * 255.0f;
                const currentTime = Date.now() / 1000.0;
                const elapsedTime = currentTime - this.startTime;
                const colorValue = (Math.sin(elapsedTime) + 1.0) * 0.5 * 255.0;
                this.m_color.r = Math.floor(colorValue);
                this.m_color.g = Math.floor(colorValue);
                this.m_color.b = Math.floor(colorValue);
                break;

            case 'rotate-yaw':
                // Prop 2: Rotate yaw (45°/s)
                this.m_orientation.yaw += 45.0 * deltaSeconds;
                break;

            case 'static':
                // Prop 3: Grid (no update, static)
                break;

            default:
                // No behavior - static prop
                break;
        }
    }

    /**
     * Render - Override EntityBase::render()
     * Matches C++ Prop::Render() const
     */
    render()
    {
        if (!this.vertexArrayHandle)
        {
            console.log('PropEntity: vertexArrayHandle is null/undefined!');
            return;
        }

        if (typeof renderer === 'undefined')
        {
            console.log('PropEntity: renderer is undefined!');
            return;
        }

        // Set model constants (position + orientation + color)
        renderer.setModelConstants(
            this.m_position.x, this.m_position.y, this.m_position.z,
            this.m_orientation.yaw, this.m_orientation.pitch, this.m_orientation.roll,
            this.m_color.r, this.m_color.g, this.m_color.b, this.m_color.a
        );

        // Set render state (matching C++ Prop::Render())
        this.rendererSystem.setDefaultRenderState();

        // Draw the prop
        renderer.drawVertexArray(this.vertexArrayHandle);
    }

    /**
     * Cleanup - Destroy vertex array when entity is destroyed
     * Matches C++ Prop::~Prop()
     */
    destroy()
    {
        if (this.vertexArrayHandle)
        {
            // TODO: Add destroyVertexArray to RendererScriptInterface when implemented
            // renderer.destroyVertexArray(this.vertexArrayHandle);
            this.vertexArrayHandle = null;
            console.log('PropEntity: Vertex array destroyed');
        }
    }
}

console.log('PropEntity: Module loaded');
