//----------------------------------------------------------------------------------------------------
// DebugRenderInterface.js
// Wrapper for C++ debug render interface (globalThis.debugRenderInterface)
// Part of the interface wrapper layer for C++/JavaScript bridge
//----------------------------------------------------------------------------------------------------

/**
 * DebugRenderInterface - Clean abstraction over C++ debug render system
 *
 * This wrapper provides a JavaScript-friendly interface to the C++ debug visualization system,
 * abstracting direct globalThis access and providing safe fallbacks.
 *
 * Design Principles:
 * - Single Responsibility: Only wraps C++ debug render interface
 * - Safe Fallbacks: Returns sensible defaults if C++ interface unavailable
 * - Type Safety: Clear return types and parameter documentation
 * - Testability: Can be mocked for unit testing
 *
 * C++ Interface Methods (exposed via globalThis.debugRenderInterface):
 * Control: setVisible(), setHidden(), clear(), clearAll(), beginFrame(), endFrame()
 * Output: renderWorld(cameraId), renderScreen(cameraId)
 * World Geometry: addWorldPoint, addWorldLine, addWorldCylinder, addWorldWireSphere,
 *                 addWorldArrow, addWorldText, addBillboardText, addWorldBasis
 * Screen Geometry: addScreenText, addMessage
 *
 * Usage Example:
 * ```javascript
 * const debugInterface = new DebugRenderInterface();
 * debugInterface.setVisible();
 * debugInterface.addWorldLine(0, 0, 0, 1, 1, 1, 0.02, 0, 255, 0, 0, 255, "USE_DEPTH");
 * debugInterface.renderWorld(camera);
 * ```
 */
export class DebugRenderInterface
{
    constructor()
    {
        this.cppDebugRender = globalThis.debugRenderInterface; // C++ debug render interface reference

        if (!this.cppDebugRender)
        {
            console.log('DebugRenderInterface: C++ debug render interface (globalThis.debugRenderInterface) not available');
        }
        else
        {
            console.log('DebugRenderInterface: Successfully connected to C++ debug render interface');
        }
    }

    // === CONTROL METHODS ===

    /**
     * Make debug rendering visible
     */
    setVisible()
    {
        if (!this.cppDebugRender || !this.cppDebugRender.setVisible)
        {
            console.log('DebugRenderInterface: setVisible not available');
            return;
        }
        return this.cppDebugRender.setVisible();
    }

    /**
     * Hide debug rendering
     */
    setHidden()
    {
        if (!this.cppDebugRender || !this.cppDebugRender.setHidden)
        {
            console.log('DebugRenderInterface: setHidden not available');
            return;
        }
        return this.cppDebugRender.setHidden();
    }

    /**
     * Clear all debug rendering objects
     */
    clear()
    {
        if (!this.cppDebugRender || !this.cppDebugRender.clear)
        {
            console.log('DebugRenderInterface: clear not available');
            return;
        }
        return this.cppDebugRender.clear();
    }

    /**
     * Clear all debug primitives from StateBuffer (use when changing game states)
     */
    clearAll()
    {
        if (!this.cppDebugRender || !this.cppDebugRender.clearAll)
        {
            console.log('DebugRenderInterface: clearAll not available');
            return;
        }
        return this.cppDebugRender.clearAll();
    }

    // === OUTPUT METHODS ===

    /**
     * Begin debug rendering frame
     */
    beginFrame()
    {
        if (!this.cppDebugRender || !this.cppDebugRender.beginFrame)
        {
            console.log('DebugRenderInterface: beginFrame not available');
            return;
        }
        return this.cppDebugRender.beginFrame();
    }

    /**
     * Render world-space debug objects with specified camera
     * @param {number} cameraId - Camera ID (C++ resolves to Camera* internally)
     */
    renderWorld(cameraId)
    {
        if (!this.cppDebugRender || !this.cppDebugRender.renderWorld)
        {
            console.log('DebugRenderInterface: renderWorld not available');
            return;
        }
        return this.cppDebugRender.renderWorld(cameraId);
    }

    /**
     * Render screen-space debug objects with specified camera
     * @param {number} cameraId - Camera ID (C++ resolves to Camera* internally)
     */
    renderScreen(cameraId)
    {
        if (!this.cppDebugRender || !this.cppDebugRender.renderScreen)
        {
            console.log('DebugRenderInterface: renderScreen not available');
            return;
        }
        return this.cppDebugRender.renderScreen(cameraId);
    }

