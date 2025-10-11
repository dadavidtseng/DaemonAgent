// InputSystem.js
// Phase 4.5 ES6 Module version using Subsystem pattern + Event System

import {Subsystem} from '../core/Subsystem.js';
import {KEYCODE_F1, KEYCODE_F2, KEYCODE_SPACE, KEYCODE_ESC} from '../InputSystemCommon.js';
import {GameState} from "../JSGame.js";
import {jsGameInstance} from "../main.js";
import {EventTypes} from "../core/EventTypes.js";
import {GameStateChangedEvent} from "../events/GameStateChangedEvent.js";

/**
 * InputSystem - Handles all input-related functionality
 * Phase 4.5 ES6 Module using Subsystem pattern + Event System (DIP)
 *
 * Features:
 * - F1 key debugging toggle (rendering on/off)
 * - F2 key GameObject system toggle (enable/disable new component-based system)
 * - Spacebar game state transitions
 * - Event-based communication (NO direct AudioSystem dependency)
 *
 * Architecture:
 * - Emits GameStateChangedEvent when state transitions occur
 * - AudioSystem subscribes to events independently
 * - Dependency Inversion Principle (DIP) achieved
 */
export class InputSystem extends Subsystem
{
    constructor()
    {
        super('inputSystem', 10, {enabled: true});

        this.logTimer = 0;

        console.log('InputSystem: Module loaded (Phase 4.5 ES6 + Event System)');
    }

    /**
     * Update method - called every frame
     * @param {number} gameDelta - Game time delta
     * @param {number} systemDelta - System time delta
     */
    update(gameDelta, systemDelta)
    {
        // Accumulate time for logging
        this.logTimer += systemDelta;

        // Periodic logging (every 300ms)
        if (this.logTimer >= 300)
        {
            console.log('InputSystem: HandleInput active');
            this.logTimer = 0;
        }

        // F1 key: Toggle rendering debug mode
        if (this.wasKeyJustPressed(KEYCODE_F1))
        {
            globalThis.shouldRender = !globalThis.shouldRender;
            console.log('InputSystem: F1 pressed, shouldRender =', globalThis.shouldRender);
        }

        // Game state transitions
        if (typeof jsGameInstance !== 'undefined')
        {
            if (jsGameInstance.gameState === GameState.ATTRACT)
            {
                // SPACE: ATTRACT → GAME transition
                if (this.wasKeyJustPressed(KEYCODE_SPACE))
                {
                    const oldState = jsGameInstance.gameState;
                    const newState = GameState.GAME;

                    // Change game state
                    jsGameInstance.setGameState(newState);

                    // ✅ DEPENDENCY INVERSION: Emit event instead of calling AudioSystem directly
                    // AudioSystem will subscribe to this event and play sound
                    const event = new GameStateChangedEvent(oldState, newState, 'InputSystem');
                    globalThis.eventBus.emit(EventTypes.GAME_STATE_CHANGED, event);

                    console.log('InputSystem: Emitted GameStateChangedEvent (ATTRACT → GAME)');
                }
            }

            if (jsGameInstance.gameState === GameState.GAME)
            {
                // ESC: GAME → ATTRACT transition
                if (this.wasKeyJustPressed(KEYCODE_ESC))
                {
                    const oldState = jsGameInstance.gameState;
                    const newState = GameState.ATTRACT;

                    // Change game state
                    jsGameInstance.setGameState(newState);

                    // ✅ DEPENDENCY INVERSION: Emit event
                    const event = new GameStateChangedEvent(oldState, newState, 'InputSystem');
                    globalThis.eventBus.emit(EventTypes.GAME_STATE_CHANGED, event);

                    console.log('InputSystem: Emitted GameStateChangedEvent (GAME → ATTRACT)');
                }
            }
        }
    }

    wasKeyJustPressed(keyCode)
    {
        if (typeof input !== 'undefined')
        {
            return input.wasKeyJustPressed(keyCode);
        }
    }
}

// Export for ES6 module system
export default InputSystem;

// Export to globalThis for hot-reload detection
globalThis.InputSystem = InputSystem;

console.log('InputSystem: Component loaded (Phase 4 ES6)');
