//----------------------------------------------------------------------------------------------------
// CameraSystem.js - JavaScript wrapper for CameraScriptInterface
//----------------------------------------------------------------------------------------------------
import { Subsystem } from '../core/Subsystem.js';

/**
 * CameraSystem - JavaScript wrapper for CameraScriptInterface
 * Phase 4 ES6 Module using Subsystem pattern
 *
 * Features:
 * - Camera lifecycle management
 * - Initialization verification for C++ CameraScriptInterface
 * - Global cameraInterface exposure (registered in App.cpp)
 *
 * Note: No wrapper methods - uses global cameraInterface directly
 * Usage: cameraInterface.createCamera(), cameraInterface.destroyCamera(), etc.
 */
export class CameraSystem extends Subsystem {
    constructor() {
        super('cameraSystem', 3, { enabled: true }); // Priority 3, before input (10)

        this.isInitialized = false;

        console.log('CameraSystem: Module loaded (Phase 4 ES6)');
        this.initialize();
    }

    /**
     * Initialize the camera system and verify C++ cameraInterface availability
     */
    initialize() {
        try {
            // Check if C++ cameraInterface is available
            if (typeof cameraInterface !== 'undefined') {
                console.log('CameraSystem: C++ cameraInterface available');
                this.isInitialized = true;
            } else {
                console.error('CameraSystem: C++ cameraInterface NOT available!');
                this.isInitialized = false;
            }
        } catch (error) {
            console.error('CameraSystem: Initialization failed:', error);
            this.isInitialized = false;
        }
    }

    /**
     * Update method - called every frame
     * @param {number} gameDelta - Game time delta
     * @param {number} systemDelta - System time delta
     */
    update(gameDelta, systemDelta) {
        // Camera system has no per-frame update logic
        // All camera operations are done through cameraInterface directly
    }

    /**
     * Render method - called every frame during render phase
     */
    render() {
        // Camera system has no rendering logic
    }

    /**
     * Get system status for debugging
     */
    getSystemStatus() {
        return {
            id: this.id,
            priority: this.priority,
            enabled: this.enabled,
            isInitialized: this.isInitialized
        };
    }
}

// Export for ES6 module system
export default CameraSystem;

// Export to globalThis for hot-reload detection
globalThis.CameraSystem = CameraSystem;

console.log('CameraSystem: Component loaded (Phase 4 ES6)');
