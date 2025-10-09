// DebugRenderSystem.js
// JavaScript wrapper for DebugRenderSystemScriptInterface
// Provides clean JavaScript API for C++ debug rendering functionality

/**
 * DebugRenderSystem - JavaScript wrapper for debug visualization
 *
 * Provides debug rendering functionality including:
 * - Control (show/hide/clear)
 * - Output (render world/screen space)
 * - Geometry (points, lines, spheres, text, etc.)
 *
 * Usage:
 *   const debug = new DebugRenderSystem(debugRenderHandle);
 *   debug.addWorldLine(0, 0, 0, 1, 1, 1, 0.02, 0, 255, 0, 0, 255, "USE_DEPTH");
 *   debug.renderWorld(camera);
 */
export class DebugRenderSystem
{
    constructor(debugRenderHandle)
    {
        if (!debugRenderHandle)
        {
            throw new Error('DebugRenderSystem: debugRenderHandle is required');
        }
        this.debugRenderHandle = debugRenderHandle;
    }

    // === CONTROL METHODS ===

    /**
     * Make debug rendering visible
     */
    setVisible()
    {
        return this.debugRenderHandle.setVisible();
    }

    /**
     * Hide debug rendering
     */
    setHidden()
    {
        return this.debugRenderHandle.setHidden();
    }

    /**
     * Clear all debug rendering objects
     */
    clear()
    {
        return this.debugRenderHandle.clear();
    }

    // === OUTPUT METHODS ===

    /**
     * Begin debug rendering frame
     */
    beginFrame()
    {
        return this.debugRenderHandle.beginFrame();
    }

    /**
     * Render world-space debug objects with specified camera
     * @param {number} cameraHandle - Camera handle (pointer as number)
     */
    renderWorld(cameraHandle)
    {
        return this.debugRenderHandle.renderWorld(cameraHandle);
    }

    /**
     * Render screen-space debug objects with specified camera
     * @param {number} cameraHandle - Camera handle (pointer as number)
     */
    renderScreen(cameraHandle)
    {
        return this.debugRenderHandle.renderScreen(cameraHandle);
    }

    /**
     * End debug rendering frame
     */
    endFrame()
    {
        return this.debugRenderHandle.endFrame();
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
        return this.debugRenderHandle.addWorldPoint(x, y, z, radius, duration, r, g, b, a, mode);
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
        return this.debugRenderHandle.addWorldLine(x1, y1, z1, x2, y2, z2, radius, duration, r, g, b, a, mode);
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
        return this.debugRenderHandle.addWorldCylinder(baseX, baseY, baseZ, topX, topY, topZ, radius, duration, isWireframe, r, g, b, a, mode);
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
        return this.debugRenderHandle.addWorldWireSphere(x, y, z, radius, duration, r, g, b, a, mode);
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
        return this.debugRenderHandle.addWorldArrow(x1, y1, z1, x2, y2, z2, radius, duration, r, g, b, a, mode);
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
        return this.debugRenderHandle.addWorldText(text, transform, textHeight, alignX, alignY, duration, r, g, b, a, mode);
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
        return this.debugRenderHandle.addBillboardText(text, x, y, z, textHeight, alignX, alignY, duration, r, g, b, a, mode);
    }

    /**
     * Add debug coordinate basis in world space
     * @param {Array<number>} transform - 16-element transform matrix
     * @param {number} duration - Display duration
     * @param {string} mode - Render mode
     */
    addWorldBasis(transform, duration, mode = "USE_DEPTH")
    {
        return this.debugRenderHandle.addWorldBasis(transform, duration, mode);
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
        return this.debugRenderHandle.addScreenText(text, x, y, size, alignX, alignY, duration, r, g, b, a);
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
        return this.debugRenderHandle.addMessage(text, duration, r, g, b, a);
    }
}

// Export to globalThis for hot-reload detection
globalThis.DebugRenderSystem = DebugRenderSystem;

console.log('DebugRenderSystem: Component loaded (JavaScript wrapper for DebugRenderSystemScriptInterface)');
