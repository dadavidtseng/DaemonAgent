// DebugRenderSystem.js
// JavaScript wrapper for DebugRenderSystemScriptInterface
// Provides clean JavaScript API for C++ debug rendering functionality

import { Subsystem } from '../core/Subsystem.js';
import { DebugRenderInterface } from '../interfaces/DebugRenderInterface.js';

/**
 * DebugRenderSystem - JavaScript wrapper for debug visualization
 *
 * Provides debug rendering functionality including:
 * - Control (show/hide/clear)
 * - Output (render world/screen space)
 * - Geometry (points, lines, spheres, text, etc.)
 *
 * Usage:
 *   const debug = new DebugRenderSystem();
 *   debug.addWorldLine(0, 0, 0, 1, 1, 1, 0.02, 0, 255, 0, 0, 255, "USE_DEPTH");
 *   debug.renderWorld(camera);
 */
export class DebugRenderSystem extends Subsystem
{
    constructor()
    {
        super('debugRenderSystem', 95, { enabled: true }); // Priority 95, before renderer (100)

        this.isInitialized = false;
        this.debugRenderInterface = new DebugRenderInterface();

        console.log('DebugRenderSystem: Module loaded (Phase 4 ES6)');
        console.log(`DebugRenderSystem: this.id = ${this.id}, this.priority = ${this.priority}`);
        console.log(`DebugRenderSystem: this.update type = ${typeof this.update}, this.render type = ${typeof this.render}`);
        this.initialize();
    }

    /**
     * Initialize the debug render system and verify C++ debugRenderInterface availability
     */
    initialize()
    {
        try
        {
            // Check if C++ debugRenderInterface is available through wrapper
            if (this.debugRenderInterface.isAvailable())
            {
                console.log('DebugRenderSystem: C++ debugRenderInterface available');
                this.isInitialized = true;
            }
            else
            {
                console.log('DebugRenderSystem: C++ debugRenderInterface NOT available!');
                this.isInitialized = false;
            }
        }
        catch (error)
        {
            console.log('DebugRenderSystem: Initialization failed:', error);
            this.isInitialized = false;
        }
    }

    /**
     * Update method - called every frame
     * @param {number} gameDelta - Game time delta
     * @param {number} systemDelta - System time delta
     */
    update(gameDelta, systemDelta)
    {
        // Debug render system has no per-frame update logic
        // All debug rendering operations are done through methods directly
    }

    /**
     * Render method - called every frame during render phase
     */
    render()
    {
        // Debug render system has no automatic rendering logic
        // Rendering is done explicitly through renderWorld/renderScreen methods
    }

    /**
     * Get system status for debugging
     */
    getSystemStatus()
    {
        return {
            id: this.id,
            priority: this.priority,
            enabled: this.enabled,
            isInitialized: this.isInitialized
        };
    }

    // === CONTROL METHODS ===

    /**
     * Make debug rendering visible
     */
    setVisible()
    {
        return this.debugRenderInterface.setVisible();
    }

    /**
     * Hide debug rendering
     */
    setHidden()
    {
        return this.debugRenderInterface.setHidden();
    }

    /**
     * Clear all debug rendering objects
     */
    clear()
    {
        return this.debugRenderInterface.clear();
    }

    // === OUTPUT METHODS ===

    /**
     * Begin debug rendering frame
     */
    beginFrame()
    {
        return this.debugRenderInterface.beginFrame();
    }

    /**
     * Render world-space debug objects with specified camera
     * @param {number} cameraId - Camera ID (will be looked up to get camera handle)
     */
    renderWorld(cameraId)
    {
        // Look up camera handle from camera ID
        // getCameraHandle is exposed through globalThis.entity (EntityScriptInterface)
        if (!globalThis.entity || !globalThis.entity.getCameraHandle)
        {
            console.log('DebugRenderSystem.renderWorld: entity.getCameraHandle not available');
            return;
        }

        const cameraHandle = globalThis.entity.getCameraHandle(cameraId);
        if (cameraHandle === 0)
        {
            console.log(`DebugRenderSystem.renderWorld: Camera ${cameraId} not found`);
            return;
        }

        return this.debugRenderInterface.renderWorld(cameraHandle);
    }

