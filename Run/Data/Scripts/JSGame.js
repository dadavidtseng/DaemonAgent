//----------------------------------------------------------------------------------------------------
// JSGame.js - Game System Coordinator (Phase 4 ES6 Module Architecture)
//----------------------------------------------------------------------------------------------------


import {CppBridgeSystem} from './components/CppBridgeSystem.js';
import {InputSystem} from './components/InputSystem.js';
import {AudioSystem} from './components/AudioSystem.js';
import {CameraSystem} from './components/CameraSystem.js';
import {RendererSystem} from './components/RendererSystem.js';

// === Phase 4: Entity classes (matching C++ structure) ===
import {PlayerEntity} from './entities/PlayerEntity.js';
import {PropEntity} from './entities/PropEntity.js';
import {NewFeatureSystem} from "./components/NewFeatureSystem.js";
import {jsGameInstance} from "./main";

/**
 * JSGame - Game system coordinator
 *
 * Responsibilities:
 * - Create all game system instances
 * - Register systems with JSEngine
 * - Coordinate system lifecycle
 * - Manage game entities (PlayerEntity, PropEntity instances)
 *
 * Architecture:
 * - PlayerEntity: Single instance (like C++ Player* m_player)
 * - PropEntity[]: Multiple instances (like C++ std::vector<Prop*> m_props)
 * - Uses EntityBase architecture (not Subsystem for entities)
 */

export class JSGame
{
    constructor(engine)
    {
        console.log('(JSGame::constructor)(start) - Phase 4 ES6 Module pattern');
        this.engine = engine;

        // Phase 3.5: Create component instances
        this.createComponentInstances();

        // Register all component systems with JSEngine
        this.registerGameSystems();

        this.gameState = 'ATTRACT';  // 'ATTRACT', 'GAME', 'PAUSED'
        globalThis.jsGameInstance = this;
        console.log('(JSGame::constructor)(end) - All components registered');
    }

    /**
     * Create all component system instances
     * Phase 4: Pure ES6 Module imports with Subsystem pattern
     */
    createComponentInstances()
    {
        console.log('JSGame: Creating component instances...');

        // Core C++ bridge (priority: 0)
        this.cppBridge = new CppBridgeSystem(this.engine);

        // Camera system (priority: 3) - Initialize camera interface early
        this.cameraSystem = new CameraSystem();

        // Audio system (priority: 5) - must create before InputSystem
        this.audioSystem = new AudioSystem();

        // Input system (priority: 10)
        this.inputSystem = new InputSystem();

        // === Phase 4: Renderer system (priority: 100) - must create BEFORE entities ===
        this.rendererSystem = new RendererSystem();

        // === Phase 4: Game entities (matching C++ architecture) ===
        // PlayerEntity (like C++ Player* m_player)
        try {
            console.log('JSGame: About to create PlayerEntity...');
            this.playerEntity = new PlayerEntity(this);
            console.log('JSGame: PlayerEntity created successfully');
        } catch (error) {
            console.error('JSGame: ERROR creating PlayerEntity:', error);
            console.error('JSGame: Error stack:', error.stack);
            throw error;
        }

        // PropEntity array (like C++ std::vector<Prop*> m_props)
        this.props = [];
        try {
            console.log('JSGame: About to create props...');
            this.createProps();
            console.log('JSGame: Props created successfully');
        } catch (error) {
            console.error('JSGame: ERROR creating props:', error);
            console.error('JSGame: Error stack:', error.stack);
            throw error;
        }

        this.newFeature = new NewFeatureSystem();
        console.log('JSGame: All component instances created (Phase 4 with Entity structure)');
    }

    /**
     * Create the 4 props (matches C++ Game::SpawnProps behavior)
     */
    createProps()
    {
        console.log('JSGame: Creating 4 props matching C++ Game behavior...');

        // Prop 0: Rotating cube at (2, 2, 0) - pitch+roll += 30°/s
        const prop0 = new PropEntity(this, 'cube', { x: 2, y: 2, z: 0 }, this.rendererSystem);
        prop0.setBehavior('rotate-pitch-roll');
        this.props.push(prop0);

        // Prop 1: Pulsing color cube at (-2, -2, 0) - sin wave color
        const prop1 = new PropEntity(this, 'cube', { x: -2, y: -2, z: 0 }, this.rendererSystem);
        prop1.setBehavior('pulse-color');
        this.props.push(prop1);

        // Prop 2: Rotating sphere at (10, -5, 1) - yaw += 45°/s
        const prop2 = new PropEntity(this, 'sphere', { x: 10, y: -5, z: 1 }, this.rendererSystem);
        prop2.setBehavior('rotate-yaw');
        this.props.push(prop2);

        // Prop 3: Static grid at (0, 0, 0)
        const prop3 = new PropEntity(this, 'grid', { x: 0, y: 0, z: 0 }, this.rendererSystem);
        prop3.setBehavior('static');
        this.props.push(prop3);

        console.log(`JSGame: Created ${this.props.length} props`);
    }

