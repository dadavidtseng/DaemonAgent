//----------------------------------------------------------------------------------------------------
// InputInterface.js
// Wrapper for C++ input interface (globalThis.input)
// Part of the interface wrapper layer for C++/JavaScript bridge
//----------------------------------------------------------------------------------------------------

/**
 * InputInterface - Clean abstraction over C++ input system
 *
 * This wrapper provides a JavaScript-friendly interface to the C++ input system,
 * abstracting direct globalThis access and providing safe fallbacks.
 *
 * Design Principles:
 * - Single Responsibility: Only wraps C++ input interface
 * - Safe Fallbacks: Returns sensible defaults if C++ interface unavailable
 * - Type Safety: Clear return types and parameter documentation
 * - Testability: Can be mocked for unit testing
 *
 * C++ Interface Methods (exposed via globalThis.input):
 * - wasKeyJustPressed(keyCode): boolean
 * - wasKeyJustReleased(keyCode): boolean
 * - isKeyDown(keyCode): boolean
 * - getMousePosition(): {x, y}
 * - getCursorClientDelta(): {x, y}
 * - isMouseButtonDown(button): boolean
 * - isControllerConnected(index): boolean
 * - isControllerButtonPressed(index, button): boolean
 * - getControllerAxis(index, axis): number
 *
 * Xbox Controller Button Mapping:
 * - A = 0, B = 1, X = 2, Y = 3
 * - LSHOULDER = 4, RSHOULDER = 5
 * - BACK = 6, START = 7
 * - LTHUMB = 8, RTHUMB = 9
 * - DPAD_UP = 10, DPAD_DOWN = 11, DPAD_LEFT = 12, DPAD_RIGHT = 13
 *
 * Xbox Controller Axis Mapping:
 * - LEFT_STICK_X = 0, LEFT_STICK_Y = 1
 * - RIGHT_STICK_X = 2, RIGHT_STICK_Y = 3
 * - LEFT_TRIGGER = 4, RIGHT_TRIGGER = 5
 *
 * Usage Example:
 * ```javascript
 * const inputInterface = new InputInterface();
 * if (inputInterface.wasKeyJustPressed(KEYCODE_SPACE)) {
 *     // Handle spacebar press
 * }
 * const controller = inputInterface.getController(0);
 * if (controller.isButtonDown('A')) {
 *     // Handle A button press
 * }
 * ```
 */
export class InputInterface
{
    constructor()
    {
        this.cppInput = globalThis.input; // C++ input interface reference

        if (!this.cppInput)
        {
            console.warn('InputInterface: C++ input interface (globalThis.input) not available');
        }
        else
        {
            console.log('InputInterface: Successfully connected to C++ input interface');
        }

        // Create controller wrapper cache
        this.controllerCache = [];
        for (let i = 0; i < 4; i++)
        {
            this.controllerCache[i] = new XboxController(this, i);
        }
    }

    // === KEYBOARD INPUT ===

    /**
     * Check if a key was just pressed this frame
     * @param {number} keyCode - Virtual key code
     * @returns {boolean} True if key was just pressed
     */
    wasKeyJustPressed(keyCode)
    {
        if (!this.cppInput)
        {
            return false;
        }
        return this.cppInput.wasKeyJustPressed(keyCode) ?? false;
    }

    /**
     * Check if a key was just released this frame
     * @param {number} keyCode - Virtual key code
     * @returns {boolean} True if key was just released
     */
    wasKeyJustReleased(keyCode)
    {
        if (!this.cppInput)
        {
            return false;
        }
        return this.cppInput.wasKeyJustReleased(keyCode) ?? false;
    }

    /**
     * Check if a key is currently held down
     * @param {number} keyCode - Virtual key code
     * @returns {boolean} True if key is down
     */
    isKeyDown(keyCode)
    {
        if (!this.cppInput)
        {
            return false;
        }
        return this.cppInput.isKeyDown(keyCode) ?? false;
    }

