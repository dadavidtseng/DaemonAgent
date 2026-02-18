//----------------------------------------------------------------------------------------------------
// InputInterface.js
// Wrapper over globalThis.inputState (FrameEventQueue-driven local input state)
// Part of the interface wrapper layer for C++/JavaScript bridge
//----------------------------------------------------------------------------------------------------

/**
 * InputInterface - Clean abstraction over the JS-local InputState
 *
 * After the FrameEventQueue refactor, keyboard/mouse state is maintained locally
 * in globalThis.inputState (populated by InputSystem draining FrameEventQueue).
 * This wrapper provides the same API surface that KeyboardInputComponent and
 * other consumers relied on, now backed by the race-free local state.
 *
 * Controller input is not yet routed through FrameEventQueue and returns defaults.
 */
export class InputInterface
{
    constructor()
    {
        // Create controller wrapper cache (stubs until controller events are added)
        this.controllerCache = [];
        for (let i = 0; i < 4; i++)
        {
            this.controllerCache[i] = new XboxController(this, i);
        }

        console.log('InputInterface: Connected to globalThis.inputState (FrameEventQueue)');
    }

    // === KEYBOARD INPUT ===

    /**
     * Check if a key was just pressed this frame
     * @param {number} keyCode - Virtual key code
     * @returns {boolean} True if key was just pressed
     */
    wasKeyJustPressed(keyCode)
    {
        const state = globalThis.inputState;
        return state ? !!state.justPressed[keyCode] : false;
    }

    /**
     * Check if a key was just released this frame
     * @param {number} keyCode - Virtual key code
     * @returns {boolean} True if key was just released
     */
    wasKeyJustReleased(keyCode)
    {
        const state = globalThis.inputState;
        return state ? !!state.justReleased[keyCode] : false;
    }

    /**
     * Check if a key is currently held down
     * @param {number} keyCode - Virtual key code
     * @returns {boolean} True if key is down
     */
    isKeyDown(keyCode)
    {
        const state = globalThis.inputState;
        return state ? !!state.keys[keyCode] : false;
    }

    // === MOUSE INPUT ===

    /**
     * Get current mouse position
     * @returns {{x: number, y: number}} Mouse coordinates
     */
    getMousePosition()
    {
        const state = globalThis.inputState;
        return state ? {x: state.cursorX, y: state.cursorY} : {x: 0, y: 0};
    }

    /**
     * Get mouse cursor delta (movement since last frame)
     * @returns {{x: number, y: number}} Mouse delta coordinates
     */
    getCursorClientDelta()
    {
        const state = globalThis.inputState;
        return state ? {x: state.cursorDX, y: state.cursorDY} : {x: 0, y: 0};
    }

    /**
     * Check if a mouse button is currently down
     * Mouse buttons are tracked via mouseButtonDown/mouseButtonUp events in InputState.
     * @param {number} button - Mouse button index (0=left, 1=right, 2=middle)
     * @returns {boolean} True if button is down
     */
    isMouseButtonDown(button)
    {
        // Mouse buttons share the keys[] array via keyCode from C++ FrameEvent
        // C++ maps mouse buttons to virtual key codes (VK_LBUTTON=1, VK_RBUTTON=2, VK_MBUTTON=4)
        const state = globalThis.inputState;
        if (!state) return false;
        // Map button index to Windows VK codes: 0→1(VK_LBUTTON), 1→2(VK_RBUTTON), 2→4(VK_MBUTTON)
        const vkMap = [1, 2, 4];
        const vk = vkMap[button];
        return vk !== undefined ? !!state.keys[vk] : false;
    }

    // === CONTROLLER INPUT (stubs — not yet routed through FrameEventQueue) ===

    /**
     * Get Xbox controller wrapper for a specific controller index
     * @param {number} index - Controller index (0-3)
     * @returns {XboxController} Controller wrapper with button/axis methods
     */
    getController(index)
    {
        if (index < 0 || index >= 4)
        {
            console.log(`InputInterface: Invalid controller index ${index} (must be 0-3)`);
            return this.controllerCache[0];
        }
        return this.controllerCache[index];
    }

    /**
     * Check if controller is connected
     * TODO: Route controller events through FrameEventQueue
     */
    isControllerConnected(index)
    {
        return false;
    }

    /**
     * Check if controller button is pressed
     * TODO: Route controller events through FrameEventQueue
     */
    isControllerButtonPressed(controllerIndex, buttonIndex)
    {
        return false;
    }

    /**
     * Get controller axis value
     * TODO: Route controller events through FrameEventQueue
     */
    getControllerAxis(controllerIndex, axisIndex)
    {
        return 0.0;
    }

    // === INTERFACE STATUS ===

    /**
     * Check if input state is available
     * @returns {boolean} True if globalThis.inputState exists
     */
    isAvailable()
    {
        return globalThis.inputState !== undefined && globalThis.inputState !== null;
    }

    /**
     * Get interface status for debugging
     */
    getStatus()
    {
        return {
            available: this.isAvailable(),
            backend: 'FrameEventQueue (inputState)',
            controllerSupport: false
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
            console.log(`XboxController: Unknown button name '${buttonName}'`);
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

console.log('InputInterface: Wrapper loaded (FrameEventQueue backend with Xbox Controller stubs)');
