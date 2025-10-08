//----------------------------------------------------------------------------------------------------
// PropEntity.js - JavaScript Prop Entity (Matches C++ Prop behavior exactly)
//----------------------------------------------------------------------------------------------------
import { EntityBase } from './EntityBase.js';
import { Subsystem } from '../core/Subsystem.js';

/**
 * PropEntity - Recreates exact C++ Prop behavior with JavaScript rendering
 *
 * C++ Prop behaviors:
 * - Prop[0]: Cube at (2, 2, 0) - pitch+roll += 30°/s
 * - Prop[1]: Cube at (-2, -2, 0) - pulsing color (sin wave)
 * - Prop[2]: Sphere at (10, -5, 1) - yaw += 45°/s
 * - Prop[3]: Grid at (0, 0, 0) - static
 *
 * This system creates 4 props matching C++ and renders them using RendererScriptInterface.
 * Geometry types: Cube, Sphere, and Grid (matching C++ Prop::InitializeLocalVerts*)
 */
export class PropEntity extends Subsystem {
    constructor(engine, rendererSystem) {
        super('propEntity', 15, {enabled: true}); // Priority 15, ENABLED
        this.engine = engine;
        this.rendererSystem = rendererSystem;

        this.data = {
            props: [],
            vertexArrayHandles: [],
            initialized: false,
            startTime: Date.now() / 1000.0
        };
    }

    /**
     * Update all props
     */
    update(gameDelta, systemDelta) {
        if (!this.data.initialized) {
            this.createProps();
            this.data.initialized = true;
        }

        // Update all props
        for (const prop of this.data.props) {
            prop.update(gameDelta);
        }

        // Update specific prop behaviors (matching C++ exactly)
        this.updatePropBehaviors(gameDelta);
    }

    /**
     * Render all props (called by JSEngine render phase)
     */
    render() {
        if (!this.data.initialized) {
            return;
        }
        if (typeof renderer === 'undefined') {
            console.error('PropEntity: renderer is undefined!');
            return;
        }

        // Render each prop
        for (let i = 0; i < this.data.props.length; i++) {
            this.renderProp(this.data.props[i], this.data.vertexArrayHandles[i]);
        }
    }

    /**
     * Create the 4 C++ props
     */
    createProps() {
        if (typeof renderer === 'undefined') {
            console.error('PropEntity: renderer not available!');
            return;
        }

        // Prop 0: Rotating cube (pitch+roll += 30°/s)
        const prop0 = new EntityBase();
        prop0.m_position = { x: 2, y: 2, z: 0 };
        prop0.m_color = { r: 255, g: 255, b: 255, a: 255 };
        prop0.propIndex = 0;
        this.data.props.push(prop0);
        this.data.vertexArrayHandles.push(this.rendererSystem.createCubeVertexArray(1.0));

        // Prop 1: Pulsing color cube (sin wave)
        const prop1 = new EntityBase();
        prop1.m_position = { x: -2, y: -2, z: 0 };
        prop1.m_color = { r: 255, g: 255, b: 255, a: 255 };
        prop1.propIndex = 1;
        this.data.props.push(prop1);
        this.data.vertexArrayHandles.push(this.rendererSystem.createCubeVertexArray(1.0));

        // Prop 2: Rotating sphere (yaw += 45°/s)
        const prop2 = new EntityBase();
        prop2.m_position = { x: 10, y: -5, z: 1 };
        prop2.m_color = { r: 255, g: 255, b: 255, a: 255 };
        prop2.propIndex = 2;
        this.data.props.push(prop2);
        this.data.vertexArrayHandles.push(this.rendererSystem.createSphereVertexArray(0.5, 32, 16)); // radius=0.5, slices=32, stacks=16

        // Prop 3: Grid (static)
        const prop3 = new EntityBase();
        prop3.m_position = { x: 0, y: 0, z: 0 };
        prop3.m_color = { r: 255, g: 255, b: 255, a: 255 };
        prop3.propIndex = 3;
        this.data.props.push(prop3);
        this.data.vertexArrayHandles.push(this.rendererSystem.createGridVertexArray(100.0)); // gridLineLength=100
    }

    /**
     * Update prop behaviors to match C++ exactly
     */
    updatePropBehaviors(gameDelta) {
        if (this.data.props.length < 4) return;

        // Prop 0: Rotate pitch and roll (30°/s each)
        const prop0 = this.data.props[0];
        prop0.m_orientation.pitch += 30.0 * gameDelta;
        prop0.m_orientation.roll += 30.0 * gameDelta;

        // Prop 1: Pulsing color (sin wave)
        // C++ code: float const colorValue = (sinf(time) + 1.0f) * 0.5f * 255.0f;
        const prop1 = this.data.props[1];
        const currentTime = Date.now() / 1000.0;
        const elapsedTime = currentTime - this.data.startTime;
        const colorValue = (Math.sin(elapsedTime) + 1.0) * 0.5 * 255.0;
        prop1.m_color.r = Math.floor(colorValue);
        prop1.m_color.g = Math.floor(colorValue);
        prop1.m_color.b = Math.floor(colorValue);

        // Prop 2: Rotate yaw (45°/s)
        const prop2 = this.data.props[2];
        prop2.m_orientation.yaw += 45.0 * gameDelta;

        // Prop 3: Grid (no update, static)
    }

    /**
     * Render a single prop
     */
    renderProp(prop, vertexArrayHandle) {

        if (!vertexArrayHandle) {
            console.error('PropEntity: vertexArrayHandle is null/undefined!');
            return;
        }

        // Set model constants (position + orientation + color)
        const pos = prop.m_position;
        const ori = prop.m_orientation;
        const col = prop.m_color;

        renderer.setModelConstants(
            pos.x, pos.y, pos.z,           // Position
            ori.yaw, ori.pitch, ori.roll,  // Orientation
            col.r, col.g, col.b, col.a     // Color
        );

        // Set render state (matching C++ Prop::Render())
        this.rendererSystem.setDefaultRenderState();


        renderer.drawVertexArray(vertexArrayHandle);

    }

    /**
     * Get all props
     */
    getProps() {
        return this.data.props;
    }
}
