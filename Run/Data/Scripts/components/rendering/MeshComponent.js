//----------------------------------------------------------------------------------------------------
// MeshComponent.js
// Mesh/geometry rendering component (migrated from PropEntity)
//----------------------------------------------------------------------------------------------------

import {Component} from '../../core/Component.js';
import {RendererInterface} from '../../core/interfaces/RendererInterface.js';

/**
 * MeshComponent - Manages geometry and rendering for GameObjects
 *
 * Migrated from PropEntity.js geometry and rendering logic.
 * Handles vertex array creation, model constants, and rendering.
 *
 * Features:
 * - Geometry creation (cube, sphere, grid)
 * - Vertex array management
 * - Model constants (position, orientation, color)
 * - Rendering integration with RendererSystem
 *
 * Requires:
 * - GameObject with position, orientation properties
 * - RendererInterface (wrapper for C++ renderer interface)
 * - RendererSystem instance for geometry creation
 *
 * Usage:
 * ```javascript
 * const mesh = new MeshComponent(rendererSystem, 'cube', {r: 255, g: 0, b: 0, a: 255});
 * prop.addComponent(mesh);
 * ```
 */
export class MeshComponent extends Component
{
    /**
     * @param {RendererSystem} rendererSystem - Renderer system for geometry creation
     * @param {string} meshType - Geometry type ('cube', 'sphere', 'grid')
     * @param {Object} color - Initial color {r, g, b, a} (default: white)
     */
    constructor(rendererSystem, meshType, color = {r: 255, g: 255, b: 255, a: 255})
    {
        super('mesh');

        this.rendererSystem = rendererSystem;
        this.meshType = meshType;
        this.color = color;
        this.vertexArrayHandle = null;
        this.rendererInterface = new RendererInterface();

        console.log(`MeshComponent: Created with meshType=${meshType}`);
    }

    /**
     * Initialize component (called when attached to GameObject)
     * Creates geometry based on mesh type
     * @param {GameObject} gameObject
     */
    initialize(gameObject)
    {
        super.initialize(gameObject);

        // Ensure GameObject has required properties
        if (!this.gameObject.position)
        {
            this.gameObject.position = {x: 0, y: 0, z: 0};
        }
        if (!this.gameObject.orientation)
        {
            this.gameObject.orientation = {yaw: 0, pitch: 0, roll: 0};
        }

        // Create geometry
        this.createGeometry();

        console.log('MeshComponent: Initialized for GameObject');
    }

    /**
     * Create geometry based on mesh type (from PropEntity.initializeGeometry)
     */
    createGeometry()
    {
        if (!this.rendererInterface.isAvailable())
        {
            console.error('MeshComponent: RendererInterface not available!');
            return;
        }

        switch (this.meshType)
        {
            case 'cube':
                this.vertexArrayHandle = this.rendererSystem.createCubeVertexArray(1.0);
                console.log('MeshComponent: Created cube vertex array');
                break;

            case 'sphere':
                this.vertexArrayHandle = this.rendererSystem.createSphereVertexArray(0.5, 32, 16);
                console.log('MeshComponent: Created sphere vertex array');
                break;

            case 'grid':
                this.vertexArrayHandle = this.rendererSystem.createGridVertexArray(100.0);
                console.log('MeshComponent: Created grid vertex array');
                break;

            default:
                console.error(`MeshComponent: Unknown mesh type '${this.meshType}'`);
                break;
        }
    }

    /**
     * Render the mesh (from PropEntity.render)
     * Called by GameObject render system
     */
    render()
    {
        if (!this.vertexArrayHandle)
        {
            return;
        }

        if (!this.rendererInterface.isAvailable())
        {
            console.error('MeshComponent: RendererInterface not available!');
            return;
        }

        // Set model constants (position + orientation + color)
        this.rendererInterface.setModelConstants(
            this.gameObject.position.x,
            this.gameObject.position.y,
            this.gameObject.position.z,
            this.gameObject.orientation.yaw,
            this.gameObject.orientation.pitch,
            this.gameObject.orientation.roll,
            this.color.r,
            this.color.g,
            this.color.b,
            this.color.a
        );

        // Set render state (matching C++ Prop::Render())
        this.rendererSystem.setDefaultRenderState();

        // Draw the mesh
        this.rendererInterface.drawVertexArray(this.vertexArrayHandle);
    }

    /**
     * Set mesh color (for behavior-driven color changes)
     * @param {Object} color - Color {r, g, b, a}
     */
    setColor(color)
    {
        this.color = color;
    }

    /**
     * Get current mesh color
     * @returns {Object} Color {r, g, b, a}
     */
    getColor()
    {
        return this.color;
    }

    /**
     * Cleanup - Destroy vertex array when component is destroyed
     */
    destroy()
    {
        if (this.vertexArrayHandle)
        {
            // TODO: Add destroyVertexArray to RendererScriptInterface when implemented
            // renderer.destroyVertexArray(this.vertexArrayHandle);
            this.vertexArrayHandle = null;
            console.log('MeshComponent: Vertex array destroyed');
        }

        super.destroy();
    }

    /**
     * Get component status for debugging
     */
    getStatus()
    {
        return {
            ...super.getStatus(),
            meshType: this.meshType,
            hasVertexArray: this.vertexArrayHandle !== null,
            color: this.color
        };
    }
}

// Export for ES6 module system
export default MeshComponent;

// Export to globalThis for hot-reload detection
globalThis.MeshComponent = MeshComponent;

console.log('MeshComponent: Loaded (Phase 4 - Prop Migration)');
