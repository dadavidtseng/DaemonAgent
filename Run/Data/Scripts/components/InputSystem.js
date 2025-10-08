// InputSystem.js
// Phase 4 ES6 Module version using Subsystem pattern

import {Subsystem} from '../core/Subsystem.js';
import AudioSystem from "./AudioSystem.js";
import {KEYCODE_F1, KEYCODE_SPACE, KEYCODE_P} from '../InputSystemCommon.js';

/**
 * InputSystem - Handles all input-related functionality
 * Phase 4 ES6 Module using Subsystem pattern
 *
 * Features:
 * - F1 key debugging toggle
 * - Spacebar game state transitions
 * - Audio system integration
 */
export class InputSystem extends Subsystem
{
    constructor()
    {
        super('inputSystem', 10, {enabled: true});

        this.audioSystem = new AudioSystem();
        this.logTimer = 0;

        console.log('InputSystem: Module loaded (Phase 4 ES6)');
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
        // console.log('XXXXXXXX WORKS!!!');
        // Periodic logging (every 300ms)
        if (this.logTimer >= 300)
        {
            console.log('MODIFY InputSystem HandleInput');

            this.logTimer = 0;
        }

        if (this.wasKeyJustPressed(KEYCODE_F1))
        {
            shouldRender = !shouldRender;
            console.log('InputSystem: F1 pressed, shouldRender =', shouldRender);
        }

        if (typeof game !== 'undefined')
        {
            if (game.gameState === 'ATTRACT')
            {
                if (this.wasKeyJustPressed(KEYCODE_SPACE))
                {
                    // Play click sound effect when changing from ATTRACT to GAME
                    if (this.audioSystem)
                    {
                        const clickSound = this.audioSystem.createOrGetSound("Data/Audio/TestSound.mp3", "Sound2D");
                        if (clickSound !== null && clickSound !== undefined)
                        {
                            this.audioSystem.startSound(clickSound);
                        }
                    }

                    game.gameState = 'GAME';
                }
            }

            if (game.gameState === 'GAME')
            {
                if (this.wasKeyJustPressed(KEYCODE_ESC))
                {
                    game.gameState = 'ATTRACT';
                }

                if (this.wasKeyJustPressed(KEYCODE_P))
                {
                    game.pauseGameClock();
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