    // === MOUSE INPUT ===

    /**
     * Get current mouse position
     * @returns {{x: number, y: number}} Mouse coordinates
     */
    getMousePosition()
    {
        if (!this.cppInput || !this.cppInput.getMousePosition)
        {
            return {x: 0, y: 0};
        }
        return this.cppInput.getMousePosition() ?? {x: 0, y: 0};
    }

    /**
     * Get mouse cursor delta (movement since last frame)
     * @returns {{x: number, y: number}} Mouse delta coordinates
     */
    getCursorClientDelta()
    {
        if (!this.cppInput || !this.cppInput.getCursorClientDelta)
        {
            return {x: 0, y: 0};
        }

        // C++ returns a string like "{ x: 1.5, y: -2.3 }", need to parse it
        const deltaStr = this.cppInput.getCursorClientDelta();

        if (!deltaStr || typeof deltaStr !== 'string')
        {
            return {x: 0, y: 0};
        }

        try
        {
            // Parse C++ Vec2 string format: "{ x: 1.5, y: -2.3 }"
            // Using eval is safe here since the string comes directly from C++ engine
            return eval('(' + deltaStr + ')');
        }
        catch (error)
        {
            console.error('InputInterface: Failed to parse getCursorClientDelta:', deltaStr, error);
            return {x: 0, y: 0};
        }
    }

    /**
     * Check if a mouse button is currently down
     * @param {number} button - Mouse button index (0=left, 1=right, 2=middle)
     * @returns {boolean} True if button is down
     */
    isMouseButtonDown(button)
    {
        if (!this.cppInput || !this.cppInput.isMouseButtonDown)
        {
            return false;
        }
        return this.cppInput.isMouseButtonDown(button) ?? false;
    }

    // === CONTROLLER INPUT ===

    /**
     * Get Xbox controller wrapper for a specific controller index
     * @param {number} index - Controller index (0-3)
     * @returns {XboxController} Controller wrapper with button/axis methods
     */
    getController(index)
    {
        if (index < 0 || index >= 4)
        {
            console.error(`InputInterface: Invalid controller index ${index} (must be 0-3)`);
            return this.controllerCache[0]; // Return controller 0 as fallback
        }
        return this.controllerCache[index];
    }

    /**
     * Check if controller is connected
     * @param {number} index - Controller index (0-3)
     * @returns {boolean} True if controller is connected
     */
    isControllerConnected(index)
    {
        if (!this.cppInput || !this.cppInput.isControllerConnected)
        {
            return false;
        }
        return this.cppInput.isControllerConnected(index) ?? false;
    }

    /**
     * Check if controller button is pressed
     * @param {number} controllerIndex - Controller index (0-3)
     * @param {number} buttonIndex - Button index (0-13)
     * @returns {boolean} True if button is pressed
     */
    isControllerButtonPressed(controllerIndex, buttonIndex)
    {
        if (!this.cppInput || !this.cppInput.isControllerButtonPressed)
        {
            return false;
        }
        return this.cppInput.isControllerButtonPressed(controllerIndex, buttonIndex) ?? false;
    }

    /**
     * Get controller axis value
     * @param {number} controllerIndex - Controller index (0-3)
     * @param {number} axisIndex - Axis index (0-5)
     * @returns {number} Axis value (-1.0 to 1.0 for sticks, 0.0 to 1.0 for triggers)
     */
    getControllerAxis(controllerIndex, axisIndex)
    {
        if (!this.cppInput || !this.cppInput.getControllerAxis)
        {
            return 0.0;
        }
        return this.cppInput.getControllerAxis(controllerIndex, axisIndex) ?? 0.0;
    }

    // === INTERFACE STATUS ===

    /**
     * Check if C++ input interface is available
     * @returns {boolean} True if C++ interface is connected
     */
    isAvailable()
    {
        return this.cppInput !== undefined && this.cppInput !== null;
    }

