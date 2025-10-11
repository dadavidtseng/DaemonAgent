//----------------------------------------------------------------------------------------------------
// PulseColorBehavior.js
// Pulsing color using sin wave (Prop 1 behavior)
//----------------------------------------------------------------------------------------------------

import {BehaviorComponent} from './BehaviorComponent.js';

/**
 * PulseColorBehavior - Pulsates mesh color using sin wave
 *
 * Migrated from PropEntity 'pulse-color' behavior.
 * Used by Prop 1: Cube at (-2, -2, 0).
 *
 * Behavior:
 * - Color pulsates using: (sin(time) + 1) * 0.5 * 255
 * - Requires MeshComponent to set color
 *
 * Usage:
 * ```javascript
 * const behavior = new PulseColorBehavior();
 * prop.addComponent(behavior);
 * ```
 */
export class PulseColorBehavior extends BehaviorComponent
{
    constructor()
    {
        super('pulse-color');

        this.startTime = Date.now() / 1000.0; // seconds
        this.meshComponent = null;

        console.log('PulseColorBehavior: Created');
    }

    /**
     * Initialize - find MeshComponent to control color
     * @param {GameObject} gameObject
     */
    initialize(gameObject)
    {
        super.initialize(gameObject);

        // Find MeshComponent on GameObject
        this.meshComponent = this.gameObject.getComponent('mesh');

        if (!this.meshComponent)
        {
            console.warn('PulseColorBehavior: No MeshComponent found on GameObject!');
        }
    }

    /**
     * Update behavior - pulse color
     * @param {number} deltaTime - Time since last frame in milliseconds
     */
    update(deltaTime)
    {
        if (!this.gameObject || !this.meshComponent)
        {
            return;
        }

        // Calculate elapsed time (from PropEntity line 101-102)
        const currentTime = Date.now() / 1000.0;
        const elapsedTime = currentTime - this.startTime;

        // Pulsing color calculation (from PropEntity line 103-106)
        // C++ code: float const colorValue = (sinf(time) + 1.0f) * 0.5f * 255.0f;
        const colorValue = (Math.sin(elapsedTime) + 1.0) * 0.5 * 255.0;
        const colorInt = Math.floor(colorValue);

        // Set mesh color
        this.meshComponent.setColor({
            r: colorInt,
            g: colorInt,
            b: colorInt,
            a: 255
        });
    }
}

// Export for ES6 module system
export default PulseColorBehavior;

// Export to globalThis for hot-reload detection
globalThis.PulseColorBehavior = PulseColorBehavior;

console.log('PulseColorBehavior: Loaded (Phase 4 - Prop Migration)');
