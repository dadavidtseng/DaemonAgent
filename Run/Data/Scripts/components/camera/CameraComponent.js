//----------------------------------------------------------------------------------------------------
// CameraComponent.js
// Camera management (migrated from PlayerEntity.js)
//----------------------------------------------------------------------------------------------------

import {Component} from '../../core/Component.js';
import {CameraInterface} from '../../interfaces/CameraInterface.js';

/**
 * CameraComponent - Manages world camera lifecycle and configuration
 *
 * Migrated from PlayerEntity.js initializeCamera() and updateCameraTransform() methods.
 * Handles camera creation, configuration, and per-frame position/orientation updates.
 *
 * Features:
 * - Creates C++ world camera via cameraInterface.createCamera()
 * - Configures perspective view, viewport, and camera-to-render transform
 * - Updates camera position/orientation every frame from GameObject transform
 * - Provides getCamera() for rendering system access
 * - Destroys camera on component destruction
 *
 * Requires:
 * - GameObject with position {x, y, z} and orientation {yaw, pitch, roll} properties
 * - CameraInterface (wrapper for C++ cameraInterface bridge)
 *
 * Usage:
 * ```javascript
 * const camera = new CameraComponent();
 * player.addComponent(camera);
 * const worldCamera = camera.getCamera(); // For rendering
 * ```
 */
export class CameraComponent extends Component
{
    static version = 2;  // Hot-reload version tracking (incremented after adding setCameraRole)

    constructor()
    {
        super('camera');

        this.worldCamera = null;
        this.cameraUpdateCount = 0;
        this.cameraInterface = new CameraInterface();

        console.log('CameraComponent: Created');
    }

    /**
     * Initialize component (called when attached to GameObject)
     * Creates and configures the world camera
     * @param {GameObject} gameObject
     */
    initialize(gameObject)
    {
        super.initialize(gameObject);

        // Ensure cameraInterface is available
        if (!this.cameraInterface.isAvailable())
        {
            console.error('CameraComponent: CameraInterface not available!');
            return;
        }

        // Create camera through CameraScriptInterface
        console.log('CameraComponent: Creating world camera...');
        try
        {
            this.worldCamera = this.cameraInterface.createCamera();
            console.log('CameraComponent: World camera created:', this.worldCamera);
        }
        catch (error)
        {
            console.error('CameraComponent: ERROR creating camera:', error);
            throw error;
        }

        // Configure camera (matches PlayerEntity.js lines 66-82)
        this.configureCamera();

        // Set initial position and orientation
        this.updateCameraTransform();

        // Mark this camera as the active world camera for 3D rendering (Phase 2)
        console.log('CameraComponent: Setting as active world camera...');
        this.cameraInterface.setActiveWorldCamera(this.worldCamera);

        // Phase 2: Set camera role for entity-based rendering
        console.log('CameraComponent: Setting camera role to "world"...');
        this.cameraInterface.setCameraRole(this.worldCamera, "world");

        console.log('CameraComponent: Initialized successfully');
    }

    /**
     * Configure camera perspective and transform (from PlayerEntity)
     */
    configureCamera()
    {
        // SetPerspectiveGraphicView(aspect=2.0, fov=60.0, near=0.1, far=100.0)
        console.log('CameraComponent: Setting perspective view (aspect=2.0, fov=60.0, near=0.1, far=100.0)');
        this.cameraInterface.setPerspectiveView(this.worldCamera, 2.0, 60.0, 0.1, 100.0);

        // SetNormalizedViewport(0, 0, 1, 1)
        console.log('CameraComponent: Setting normalized viewport (0, 0, 1, 1)');
        this.cameraInterface.setNormalizedViewport(this.worldCamera, 0.0, 0.0, 1.0, 1.0);

        // SetCameraToRenderTransform (custom matrix matching C++ Player.cpp)
        const c2r = [
            0.0, 0.0, 1.0, 0.0,  // Ix=0, Iy=0, Iz=1
            -1.0, 0.0, 0.0, 0.0, // Jx=-1, Jy=0, Jz=0
            0.0, 1.0, 0.0, 0.0,  // Kx=0, Ky=1, Kz=0
            0.0, 0.0, 0.0, 1.0   // Tx=0, Ty=0, Tz=0, Tw=1
        ];
        console.log('CameraComponent: Setting camera-to-render transform matrix');
        this.cameraInterface.setCameraToRenderTransform(this.worldCamera, ...c2r);
    }

    /**
     * Update camera transform every frame
     * @param {number} deltaTime - Time since last frame in milliseconds
     */
    update(deltaTime)
    {
        if (!this.gameObject || !this.worldCamera)
        {
            return;
        }

        // Update camera position/orientation from GameObject transform
        this.updateCameraTransform();
    }

    /**
     * Update C++ camera position and orientation (from PlayerEntity)
     */
    updateCameraTransform()
    {
        if (!this.worldCamera || !this.gameObject)
        {
            return;
        }

        // Log camera updates occasionally (every 60 frames) to avoid spam
        this.cameraUpdateCount++;
        if (this.cameraUpdateCount % 60 === 0)
        {
            console.log(`CameraComponent: Update Camera Transform (frame ${this.cameraUpdateCount})`);
            console.log('  position:', JSON.stringify(this.gameObject.position));
            console.log('  orientation:', JSON.stringify(this.gameObject.orientation));
        }

        this.cameraInterface.setCameraPositionAndOrientation(
            this.worldCamera,
            this.gameObject.position.x,
            this.gameObject.position.y,
            this.gameObject.position.z,
            this.gameObject.orientation.yaw,
            this.gameObject.orientation.pitch,
            this.gameObject.orientation.roll
        );
    }

    /**
     * Get camera handle for rendering
     */
    getCamera()
    {
        return this.worldCamera;
    }

    /**
     * Cleanup - Destroy camera when component is destroyed
     */
    destroy()
    {
        if (this.worldCamera)
        {
            console.log('CameraComponent: Destroying world camera');
            this.cameraInterface.destroyCamera(this.worldCamera);
            this.worldCamera = null;
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
            hasCamera: this.worldCamera !== null,
            cameraUpdateCount: this.cameraUpdateCount
        };
    }
}

// Export for ES6 module system
export default CameraComponent;

// Export to globalThis for hot-reload detection
globalThis.CameraComponent = CameraComponent;

console.log('CameraComponent: Loaded (Phase 2 - Camera System)');
