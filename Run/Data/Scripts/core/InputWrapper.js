//----------------------------------------------------------------------------------------------------
// InputWrapper.js - JavaScript wrapper for global 'input' object
//----------------------------------------------------------------------------------------------------
import { XboxControllerWrapper } from './XboxControllerWrapper.js';

/**
 * InputWrapper - Provides JavaScript-friendly API for the global 'input' object
 *
 * Wraps C++ InputScriptInterface methods and provides controller wrappers.
 * This makes the global 'input' object easier to use from JavaScript entities.
 */
export class InputWrapper {
    constructor() {
        // Cache controller instances
        this.controllerCache = [];
        for (let i = 0; i < 4; i++) {
            this.controllerCache[i] = new XboxControllerWrapper(i);
        }
    }

    /**
     * Get XboxController wrapper for a specific controller index
     * @param {number} index - Controller index (0-3)
     * @returns {XboxControllerWrapper} Controller wrapper with button/axis methods
     */
    getController(index) {
        if (index < 0 || index >= 4) {
            console.error(`InputWrapper: Invalid controller index ${index} (must be 0-3)`);
            return this.controllerCache[0]; // Return controller 0 as fallback
        }
        return this.controllerCache[index];
    }

    /**
     * Check if key is currently down
     * @param {string|number} keyCode - Key code (e.g., 'W', 72 for 'H')
     */
    isKeyDown(keyCode) {
        if (typeof input === 'undefined') return false;

        // Convert string to char code if needed
        if (typeof keyCode === 'string') {
            keyCode = keyCode.charCodeAt(0);
        }

        return input.isKeyDown(keyCode);
    }

    /**
     * Check if key was just pressed this frame
     * @param {string|number} keyCode - Key code
     */
    wasKeyJustPressed(keyCode) {
        if (typeof input === 'undefined') return false;

        // Convert string to char code if needed
        if (typeof keyCode === 'string') {
            keyCode = keyCode.charCodeAt(0);
        }

        return input.wasKeyJustPressed(keyCode);
    }

    /**
     * Get cursor/mouse position delta since last frame
     * @returns {{x: number, y: number}} Mouse delta
     */
    getCursorClientDelta() {
        if (typeof input === 'undefined') return { x: 0, y: 0 };

        // input.getCursorClientDelta() returns a string like "{ x: 1.5, y: -2.3 }"
        // We need to parse it
        const deltaStr = input.getCursorClientDelta();
        return this.parseVec2String(deltaStr);
    }

    /**
     * Parse C++ Vec2 string format to JavaScript object
     * @param {string} vec2Str - Format: "{ x: 1.5, y: -2.3 }"
     * @returns {{x: number, y: number}}
     */
    parseVec2String(vec2Str) {
        try {
            // Use eval to parse the object string (safe since it comes from C++)
            return eval('(' + vec2Str + ')');
        } catch (error) {
            console.error('InputWrapper: Failed to parse Vec2 string:', vec2Str, error);
            return { x: 0, y: 0 };
        }
    }
}

export default InputWrapper;
console.log('InputWrapper: Module loaded');