    /**
     * Render screen-space debug objects with specified camera
     * @param {number} cameraId - Camera ID (will be looked up to get camera handle)
     */
    renderScreen(cameraId)
    {
        // Look up camera handle from camera ID
        // getCameraHandle is exposed through globalThis.entity (EntityScriptInterface)
        if (!globalThis.entity || !globalThis.entity.getCameraHandle)
        {
            console.log('DebugRenderSystem.renderScreen: entity.getCameraHandle not available');
            return;
        }

        const cameraHandle = globalThis.entity.getCameraHandle(cameraId);
        if (cameraHandle === 0)
        {
            console.log(`DebugRenderSystem.renderScreen: Camera ${cameraId} not found`);
            return;
        }

        return this.debugRenderInterface.renderScreen(cameraHandle);
    }

    /**
     * End debug rendering frame
     */
    endFrame()
    {
        return this.debugRenderInterface.endFrame();
    }

    // === GEOMETRY METHODS - WORLD SPACE ===

    /**
     * Add debug point in world space
     * @param {number} x - X coordinate
     * @param {number} y - Y coordinate
     * @param {number} z - Z coordinate
     * @param {number} radius - Point radius
     * @param {number} duration - Display duration (0 = one frame)
     * @param {number} r - Red (0-255)
     * @param {number} g - Green (0-255)
     * @param {number} b - Blue (0-255)
     * @param {number} a - Alpha (0-255)
     * @param {string} mode - Render mode: "ALWAYS", "USE_DEPTH", "X_RAY"
     */
    addWorldPoint(x, y, z, radius, duration, r, g, b, a, mode = "USE_DEPTH")
    {
        return this.debugRenderInterface.addWorldPoint(x, y, z, radius, duration, r, g, b, a, mode);
    }

    /**
     * Add debug line in world space
     * @param {number} x1 - Start X
     * @param {number} y1 - Start Y
     * @param {number} z1 - Start Z
     * @param {number} x2 - End X
     * @param {number} y2 - End Y
     * @param {number} z2 - End Z
     * @param {number} radius - Line radius
     * @param {number} duration - Display duration (0 = one frame)
     * @param {number} r - Red (0-255)
     * @param {number} g - Green (0-255)
     * @param {number} b - Blue (0-255)
     * @param {number} a - Alpha (0-255)
     * @param {string} mode - Render mode: "ALWAYS", "USE_DEPTH", "X_RAY"
     */
    addWorldLine(x1, y1, z1, x2, y2, z2, radius, duration, r, g, b, a, mode = "USE_DEPTH")
    {
        return this.debugRenderInterface.addWorldLine(x1, y1, z1, x2, y2, z2, radius, duration, r, g, b, a, mode);
    }

    /**
     * Add debug cylinder in world space
     * @param {number} baseX - Base X
     * @param {number} baseY - Base Y
     * @param {number} baseZ - Base Z
     * @param {number} topX - Top X
     * @param {number} topY - Top Y
     * @param {number} topZ - Top Z
     * @param {number} radius - Cylinder radius
     * @param {number} duration - Display duration
     * @param {boolean} isWireframe - Wireframe mode
     * @param {number} r - Red (0-255)
     * @param {number} g - Green (0-255)
     * @param {number} b - Blue (0-255)
     * @param {number} a - Alpha (0-255)
     * @param {string} mode - Render mode
     */
    addWorldCylinder(baseX, baseY, baseZ, topX, topY, topZ, radius, duration, isWireframe, r, g, b, a, mode = "USE_DEPTH")
    {
        return this.debugRenderInterface.addWorldCylinder(baseX, baseY, baseZ, topX, topY, topZ, radius, duration, isWireframe, r, g, b, a, mode);
    }

    /**
     * Add debug wire sphere in world space
     * @param {number} x - Center X
     * @param {number} y - Center Y
     * @param {number} z - Center Z
     * @param {number} radius - Sphere radius
     * @param {number} duration - Display duration
     * @param {number} r - Red (0-255)
     * @param {number} g - Green (0-255)
     * @param {number} b - Blue (0-255)
     * @param {number} a - Alpha (0-255)
     * @param {string} mode - Render mode
     */
    addWorldWireSphere(x, y, z, radius, duration, r, g, b, a, mode = "USE_DEPTH")
    {
        return this.debugRenderInterface.addWorldWireSphere(x, y, z, radius, duration, r, g, b, a, mode);
    }