    /**
     * Get interface status for debugging
     */
    getStatus()
    {
        return {
            available: this.isAvailable(),
            cppInterfaceType: typeof this.cppInput,
            hasMethods: this.cppInput ? {
                wasKeyJustPressed: typeof this.cppInput.wasKeyJustPressed === 'function',
                wasKeyJustReleased: typeof this.cppInput.wasKeyJustReleased === 'function',
                isKeyDown: typeof this.cppInput.isKeyDown === 'function',
                isControllerConnected: typeof this.cppInput.isControllerConnected === 'function',
                isControllerButtonPressed: typeof this.cppInput.isControllerButtonPressed === 'function',
                getControllerAxis: typeof this.cppInput.getControllerAxis === 'function'
            } : null
        };
    }
}

/**
 * XboxController - Helper class for Xbox controller input
 *
 * Provides convenient button name mapping and axis access for Xbox controllers.
 * Created internally by InputInterface and accessed via getController(index).
 */
class XboxController
{
    constructor(inputInterface, controllerIndex)
    {
        this.inputInterface = inputInterface;
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
     * @returns {boolean} True if connected
     */
    isConnected()
    {
        return this.inputInterface.isControllerConnected(this.controllerIndex);
    }

    /**
     * Check if button is currently down
     * @param {string} buttonName - Button name (e.g., 'A', 'START', 'LSHOULDER')
     * @returns {boolean} True if button is down
     */
    isButtonDown(buttonName)
    {
        const buttonIndex = this.buttonMap[buttonName];
        if (buttonIndex === undefined)
        {
            console.error(`XboxController: Unknown button name '${buttonName}'`);
            return false;
        }

        return this.inputInterface.isControllerButtonPressed(this.controllerIndex, buttonIndex);
    }

    /**
     * Check if button was just pressed this frame
     * @param {string} buttonName - Button name (e.g., 'A', 'START', 'LSHOULDER')
     * @returns {boolean} True if button was just pressed
     */
    wasButtonJustPressed(buttonName)
    {
        // Note: InputScriptInterface doesn't have wasButtonJustPressed yet
        // For now, use isButtonDown (same behavior as C++ until we add the method)
        return this.isButtonDown(buttonName);
    }

    /**
     * Get left analog stick position
     * @returns {{x: number, y: number}} Stick position (-1.0 to 1.0)
     */
    getLeftStick()
    {
        const x = this.inputInterface.getControllerAxis(this.controllerIndex, this.axisMap.LEFT_STICK_X);
        const y = this.inputInterface.getControllerAxis(this.controllerIndex, this.axisMap.LEFT_STICK_Y);
        return {x, y};
    }

    /**
     * Get right analog stick position
     * @returns {{x: number, y: number}} Stick position (-1.0 to 1.0)
     */
    getRightStick()
    {
        const x = this.inputInterface.getControllerAxis(this.controllerIndex, this.axisMap.RIGHT_STICK_X);
        const y = this.inputInterface.getControllerAxis(this.controllerIndex, this.axisMap.RIGHT_STICK_Y);
        return {x, y};
    }

    /**
     * Get left trigger value
     * @returns {number} Trigger value (0.0 to 1.0)
     */
    getLeftTrigger()
    {
        return this.inputInterface.getControllerAxis(this.controllerIndex, this.axisMap.LEFT_TRIGGER);
    }

    /**
     * Get right trigger value
     * @returns {number} Trigger value (0.0 to 1.0)
     */
    getRightTrigger()
    {
        return this.inputInterface.getControllerAxis(this.controllerIndex, this.axisMap.RIGHT_TRIGGER);
    }
}

// Export for ES6 module system
export default InputInterface;

// Export to globalThis for hot-reload detection
globalThis.InputInterface = InputInterface;

console.log('InputInterface: Wrapper loaded (Interface Layer with Xbox Controller support)');
