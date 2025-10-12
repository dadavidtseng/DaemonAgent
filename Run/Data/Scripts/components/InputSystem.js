// InputSystem.js
// Phase 4.5 ES6 Module version using Subsystem pattern + Event System

import {Subsystem} from '../core/Subsystem.js';
import {KEYCODE_F1, KEYCODE_F2, KEYCODE_SPACE, KEYCODE_ESC, KEYCODE_E} from '../InputSystemCommon.js';
import {GameState} from "../JSGame.js";
import {jsGameInstance} from "../main.js";
import {EventTypes} from "../events/EventTypes.js";
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

            if(this.wasKeyJustPressed(KEYCODE_E))
            {
                console.log('🧪 Phase 4 Heartbeat Test - Connection Monitoring');
                console.log('================================================\n');

                // Generate Ed25519 keys
                console.log('[1] Generating Ed25519 key pair...');
                const keyPair = kadi.generateKeyPair();
                console.log('✓ Keys generated\n');

                // Register connection state callback
                console.log('[2] Registering connection state callback...');
                let stateChanges = [];

                kadi.onConnectionStateChange(function(oldState, newState) {
                    const timestamp = new Date().toISOString().substring(11, 19);
                    console.log(`  [${timestamp}] State: ${oldState} → ${newState}`);
                    stateChanges.push({ time: timestamp, old: oldState, new: newState });
                });
                console.log('✓ Callback registered\n');

                // Connect to broker
                console.log('[3] Connecting to MockKADIBroker...');
                kadi.connect('ws://localhost:8080', keyPair.publicKey, keyPair.privateKey);
                console.log('✓ Connection initiated\n');

                // Register minimal tools
                console.log('[4] Registering test tool...');
                const tools = [{
                    name: 'test_heartbeat',
                    description: 'Test tool for heartbeat monitoring',
                    parameters: { type: 'object', properties: {} }
                }];

                kadi.registerTools(JSON.stringify(tools));
                console.log('✓ Tool registered\n');

                console.log('================================================');
                console.log('📊 HEARTBEAT MONITORING STARTED');
                console.log('================================================');
                console.log('Expected behavior:');
                console.log('  • PING sent every ~30 seconds');
                console.log('  • PONG response logged immediately');
                console.log('  • Connection remains stable');
                console.log('');
                console.log('Watch the C++ debug output for:');
                console.log('  "KADIWebSocketSubsystem: Sending heartbeat PING"');
                console.log('  "KADIWebSocketSubsystem: Received PONG response"');
                console.log('');
                console.log('⏱️  Wait at least 90 seconds to observe multiple heartbeats');
                console.log('');
                console.log('To disconnect: kadi.disconnect();');
                console.log('================================================\n');
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
