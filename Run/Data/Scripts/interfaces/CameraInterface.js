//----------------------------------------------------------------------------------------------------
// CameraInterface.js
// Wrapper for C++ camera interface (globalThis.cameraInterface)
// Part of the interface wrapper layer for C++/JavaScript bridge
//----------------------------------------------------------------------------------------------------

/**
 * CameraInterface - Clean abstraction over C++ camera system
 *
 * This wrapper provides a JavaScript-friendly interface to the C++ camera system,
 * abstracting direct globalThis access and providing safe fallbacks.
 *
 * Design Principles:
 * - Single Responsibility: Only wraps C++ camera interface
 * - Safe Fallbacks: Returns sensible defaults if C++ interface unavailable
 * - Type Safety: Clear return types and parameter documentation
 * - Testability: Can be mocked for unit testing
 *
 * C++ Interface Methods (exposed via globalThis.cameraInterface):
 * - createCamera(): cameraHandle
 * - destroyCamera(cameraHandle): void
 * - setPerspectiveView(camera, aspect, fov, near, far): void
 * - setOrthographicView(camera, left, top, right, bottom): void
 * - setNormalizedViewport(camera, x, y, width, height): void
 * - setCameraToRenderTransform(camera, ...matrix16): void
 * - setCameraPositionAndOrientation(camera, x, y, z, yaw, pitch, roll): void
 * - setActiveWorldCamera(camera): void
 * - getActiveWorldCamera(): cameraHandle
 *
 * Usage Example:
 * ```javascript
 * const cameraInterface = new CameraInterface();
 * const camera = cameraInterface.createCamera();
 * cameraInterface.setPerspectiveView(camera, 2.0, 60.0, 0.1, 100.0);
 * ```
 */
export class CameraInterface
{
    static version = 2;  // Hot-reload version tracking (incremented after adding setCameraRole)

    constructor()
    {
        this.cppCamera = globalThis.cameraInterface; // C++ camera interface reference

        if (!this.cppCamera)
        {
            console.warn('CameraInterface: C++ camera interface (globalThis.cameraInterface) not available');
        }
        else
        {
            console.log('CameraInterface: Successfully connected to C++ camera interface');
        }
    }

    /**
     * Create a new camera
     * @returns {number} Camera handle or null if creation failed
     */
    createCamera()
    {
        if (!this.cppCamera || !this.cppCamera.createCamera)
        {
            console.error('CameraInterface: createCamera not available');
            return null;
        }
        return this.cppCamera.createCamera();
    }

    /**
     * Destroy a camera
     * @param {number} cameraHandle - Camera handle to destroy
     */
    destroyCamera(cameraHandle)
    {
        if (!this.cppCamera || !this.cppCamera.destroyCamera)
        {
            console.error('CameraInterface: destroyCamera not available');
            return;
        }
        this.cppCamera.destroyCamera(cameraHandle);
    }

    /**
     * Set perspective projection
     * @param {number} camera - Camera handle
     * @param {number} aspect - Aspect ratio
     * @param {number} fov - Field of view in degrees
     * @param {number} near - Near plane distance
     * @param {number} far - Far plane distance
     */
    setPerspectiveView(camera, aspect, fov, near, far)
    {
        if (!this.cppCamera || !this.cppCamera.setPerspectiveView)
        {
            console.error('CameraInterface: setPerspectiveView not available');
            return;
        }
        this.cppCamera.setPerspectiveView(camera, aspect, fov, near, far);
    }

    /**
     * Set orthographic projection
     * @param {number} camera - Camera handle
     * @param {number} left - Left coordinate
     * @param {number} top - Top coordinate
     * @param {number} right - Right coordinate
     * @param {number} bottom - Bottom coordinate
     */
    setOrthographicView(camera, left, top, right, bottom)
    {
        if (!this.cppCamera || !this.cppCamera.setOrthographicView)
        {
            console.error('CameraInterface: setOrthographicView not available');
            return;
        }
        this.cppCamera.setOrthographicView(camera, left, top, right, bottom);
    }

    /**
     * Set normalized viewport
     * @param {number} camera - Camera handle
     * @param {number} x - Viewport X (0-1)
     * @param {number} y - Viewport Y (0-1)
     * @param {number} width - Viewport width (0-1)
     * @param {number} height - Viewport height (0-1)
     */
    setNormalizedViewport(camera, x, y, width, height)
    {
        if (!this.cppCamera || !this.cppCamera.setNormalizedViewport)
        {
            console.error('CameraInterface: setNormalizedViewport not available');
            return;
        }
        this.cppCamera.setNormalizedViewport(camera, x, y, width, height);
    }

