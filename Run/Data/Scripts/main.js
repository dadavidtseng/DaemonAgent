//----------------------------------------------------------------------------------------------------
// main.js - JavaScript Framework Entry Point
//----------------------------------------------------------------------------------------------------

/**
 * Main entry point for the JavaScript framework
 *
 * Architecture:
 * - Single entry point loaded by C++
 * - ES6 module imports for clean dependency management
 * - Initializes JSEngine and JSGame
 * - Sets up global references for C++ bridge and hot-reload
 *
 * Loading Order:
 * 1. C++ loads main.js (ES6 module - this file)
 *    ↳ Imports InputSystemCommon.js (key code constants)
 *    ↳ Imports JSEngine.js
 *    ↳ Imports JSGame.js
 *       ↳ Imports InputSystem.js, AudioSystem.js
 *       ↳ Imports CppBridgeSystem.js, CubeSpawner.js, PropMover.js, CameraShaker.js
 */

import './InputSystemCommon.js'; // Global key code constants
import { JSEngine } from './JSEngine.js';
import { JSGame } from './JSGame.js';
import { CommandQueue } from './Interface/CommandQueue.js';  // GenericCommand facade

console.log('(main.js)(start) - JavaScript Framework Entry Point');

// Create JSEngine instance
const jsEngineInstance = new JSEngine();

// Create JSGame instance
const jsGameInstance = new JSGame(jsEngineInstance);

// Set game instance in engine
jsEngineInstance.setGame(jsGameInstance);

// ============================================================================
// GLOBAL REFERENCES (for C++ bridge and hot-reload)
// ============================================================================

// REQUIRED: C++ calls JSEngine.update() and JSEngine.render() through globalThis
globalThis.JSEngine = jsEngineInstance;

// REQUIRED: Hot-reload system needs global reference to JSGame instance
globalThis.jsGameInstance = jsGameInstance;

// REQUIRED: F1 toggle functionality (rendering control)
if (typeof globalThis.shouldRender === 'undefined') {
    globalThis.shouldRender = true;
}

// ============================================================================
// GENERIC COMMAND SYSTEM (CommandQueue instantiation)
// ============================================================================

const commandQueueInstance = new CommandQueue();
globalThis.CommandQueueAPI = commandQueueInstance;  // Global reference for EntityAPI.createMeshViaCommand
console.log('CommandQueue: Instance created, available:', commandQueueInstance.isAvailable());

// === Smoke Test: Round-trip verification (Task 7.0) ===
// Submit a "ping" command to verify JS → C++ → JS callback pipeline
if (commandQueueInstance.isAvailable()) {
    // Test 1: Valid "ping" command — expects {pong: "hello"} callback
    commandQueueInstance.submit('ping', {message: 'hello'}, 'smoke-test', (result) => {
        console.log('=== GenericCommand SMOKE TEST: ping callback received ===');
        console.log('  success:', result.success);
        console.log('  resultId:', result.resultId);
        console.log('  error:', result.error);
        console.log('=== SMOKE TEST COMPLETE ===');
    });
    console.log('CommandQueue: Smoke test "ping" submitted');

    // Test 2: Unregistered command — expects ERR_NO_HANDLER callback
    commandQueueInstance.submit('unknown_cmd', {}, 'smoke-test', (result) => {
        console.log('=== GenericCommand ERROR PATH TEST: unknown_cmd callback ===');
        console.log('  success:', result.success, '(expected: false)');
        console.log('  error:', result.error, '(expected: ERR_NO_HANDLER or similar)');
        console.log('=== ERROR PATH TEST COMPLETE ===');
    });
    console.log('CommandQueue: Smoke test "unknown_cmd" submitted');
} else {
    console.log('CommandQueue: C++ interface not available, skipping smoke test');
}

// === Smoke Test: create_mesh via GenericCommand (Task 8.3) ===
if (commandQueueInstance.isAvailable()) {
    commandQueueInstance.submit('create_mesh', {
        meshType: 'cube',
        position: [0, 0, 5],
        scale: 0.5,
        color: [0, 255, 0, 255]
    }, 'smoke-test', (result) => {
        console.log('=== GenericCommand CREATE_MESH TEST ===');
        console.log('  success:', result.success);
        console.log('  entityId (resultId):', result.resultId);
        console.log('  error:', result.error);
        console.log('=== CREATE_MESH TEST COMPLETE ===');
    });
    console.log('CommandQueue: Smoke test "create_mesh" submitted');
}

// ============================================================================
// STATUS LOGGING
// ============================================================================

console.log('JSGame: System registration framework initialized');
console.log('Available API: globalThis.JSEngine for system management');
console.log('Input system status:', jsGameInstance.isInputEnabled() ? 'ENABLED' : 'DISABLED');
console.log('Audio system status:', jsGameInstance.isAudioEnabled() ? 'ENABLED' : 'DISABLED');
console.log('Hot-reload system status:', jsEngineInstance.hotReloadEnabled ? 'AVAILABLE (C++)' : 'NOT AVAILABLE');

// ============================================================================
// KADI BROKER INTEGRATION STATUS
// ============================================================================

console.log('\n========================================');
console.log('KADI Broker Integration Status');
console.log('========================================');

if (typeof kadi !== 'undefined') {
    console.log('✓ KADI global object: REGISTERED');
    console.log('  Type:', typeof kadi);

    // Test key generation to verify functionality
    try {
        const testKeys = kadi.generateKeyPair();
        if (testKeys && testKeys.publicKey && testKeys.privateKey) {
            console.log('✓ KADI functionality: WORKING');
            console.log('  Public key length:', testKeys.publicKey.length, 'chars');
            console.log('  Private key length:', testKeys.privateKey.length, 'chars');
        } else {
            console.log('✗ KADI functionality: INVALID RESPONSE');
        }
    } catch (e) {
        console.log('✗ KADI functionality: ERROR -', e.message);
    }

    console.log('\nℹ  KADI will auto-connect via KADIGameControl subsystem');
    console.log('   Connection: ws://localhost:8080');
    console.log('   Tools registered: 15 (Game Control + Development)');
} else {
    console.log('✗ KADI global object: NOT REGISTERED');
    console.log('  This means KADIScriptInterface was not registered with ScriptSubsystem');
    console.log('  Check App::SetupScriptingBindings() for KADI registration code');
}

console.log('========================================\n');

console.log('(main.js)(end) - JavaScript framework initialized');

// Export for potential future use
export { jsEngineInstance, jsGameInstance };
