//----------------------------------------------------------------------------------------------------
// InputSetup.js - Extends global 'input' object with JavaScript wrapper methods
//----------------------------------------------------------------------------------------------------
import { XboxControllerWrapper } from './XboxControllerWrapper.js';

/**
 * InputSetup - Patches the global 'input' object with convenient wrapper methods
 *
 * Adds getController() method that returns XboxControllerWrapper instances.
 * This allows JavaScript code to use controllers with a clean API.
 */
export function setupInputGlobal() {
    if (typeof input === 'undefined') {
        console.error('InputSetup: global "input" object not available!');
        return;
    }

    console.log('InputSetup: Setting up input wrapper...');

    // Create controller wrapper cache
    const controllerCache = [];
    for (let i = 0; i < 4; i++) {
        controllerCache[i] = new XboxControllerWrapper(i);
    }

    // Store references to original C++ methods (don't enumerate, directly reference)
    const originalGetCursorClientDelta = input.getCursorClientDelta;
    const originalIsKeyDown = input.isKeyDown;
    const originalWasKeyJustPressed = input.wasKeyJustPressed;

    console.log('InputSetup: Original method checks:');
    console.log('  - getCursorClientDelta:', typeof originalGetCursorClientDelta);
    console.log('  - isKeyDown:', typeof originalIsKeyDown);
    console.log('  - wasKeyJustPressed:', typeof originalWasKeyJustPressed);

    // Patch getController method
    input.getController = function(index) {
        if (index < 0 || index >= 4) {
            console.error(`input.getController: Invalid controller index ${index} (must be 0-3)`);
            return controllerCache[0]; // Return controller 0 as fallback
        }
        return controllerCache[index];
    };

    // Wrap isKeyDown to handle string key codes
    if (originalIsKeyDown) {
        input.isKeyDown = function(keyCode) {
            // Convert string to char code if needed
            if (typeof keyCode === 'string') {
                keyCode = keyCode.charCodeAt(0);
            }
            return originalIsKeyDown.call(input, keyCode);
        };
        console.log('InputSetup: Patched isKeyDown');
    }

    // Wrap wasKeyJustPressed to handle string key codes
    if (originalWasKeyJustPressed) {
        input.wasKeyJustPressed = function(keyCode) {
            // Convert string to char code if needed
            if (typeof keyCode === 'string') {
                keyCode = keyCode.charCodeAt(0);
            }
            return originalWasKeyJustPressed.call(input, keyCode);
        };
        console.log('InputSetup: Patched wasKeyJustPressed');
    }

    // Add convenience wrapper for getCursorClientDelta that returns parsed object
    if (originalGetCursorClientDelta) {
        input.getCursorClientDelta = function() {
            const deltaStr = originalGetCursorClientDelta.call(input);
            try {
                // Parse C++ Vec2 string format: "{ x: 1.5, y: -2.3 }"
                return eval('(' + deltaStr + ')');
            } catch (error) {
                console.error('input.getCursorClientDelta: Failed to parse:', deltaStr, error);
                return { x: 0, y: 0 };
            }
        };
        console.log('InputSetup: Patched getCursorClientDelta');
    } else {
        console.error('InputSetup: getCursorClientDelta not found in input object!');
    }

    console.log('InputSetup: Global input object patched successfully');
}

// Auto-setup when module loads
if (typeof input !== 'undefined') {
    setupInputGlobal();
}

export default setupInputGlobal;
console.log('InputSetup: Module loaded');
