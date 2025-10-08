//----------------------------------------------------------------------------------------------------
// XboxControllerWrapper.js - JavaScript wrapper for XboxController C++ interface
//----------------------------------------------------------------------------------------------------

/**
 * XboxControllerWrapper - Wraps C++ XboxController methods with JavaScript API
 *
 * Provides convenient access to controller state through the InputScriptInterface.
 * Maps button names to indices and wraps C++ methods for JavaScript usage.
 *
 * Button Mapping (from C++ XboxController):
 * - 'A'         → 0   (XBOX_BUTTON_A)
 * - 'B'         → 1   (XBOX_BUTTON_B)
 * - 'X'         → 2   (XBOX_BUTTON_X)
 * - 'Y'         → 3   (XBOX_BUTTON_Y)
 * - 'LSHOULDER' → 4   (XBOX_BUTTON_LSHOULDER)
 * - 'RSHOULDER' → 5   (XBOX_BUTTON_RSHOULDER)
 * - 'BACK'      → 6   (XBOX_BUTTON_BACK)
 * - 'START'     → 7   (XBOX_BUTTON_START)
 * - 'LTHUMB'    → 8   (XBOX_BUTTON_LTHUMB)
 * - 'RTHUMB'    → 9   (XBOX_BUTTON_RTHUMB)
 * - 'DPAD_UP'   → 10  (XBOX_BUTTON_DPAD_UP)
 * - 'DPAD_DOWN' → 11  (XBOX_BUTTON_DPAD_DOWN)
 * - 'DPAD_LEFT' → 12  (XBOX_BUTTON_DPAD_LEFT)
 * - 'DPAD_RIGHT'→ 13  (XBOX_BUTTON_DPAD_RIGHT)
 */
export class XboxControllerWrapper {
    constructor(controllerIndex) {
        this.controllerIndex = controllerIndex;

        // Button name to index mapping
        this.buttonMap = {
            'A': 0,
            'B': 1,
            'X': 2,
            'Y': 3,
            'LSHOULDER': 4,
            'RSHOULDER': 5,
            'BACK': 6,
            'START': 7,
            'LTHUMB': 8,
            'RTHUMB': 9,
            'DPAD_UP': 10,
            'DPAD_DOWN': 11,
            'DPAD_LEFT': 12,
            'DPAD_RIGHT': 13
        };

        // Axis indices
        this.axisMap = {
            'LEFT_STICK_X': 0,
            'LEFT_STICK_Y': 1,
            'RIGHT_STICK_X': 2,
            'RIGHT_STICK_Y': 3,
            'LEFT_TRIGGER': 4,
            'RIGHT_TRIGGER': 5
        };
    }

    /**
     * Check if controller is connected
     */
    isConnected() {
        if (typeof input === 'undefined') return false;
        return input.isControllerConnected(this.controllerIndex);
    }

    /**
     * Check if button is currently down
     * @param {string} buttonName - Button name (e.g., 'A', 'START', 'LSHOULDER')
     */
    isButtonDown(buttonName) {
        if (typeof input === 'undefined') return false;

        const buttonIndex = this.buttonMap[buttonName];
        if (buttonIndex === undefined) {
            console.error(`XboxControllerWrapper: Unknown button name '${buttonName}'`);
            return false;
        }

        return input.isControllerButtonPressed(this.controllerIndex, buttonIndex);
    }

    /**
     * Check if button was just pressed this frame
     * @param {string} buttonName - Button name (e.g., 'A', 'START', 'LSHOULDER')
     */
    wasButtonJustPressed(buttonName) {
        // Note: InputScriptInterface doesn't have wasButtonJustPressed yet
        // For now, use isButtonDown (same behavior as C++ until we add the method)
        return this.isButtonDown(buttonName);
    }

    /**
     * Get left analog stick position
     * @returns {{x: number, y: number}} Stick position (-1.0 to 1.0)
     */
    getLeftStick() {
        if (typeof input === 'undefined') return { x: 0.0, y: 0.0 };

        const x = input.getControllerAxis(this.controllerIndex, this.axisMap.LEFT_STICK_X);
        const y = input.getControllerAxis(this.controllerIndex, this.axisMap.LEFT_STICK_Y);

        return { x, y };
    }

    /**
     * Get right analog stick position
     * @returns {{x: number, y: number}} Stick position (-1.0 to 1.0)
     */
    getRightStick() {
        if (typeof input === 'undefined') return { x: 0.0, y: 0.0 };

        const x = input.getControllerAxis(this.controllerIndex, this.axisMap.RIGHT_STICK_X);
        const y = input.getControllerAxis(this.controllerIndex, this.axisMap.RIGHT_STICK_Y);

        return { x, y };
    }

    /**
     * Get left trigger value
     * @returns {number} Trigger value (0.0 to 1.0)
     */
    getLeftTrigger() {
        if (typeof input === 'undefined') return 0.0;
        return input.getControllerAxis(this.controllerIndex, this.axisMap.LEFT_TRIGGER);
    }

    /**
     * Get right trigger value
     * @returns {number} Trigger value (0.0 to 1.0)
     */
    getRightTrigger() {
        if (typeof input === 'undefined') return 0.0;
        return input.getControllerAxis(this.controllerIndex, this.axisMap.RIGHT_TRIGGER);
    }
}

export default XboxControllerWrapper;
console.log('XboxControllerWrapper: Module loaded');