    /**
     * Set camera-to-render transform matrix
     * @param {number} camera - Camera handle
     * @param {...number} matrix16 - 16 matrix elements (column-major)
     */
    setCameraToRenderTransform(camera, ...matrix16)
    {
        if (!this.cppCamera || !this.cppCamera.setCameraToRenderTransform)
        {
            console.error('CameraInterface: setCameraToRenderTransform not available');
            return;
        }
        this.cppCamera.setCameraToRenderTransform(camera, ...matrix16);
    }

    /**
     * Set camera position and orientation
     * @param {number} camera - Camera handle
     * @param {number} x - X position
     * @param {number} y - Y position
     * @param {number} z - Z position
     * @param {number} yaw - Yaw angle in degrees
     * @param {number} pitch - Pitch angle in degrees
     * @param {number} roll - Roll angle in degrees
     */
    setCameraPositionAndOrientation(camera, x, y, z, yaw, pitch, roll)
    {
        if (!this.cppCamera || !this.cppCamera.setCameraPositionAndOrientation)
        {
            console.error('CameraInterface: setCameraPositionAndOrientation not available');
            return;
        }
        this.cppCamera.setCameraPositionAndOrientation(camera, x, y, z, yaw, pitch, roll);
    }

    /**
     * Set the active world camera for 3D rendering
     * @param {number} camera - Camera handle to set as active
     */
    setActiveWorldCamera(camera)
    {
        if (!this.cppCamera || !this.cppCamera.setActiveWorldCamera)
        {
            console.error('CameraInterface: setActiveWorldCamera not available');
            return;
        }
        this.cppCamera.setActiveWorldCamera(camera);
        console.log('CameraInterface: Set active world camera:', camera);
    }

    /**
     * Get the active world camera
     * @returns {number} Active camera handle or null if not set
     */
    getActiveWorldCamera()
    {
        if (!this.cppCamera || !this.cppCamera.getActiveWorldCamera)
        {
            console.error('CameraInterface: getActiveWorldCamera not available');
            return null;
        }
        return this.cppCamera.getActiveWorldCamera();
    }

    /**
     * Set camera role for entity-based rendering (Phase 2)
     * @param {number} camera - Camera handle
     * @param {string} role - Role name ("world" for 3D, "screen" for 2D UI)
     */
    setCameraRole(camera, role)
    {
        if (!this.cppCamera || !this.cppCamera.setCameraRole)
        {
            console.error('CameraInterface: setCameraRole not available');
            return;
        }
        this.cppCamera.setCameraRole(camera, role);
        console.log(`CameraInterface: Set camera role: ${role} for camera ${camera}`);
    }

    /**
     * Get camera by role (Phase 2)
     * @param {string} role - Role name ("world" or "screen")
     * @returns {number} Camera handle or null if not found
     */
    getCameraByRole(role)
    {
        if (!this.cppCamera || !this.cppCamera.getCameraByRole)
        {
            console.error('CameraInterface: getCameraByRole not available');
            return null;
        }
        return this.cppCamera.getCameraByRole(role);
    }

    /**
     * Check if C++ camera interface is available
     * @returns {boolean} True if C++ interface is connected
     */
    isAvailable()
    {
        return this.cppCamera !== undefined && this.cppCamera !== null;
    }

    /**
     * Get interface status for debugging
     */
    getStatus()
    {
        return {
            available: this.isAvailable(),
            cppInterfaceType: typeof this.cppCamera,
            hasMethods: this.cppCamera ? {
                createCamera: typeof this.cppCamera.createCamera === 'function',
                destroyCamera: typeof this.cppCamera.destroyCamera === 'function',
                setPerspectiveView: typeof this.cppCamera.setPerspectiveView === 'function',
                setOrthographicView: typeof this.cppCamera.setOrthographicView === 'function',
                setNormalizedViewport: typeof this.cppCamera.setNormalizedViewport === 'function',
                setCameraToRenderTransform: typeof this.cppCamera.setCameraToRenderTransform === 'function',
                setCameraPositionAndOrientation: typeof this.cppCamera.setCameraPositionAndOrientation === 'function',
                setActiveWorldCamera: typeof this.cppCamera.setActiveWorldCamera === 'function',
                getActiveWorldCamera: typeof this.cppCamera.getActiveWorldCamera === 'function',
                setCameraRole: typeof this.cppCamera.setCameraRole === 'function',
                getCameraByRole: typeof this.cppCamera.getCameraByRole === 'function'
            } : null
        };
    }
}

// Export for ES6 module system
export default CameraInterface;

// Export to globalThis for hot-reload detection
globalThis.CameraInterface = CameraInterface;

console.log('CameraInterface: Wrapper loaded (Interface Layer)');