    /**
     * Register all game systems with the engine
     * Phase 4: Mixed pattern - Subsystems and Entity wrappers
     */
    registerGameSystems()
    {
        if (this.engine == null || this.engine.registerSystem == null)
        {
            console.log('JSGame: Engine does not support system registration, using legacy mode');
            return;
        }

        console.log('(JSGame::registerGameSystems)(start) - Phase 4 Entity Structure');

        // Register subsystems using Subsystem pattern (null, componentInstance)
        this.engine.registerSystem(null, this.cppBridge);       // Priority: 0
        this.engine.registerSystem(null, this.cameraSystem);    // Priority: 3
        this.engine.registerSystem(null, this.audioSystem);     // Priority: 5
        this.engine.registerSystem(null, this.inputSystem);     // Priority: 10

        // === Phase 4: Entity update/render systems ===
        // Game update system (priority: 12) - Updates PlayerEntity and PropEntities
        this.engine.registerSystem('gameUpdate', {
            update: (gameDelta, systemDelta) => {
                // Update player
                this.playerEntity.update(gameDelta);

                // Update all props
                for (const prop of this.props) {
                    prop.update(gameDelta);
                }
            },
            render: () => {},
            priority: 12,
            enabled: true,
            data: {}
        });

        // Game render system (priority: 90) - Renders scene with player's camera
        let renderFrameCount = 0;  // Track render calls
        this.engine.registerSystem('gameRender', {
            update: (gameDelta, systemDelta) => {},
            render: () => {
                renderFrameCount++;

                // Only render JavaScript entities when in GAME mode
                // (CppBridgeSystem handles ATTRACT mode rendering)
                if (this.gameState !== 'GAME') {
                    return;
                }

                // Check global shouldRender flag (F1 toggle functionality)
                let shouldRenderValue = true;
                if (typeof globalThis.shouldRender !== 'undefined') {
                    shouldRenderValue = globalThis.shouldRender;
                }

                if (!shouldRenderValue) {
                    if (renderFrameCount % 60 === 0) {
                        console.log('JSGame gameRender: Skipping render (shouldRender=false, F1 toggle)');
                    }
                    return;
                }

                // Use player's camera for rendering
                const camera = this.playerEntity.getCamera();
                if (!camera) {
                    console.error('JSGame: Player camera not available!');
                    return;
                }

                // Log every 60 frames to avoid spam
                if (renderFrameCount % 60 === 0) {
                    console.log(`JSGame gameRender: Frame ${renderFrameCount}, camera=${camera}, props.length=${this.props.length}`);
                }

                // Begin camera rendering
                renderer.beginCamera(camera);

                // Render all props
                for (const prop of this.props) {
                    prop.render();
                }

                // End camera rendering
                renderer.endCamera(camera);
            },
            priority: 90,
            enabled: true,
            data: {}
        });

        // === Phase 4: Renderer system (priority: 100) - renders LAST ===
        this.engine.registerSystem(null, this.rendererSystem);  // Priority: 100
        this.engine.registerSystem(null, this.newFeature);

        console.log('(JSGame::registerGameSystems)(end) - All systems registered (Entity-based architecture)');
    }

    // ============================================================================
    // AI AGENT API - For runtime system control
    // ============================================================================

    /**
     * Check if game is in attract mode (for PlayerEntity)
     * Matches C++ Game::IsAttractMode()
     */
    isAttractMode()
    {
        // For now, always return false (not in attract mode)
        // TODO: Implement game state management if needed
        return this.gameState === 'ATTRACT';
    }

    /**
     * Set game state
     */
    setGameState(newState) {
        const validStates = ['ATTRACT', 'GAME', 'PAUSED'];
        if (!validStates.includes(newState)) {
            console.log(`JSGame: Invalid game state '${newState}'`);
            return;
        }

        console.log(`JSGame: Game state changed from ${this.gameState} to ${newState}`);
        this.gameState = newState;
    }

    /**
     * Enable input system
     */
    enableInput()
    {
        return this.engine.setSystemEnabled('inputSystem', true);
    }

    /**
     * Disable input system
     */
    disableInput()
    {
        return this.engine.setSystemEnabled('inputSystem', false);
    }

    /**
     * Check if input system is enabled
     */
    isInputEnabled()
    {
        const system = this.engine.getSystem('inputSystem');
        return system ? system.enabled : false;
    }

    /**
     * Enable audio system
     */
    enableAudio()
    {
        return this.engine.setSystemEnabled('audioSystem', true);
    }

    /**
     * Disable audio system
     */
    disableAudio()
    {
        return this.engine.setSystemEnabled('audioSystem', false);
    }

    /**
     * Check if audio system is enabled
     */
    isAudioEnabled()
    {
        const system = this.engine.getSystem('audioSystem');
        return system ? system.enabled : false;
    }

    /**
     * Get audio system status
     */
    getAudioStatus()
    {
        return this.audioSystem ? this.audioSystem.getSystemStatus() : null;
    }

    /**
     * Register new system at runtime (for AI agents)
     */
    registerSystem(id, config)
    {
        return this.engine.registerSystem(id, config);
    }

    /**
     * Unregister system at runtime (for AI agents)
     */
    unregisterSystem(id)
    {
        return this.engine.unregisterSystem(id);
    }
}

console.log('JSGame: Module loaded (Phase 4 ES6)');
