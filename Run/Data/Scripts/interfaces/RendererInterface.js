//----------------------------------------------------------------------------------------------------
// RendererInterface.js
// Wrapper for C++ renderer interface (globalThis.renderer)
// Part of the interface wrapper layer for C++/JavaScript bridge
//----------------------------------------------------------------------------------------------------

/**
 * RendererInterface - Clean abstraction over C++ renderer system
 *
 * This wrapper provides a JavaScript-friendly interface to the C++ renderer system,
 * abstracting direct globalThis access and providing safe fallbacks.
 *
 * Design Principles:
 * - Single Responsibility: Only wraps C++ renderer interface
 * - Safe Fallbacks: Returns sensible defaults if C++ interface unavailable
 * - Type Safety: Clear return types and parameter documentation
 * - Testability: Can be mocked for unit testing
 *
 * C++ Interface Methods (exposed via globalThis.renderer):
 * - beginCamera(cameraHandle): void
 * - endCamera(cameraHandle): void
 * - setModelConstants(x, y, z, yaw, pitch, roll, r, g, b, a): void
 * - drawVertexArray(vertexArrayHandle): void
 *
 * Usage Example:
 * ```javascript
 * const renderer = new RendererInterface();
 * renderer.beginCamera(camera);
 * renderer.setModelConstants(0, 0, 0, 0, 0, 0, 255, 255, 255, 255);
 * renderer.drawVertexArray(vertexArray);
 * renderer.endCamera(camera);
 * ```
 */
export class RendererInterface
{
    constructor()
    {
        this.cppRenderer = globalThis.renderer; // C++ renderer interface reference

        if (!this.cppRenderer)
        {
            console.warn('RendererInterface: C++ renderer interface (globalThis.renderer) not available');
        }
        else
        {
            console.log('RendererInterface: Successfully connected to C++ renderer interface');
        }
    }

    /**
     * Begin rendering with a camera
     * @param {number} cameraHandle - Camera handle to use for rendering
     */
    beginCamera(cameraHandle)
    {
        if (!this.cppRenderer || !this.cppRenderer.beginCamera)
        {
            console.error('RendererInterface: beginCamera not available');
            return;
        }
        this.cppRenderer.beginCamera(cameraHandle);
    }

    /**
     * End rendering with a camera
     * @param {number} cameraHandle - Camera handle to end rendering
     */
    endCamera(cameraHandle)
    {
        if (!this.cppRenderer || !this.cppRenderer.endCamera)
        {
            console.error('RendererInterface: endCamera not available');
            return;
        }
        this.cppRenderer.endCamera(cameraHandle);
    }

    /**
     * Set model constants (transform + color)
     * @param {number} x - X position
     * @param {number} y - Y position
     * @param {number} z - Z position
     * @param {number} yaw - Yaw angle in degrees
     * @param {number} pitch - Pitch angle in degrees
     * @param {number} roll - Roll angle in degrees
     * @param {number} r - Red color (0-255)
     * @param {number} g - Green color (0-255)
     * @param {number} b - Blue color (0-255)
     * @param {number} a - Alpha color (0-255)
     */
    setModelConstants(x, y, z, yaw, pitch, roll, r, g, b, a)
    {
        if (!this.cppRenderer || !this.cppRenderer.setModelConstants)
        {
            console.error('RendererInterface: setModelConstants not available');
            return;
        }
        this.cppRenderer.setModelConstants(x, y, z, yaw, pitch, roll, r, g, b, a);
    }

    /**
     * Draw a vertex array
     * @param {number} vertexArrayHandle - Vertex array handle to draw
     */
    drawVertexArray(vertexArrayHandle)
    {
        if (!this.cppRenderer || !this.cppRenderer.drawVertexArray)
        {
            console.error('RendererInterface: drawVertexArray not available');
            return;
        }
        this.cppRenderer.drawVertexArray(vertexArrayHandle);
    }

    /**
     * Check if C++ renderer interface is available
     * @returns {boolean} True if C++ interface is connected
     */
    isAvailable()
    {
        return this.cppRenderer !== undefined && this.cppRenderer !== null;
    }

    /**
     * Get interface status for debugging
     */
    getStatus()
    {
        return {
            available: this.isAvailable(),
            cppInterfaceType: typeof this.cppRenderer,
            hasMethods: this.cppRenderer ? {
                beginCamera: typeof this.cppRenderer.beginCamera === 'function',
                endCamera: typeof this.cppRenderer.endCamera === 'function',
                setModelConstants: typeof this.cppRenderer.setModelConstants === 'function',
                drawVertexArray: typeof this.cppRenderer.drawVertexArray === 'function'
            } : null
        };
    }
}

// Export for ES6 module system
export default RendererInterface;

// Export to globalThis for hot-reload detection
globalThis.RendererInterface = RendererInterface;

console.log('RendererInterface: Wrapper loaded (Interface Layer)');