    /**
     * End debug rendering frame
     */
    endFrame()
    {
        if (!this.cppDebugRender || !this.cppDebugRender.endFrame)
        {
            console.log('DebugRenderInterface: endFrame not available');
            return;
        }
        return this.cppDebugRender.endFrame();
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
        if (!this.cppDebugRender || !this.cppDebugRender.addWorldPoint)
        {
            console.log('DebugRenderInterface: addWorldPoint not available');
            return;
        }
        return this.cppDebugRender.addWorldPoint(x, y, z, radius, duration, r, g, b, a, mode);
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
        if (!this.cppDebugRender || !this.cppDebugRender.addWorldLine)
        {
            console.log('DebugRenderInterface: addWorldLine not available');
            return;
        }
        return this.cppDebugRender.addWorldLine(x1, y1, z1, x2, y2, z2, radius, duration, r, g, b, a, mode);
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
        if (!this.cppDebugRender || !this.cppDebugRender.addWorldCylinder)
        {
            console.log('DebugRenderInterface: addWorldCylinder not available');
            return;
        }
        return this.cppDebugRender.addWorldCylinder(baseX, baseY, baseZ, topX, topY, topZ, radius, duration, isWireframe, r, g, b, a, mode);
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
        if (!this.cppDebugRender || !this.cppDebugRender.addWorldWireSphere)
        {
            console.log('DebugRenderInterface: addWorldWireSphere not available');
            return;
        }
        return this.cppDebugRender.addWorldWireSphere(x, y, z, radius, duration, r, g, b, a, mode);
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
        if (!this.cppDebugRender || !this.cppDebugRender.addWorldArrow)
        {
            console.log('DebugRenderInterface: addWorldArrow not available');
            return;
        }
        return this.cppDebugRender.addWorldArrow(x1, y1, z1, x2, y2, z2, radius, duration, r, g, b, a, mode);
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
        if (!this.cppDebugRender || !this.cppDebugRender.addWorldText)
        {
            console.log('DebugRenderInterface: addWorldText not available');
            return;
        }
        return this.cppDebugRender.addWorldText(text, transform, textHeight, alignX, alignY, duration, r, g, b, a, mode);
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
        if (!this.cppDebugRender || !this.cppDebugRender.addBillboardText)
        {
            console.log('DebugRenderInterface: addBillboardText not available');
            return;
        }
        return this.cppDebugRender.addBillboardText(text, x, y, z, textHeight, alignX, alignY, duration, r, g, b, a, mode);
    }

    /**
     * Add debug coordinate basis in world space
     * @param {Array<number>} transform - 16-element transform matrix
     * @param {number} duration - Display duration
     * @param {string} mode - Render mode
     */
    addWorldBasis(transform, duration, mode = "USE_DEPTH")
    {
        if (!this.cppDebugRender || !this.cppDebugRender.addWorldBasis)
        {
            console.log('DebugRenderInterface: addWorldBasis not available');
            return;
        }
        return this.cppDebugRender.addWorldBasis(transform, duration, mode);
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
        if (!this.cppDebugRender || !this.cppDebugRender.addScreenText)
        {
            console.log('DebugRenderInterface: addScreenText not available');
            return;
        }
        return this.cppDebugRender.addScreenText(text, x, y, size, alignX, alignY, duration, r, g, b, a);
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
        if (!this.cppDebugRender || !this.cppDebugRender.addMessage)
        {
            console.log('DebugRenderInterface: addMessage not available');
            return;
        }
        return this.cppDebugRender.addMessage(text, duration, r, g, b, a);
    }

    /**
     * Check if C++ debug render interface is available
     * @returns {boolean} True if C++ interface is connected
     */
    isAvailable()
    {
        return this.cppDebugRender !== undefined && this.cppDebugRender !== null;
    }

    /**
     * Get interface status for debugging
     */
    getStatus()
    {
        return {
            available: this.isAvailable(),
            cppInterfaceType: typeof this.cppDebugRender,
            hasMethods: this.cppDebugRender ? {
                // Control methods
                setVisible: typeof this.cppDebugRender.setVisible === 'function',
                setHidden: typeof this.cppDebugRender.setHidden === 'function',
                clear: typeof this.cppDebugRender.clear === 'function',
                clearAll: typeof this.cppDebugRender.clearAll === 'function',
                beginFrame: typeof this.cppDebugRender.beginFrame === 'function',
                endFrame: typeof this.cppDebugRender.endFrame === 'function',
                // Output methods
                renderWorld: typeof this.cppDebugRender.renderWorld === 'function',
                renderScreen: typeof this.cppDebugRender.renderScreen === 'function',
                // World geometry methods
                addWorldPoint: typeof this.cppDebugRender.addWorldPoint === 'function',
                addWorldLine: typeof this.cppDebugRender.addWorldLine === 'function',
                addWorldCylinder: typeof this.cppDebugRender.addWorldCylinder === 'function',
                addWorldWireSphere: typeof this.cppDebugRender.addWorldWireSphere === 'function',
                addWorldArrow: typeof this.cppDebugRender.addWorldArrow === 'function',
                addWorldText: typeof this.cppDebugRender.addWorldText === 'function',
                addBillboardText: typeof this.cppDebugRender.addBillboardText === 'function',
                addWorldBasis: typeof this.cppDebugRender.addWorldBasis === 'function',
                // Screen geometry methods
                addScreenText: typeof this.cppDebugRender.addScreenText === 'function',
                addMessage: typeof this.cppDebugRender.addMessage === 'function'
            } : null
        };
    }
}

// Export for ES6 module system
export default DebugRenderInterface;

// Export to globalThis for hot-reload detection
globalThis.DebugRenderInterface = DebugRenderInterface;

console.log('DebugRenderInterface: Wrapper loaded (Interface Layer)');