    /**
     * Add debug arrow in world space
     * @param {number} x1 - Start X
     * @param {number} y1 - Start Y
     * @param {number} z1 - Start Z
     * @param {number} x2 - End X
     * @param {number} y2 - End Y
     * @param {number} z2 - End Z
     * @param {number} radius - Arrow radius
     * @param {number} duration - Display duration
     * @param {number} r - Red (0-255)
     * @param {number} g - Green (0-255)
     * @param {number} b - Blue (0-255)
     * @param {number} a - Alpha (0-255)
     * @param {string} mode - Render mode
     */
    addWorldArrow(x1, y1, z1, x2, y2, z2, radius, duration, r, g, b, a, mode = "USE_DEPTH")
    {
        return this.debugRenderInterface.addWorldArrow(x1, y1, z1, x2, y2, z2, radius, duration, r, g, b, a, mode);
    }

    /**
     * Add debug text in world space
     * @param {string} text - Text to display
     * @param {Array<number>} transform - 16-element transform matrix
     * @param {number} textHeight - Text height
     * @param {number} alignX - X alignment
     * @param {number} alignY - Y alignment
     * @param {number} duration - Display duration
     * @param {number} r - Red (0-255)
     * @param {number} g - Green (0-255)
     * @param {number} b - Blue (0-255)
     * @param {number} a - Alpha (0-255)
     * @param {string} mode - Render mode
     */
    addWorldText(text, transform, textHeight, alignX, alignY, duration, r, g, b, a, mode = "USE_DEPTH")
    {
        return this.debugRenderInterface.addWorldText(text, transform, textHeight, alignX, alignY, duration, r, g, b, a, mode);
    }

    /**
     * Add billboard text in world space
     * @param {string} text - Text to display
     * @param {number} x - Position X
     * @param {number} y - Position Y
     * @param {number} z - Position Z
     * @param {number} textHeight - Text height
     * @param {number} alignX - X alignment
     * @param {number} alignY - Y alignment
     * @param {number} duration - Display duration
     * @param {number} r - Red (0-255)
     * @param {number} g - Green (0-255)
     * @param {number} b - Blue (0-255)
     * @param {number} a - Alpha (0-255)
     * @param {string} mode - Render mode
     */
    addBillboardText(text, x, y, z, textHeight, alignX, alignY, duration, r, g, b, a, mode = "USE_DEPTH")
    {
        return this.debugRenderInterface.addBillboardText(text, x, y, z, textHeight, alignX, alignY, duration, r, g, b, a, mode);
    }

    /**
     * Add debug coordinate basis in world space
     * @param {Array<number>} transform - 16-element transform matrix
     * @param {number} duration - Display duration
     * @param {string} mode - Render mode
     */
    addWorldBasis(transform, duration, mode = "USE_DEPTH")
    {
        return this.debugRenderInterface.addWorldBasis(transform, duration, mode);
    }

    // === GEOMETRY METHODS - SCREEN SPACE ===

    /**
     * Add debug text in screen space
     * @param {string} text - Text to display
     * @param {number} x - Screen X coordinate
     * @param {number} y - Screen Y coordinate
     * @param {number} size - Text size
     * @param {number} alignX - X alignment
     * @param {number} alignY - Y alignment
     * @param {number} duration - Display duration
     * @param {number} r - Red (0-255)
     * @param {number} g - Green (0-255)
     * @param {number} b - Blue (0-255)
     * @param {number} a - Alpha (0-255)
     */
    addScreenText(text, x, y, size, alignX, alignY, duration, r, g, b, a)
    {
        return this.debugRenderInterface.addScreenText(text, x, y, size, alignX, alignY, duration, r, g, b, a);
    }

    /**
     * Add debug message
     * @param {string} text - Message text
     * @param {number} duration - Display duration
     * @param {number} r - Red (0-255)
     * @param {number} g - Green (0-255)
     * @param {number} b - Blue (0-255)
     * @param {number} a - Alpha (0-255)
     */
    addMessage(text, duration, r, g, b, a)
    {
        return this.debugRenderInterface.addMessage(text, duration, r, g, b, a);
    }
}

// Export to globalThis for hot-reload detection
globalThis.DebugRenderSystem = DebugRenderSystem;

console.log('DebugRenderSystem: Component loaded (JavaScript wrapper for DebugRenderSystemScriptInterface)');
