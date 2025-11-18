//----------------------------------------------------------------------------------------------------
// JSGame.js - Game System Coordinator
//----------------------------------------------------------------------------------------------------

import {CppBridgeSystem} from './components/CppBridgeSystem.js';
import {InputSystem} from './components/InputSystem.js';
import {AudioSystem} from './components/AudioSystem.js';
// Phase 2b: CameraSystem removed - replaced by CameraAPI
// import {CameraSystem} from './components/CameraSystem.js';
// Phase 2: RendererSystem removed - replaced by EntityAPI
// import {RendererSystem} from './components/RendererSystem.js';
import {DebugRenderSystem} from './components/DebugRenderSystem.js';
import {KADIGameControl} from './kadi/KADIGameControl.js';
import {PhysicsSystem} from './components/PhysicsSystem.js';  // Physics simulation system
import {KEYCODE_O, KEYCODE_P} from "./InputSystemCommon";
import {Player} from './objects/Player.js';
import {Prop} from './objects/Prop.js';
import {hotReloadRegistry} from './core/HotReloadRegistry.js';
import {CameraAPI} from './interfaces/CameraAPI.js';  // Phase 2b: Screen camera creation

// import {NewFeatureSystem} from './components/NewFeatureSystem.js';

export const GameState = Object.freeze({
    ATTRACT: 'ATTRACT',
    GAME: 'GAME',
    PAUSED: 'PAUSED'
});

// Vec2 constants (matching C++ Vec2 constants)
const Vec2 = {
    ZERO: {x: 0, y: 0},
    ONE: {x: 1, y: 1}
};

// Vec3 basis constants (matching C++ Vec3 basis vectors)
const Vec3 = {
    X_BASIS: {x: 1, y: 0, z: 0},
    Y_BASIS: {x: 0, y: 1, z: 0},
    Z_BASIS: {x: 0, y: 0, z: 1}
};

// Rgba8 color constants (matching C++ Rgba8 colors)
const Rgba8 = {
    RED: {r: 255, g: 0, b: 0, a: 255},
    GREEN: {r: 0, g: 255, b: 0, a: 255},
    BLUE: {r: 0, g: 0, b: 255, a: 255}
};

/**
 * Create a 4x4 transformation matrix from I, J, K basis vectors and translation T
 * Matches C++ Mat44::SetIJKT3D(I, J, K, T)
 *
 * @param {Object} I - Right vector {x, y, z}
 * @param {Object} J - Up vector {x, y, z}
 * @param {Object} K - Forward vector {x, y, z}
 * @param {Object} T - Translation {x, y, z}
 * @returns {Array<number>} 16-element matrix in column-major order
 */
function createTransformMatrix(I, J, K, T)
{
    return [
        I.x, I.y, I.z, 0,  // Column 0: Right (I basis)
        J.x, J.y, J.z, 0,  // Column 1: Up (J basis)
        K.x, K.y, K.z, 0,  // Column 2: Forward (K basis)
        T.x, T.y, T.z, 1   // Column 3: Translation
    ];
}

/**
 * Create identity matrix (matches C++ Mat44())
 */
function identityMatrix()
{
    return [
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    ];
}

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
    gameState;
    gameClock;
    screenCamera;

    constructor(engine)
    {
        console.log('(JSGame::constructor)(start) - Phase 4 ES6 Module pattern');
        this.engine = engine;

        // Phase 3.5: Create component instances
        this.createComponentInstances();

        // Register all component systems with JSEngine
        this.registerGameSystems();

        // Default game state: Start in ATTRACT mode
        // Press SPACE to transition to GAME mode
        this.gameState = GameState.ATTRACT;

        // Create game clock (uses ClockInterface wrapper for C++ clock)
        // Matches C++ Game::Game() which creates: m_gameClock = new Clock(Clock::GetSystemClock())
        try
        {
            this.gameClock = new Clock();
            console.log('JSGame: Game clock created successfully');
        } catch (error)
        {
            console.log('JSGame: ERROR - Clock creation failed:', error);
        }

        // === Debug Visualization Setup (migrated from C++ Game constructor) ===
        // Now with comprehensive error handling to prevent constructor failure
        this.setupDebugVisualization();

        // === Hot-Reload Version Tracking ===
        // Track versions for Player and Prop classes to detect hot-reloads
        this.playerVersion = hotReloadRegistry.getVersion('Player');
        this.propVersion = hotReloadRegistry.getVersion('Prop');

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

        // Phase 2b: CameraSystem removed - camera management now via CameraAPI
        // this.cameraSystem = new CameraSystem();

        // Audio system (priority: 5) - must create before InputSystem
        this.audioSystem = new AudioSystem();

        // Input system (priority: 10)
        this.inputSystem = new InputSystem();

        // Phase 2: RendererSystem removed - replaced by EntityAPI
        // === Phase 4: Renderer system (priority: 100) - must create BEFORE entities ===
        // this.rendererSystem = new RendererSystem();

        // Debug Render system (priority: 95) - debug visualization
        this.debugRenderSystem = new DebugRenderSystem();

        // === KADI Game Control (priority: 11) - game manipulation via KADI protocol ===
        this.kadiGameControl = new KADIGameControl(this);

        console.log('JSGame: KADIGameControl subsystem created successfully');

        // === Physics System (priority: 25) - physics simulation ===
        this.physicsSystem = new PhysicsSystem();

        console.log('JSGame: PhysicsSystem created successfully');

        // Phase 2b: Create screen-space camera for UI/debug rendering using CameraAPI
        this.screenCamera = null;  // Will be set in async callback
        this.cameraAPI = new CameraAPI();

        console.log('JSGame: Creating screen camera (Phase 2b CameraAPI)...');
        this.cameraAPI.createCamera(
            [0, 0, 0],  // position (not used for screen cameras)
            [0, 0, 0],  // orientation (not used for screen cameras)
            'screen',   // type: orthographic 2D camera for UI
            (cameraId) => {
                if (cameraId === 0) {
                    console.log('JSGame: ERROR - Screen camera creation failed!');
                    return;
                }

                this.screenCamera = cameraId;
                console.log('JSGame: Screen camera created with ID:', cameraId);

                // Note: Phase 2b CameraAPI auto-configures orthographic view for 'screen' type
                // No need for manual setOrthographicView/setNormalizedViewport calls
                console.log('JSGame: Screen camera ready (auto-configured for UI rendering)');
            }
        );

        // === Phase 4: Game entities (matching C++ architecture) ===
        // PlayerEntity (like C++ Player* m_player)
        try
        {
            console.log('JSGame: About to create PlayerEntity...');
            // this.playerEntity = new PlayerEntity(this);
            console.log('JSGame: PlayerEntity created successfully');
        } catch (error)
        {
            console.log('JSGame: ERROR creating PlayerEntity:', error);
            console.log('JSGame: Error stack:', error.stack);
            throw error;
        }

        // === Phase 5: GameObject system (new component-based architecture) ===
        // Create Player GameObject (F2 toggle system)
        try
        {
            console.log('JSGame: Creating Player GameObject (Phase 5)...');
            this.playerGameObject = new Player();
            console.log('JSGame: Player GameObject created successfully');
        } catch (error)
        {
            console.log('JSGame: ERROR creating Player GameObject:', error);
            console.log('JSGame: Error stack:', error.stack);
            throw error;
        }

        // Create Prop GameObjects (Phase 6: Prop migration)
        this.propGameObjects = [];
        try
        {
            console.log('JSGame: Creating Prop GameObjects (Phase 6)...');
            this.createPropGameObjects();
            console.log('JSGame: Prop GameObjects created successfully');

            // Create test physics props
            // this.createPhysicsTestProps();
        } catch (error)
        {
            console.log('JSGame: ERROR creating Prop GameObjects:', error);
            console.log('JSGame: Error stack:', error.stack);
            throw error;
        }

        // this.newFeature = new NewFeatureSystem();

        console.log('JSGame: All component instances created (Phase 4 with Entity structure + Phase 5 GameObject system)');
    }


    /**
     * Create the 4 Prop GameObjects (Phase 6: Prop migration)
     * Matches createProps() behavior with component-based architecture
     */
    createPropGameObjects()
    {
        console.log('JSGame: Creating 4 Prop GameObjects matching C++ Game behavior...');

        try {
            // Prop 0: Rotating cube at (2, 2, 0) - pitch+roll += 30°/s (Phase 2)
            const prop0 = new Prop('cube', {x: 2, y: 2, z: 0}, 'rotate-pitch-roll');
            this.propGameObjects.push(prop0);
            console.log('JSGame: Prop 0 created successfully');
        } catch (error) {
            console.log('JSGame: ERROR creating Prop 0:', error);
        }

        try {
            // Prop 1: Pulsing color cube at (-2, -2, 0) - sin wave color (Phase 2)
            const prop1 = new Prop('cube', {x: -2, y: -2, z: 0}, 'pulse-color');
            this.propGameObjects.push(prop1);
            console.log('JSGame: Prop 1 created successfully');
        } catch (error) {
            console.log('JSGame: ERROR creating Prop 1:', error);
        }

        try {
            // Prop 2: Rotating sphere at (10, -5, 1) - yaw += 45°/s (Phase 2)
            const prop2 = new Prop('sphere', {x: 10, y: -5, z: 1}, 'rotate-yaw');
            this.propGameObjects.push(prop2);
            console.log('JSGame: Prop 2 created successfully');
        } catch (error) {
            console.log('JSGame: ERROR creating Prop 2:', error);
        }

        try {
            // Prop 3: Static grid at (0, 0, 0) (Phase 2)
            const prop3 = new Prop('grid', {x: 0, y: 0, z: 0}, 'static');
            this.propGameObjects.push(prop3);
            console.log('JSGame: Prop 3 created successfully');
        } catch (error) {
            console.log('JSGame: ERROR creating Prop 3:', error);
        }

        console.log(`JSGame: Created ${this.propGameObjects.length} Prop GameObjects (Phase 2)`);
    }

    /**
     * Create test props with bounce physics enabled
     * Experimental feature for testing physics system
     * Spawns 100 cubes/spheres in a grid pattern with random heights
     */
    createPhysicsTestProps()
    {
        console.log('JSGame: Creating 100 physics test props...');

        const numObjects = 100;
        const gridSize = 10;  // 10x10 grid
        const spacing = 3.0;   // 3 units between objects
        const startX = -15.0;  // Start offset to center the grid
        const startY = -15.0;

        let createdCount = 0;

        for (let i = 0; i < numObjects; i++)
        {
            try {
                // Calculate grid position
                const gridX = i % gridSize;
                const gridY = Math.floor(i / gridSize);

                // World position
                const posX = startX + (gridX * spacing);
                const posY = startY + (gridY * spacing);
                const posZ = 5 + Math.random() * 10;  // Random height between 5 and 15

                // Alternate between cube and sphere
                const meshType = (i % 2 === 0) ? 'cube' : 'sphere';

                // Random color
                const color = {
                    r: Math.floor(Math.random() * 256),
                    g: Math.floor(Math.random() * 256),
                    b: Math.floor(Math.random() * 256),
                    a: 255
                };

                // Random physics parameters
                const bounciness = 0.5 + Math.random() * 0.4;  // 0.5 to 0.9
                const scale = 0.5 + Math.random() * 0.5;       // 0.5 to 1.0

                // Random initial velocity to ensure collisions
                const velocityX = (Math.random() - 0.5) * 4.0;  // -2 to +2 m/s
                const velocityY = (Math.random() - 0.5) * 4.0;  // -2 to +2 m/s
                const velocityZ = (Math.random() - 0.5) * 2.0;  // -1 to +1 m/s

                const physicsObject = new Prop(
                    meshType,
                    {x: posX, y: posY, z: posZ},
                    'static',
                    color,
                    scale,
                    {
                        enablePhysics: true,
                        physicsConfig: {
                            gravity: -9.8,
                            bounciness: bounciness,
                            mass: 1.0,
                            initialVelocity: [velocityX, velocityY, velocityZ]
                        }
                    }
                );

                this.propGameObjects.push(physicsObject);
                createdCount++;

            } catch (error) {
                console.log(`JSGame: ERROR creating physics test object ${i}:`, error);
            }
        }

        console.log(`JSGame: Created ${createdCount} physics test props (10x10 grid pattern)`);
    }

    /**
     * Setup debug visualization (migrated from C++ Game constructor)
     * Adds world basis, axis labels, and test screen text
     */
    setupDebugVisualization()
    {
        console.log('JSGame: Setting up debug visualization...');

        // Check if debugRenderSystem is initialized
        if (!this.debugRenderSystem || !this.debugRenderSystem.isInitialized)
        {
            console.log('JSGame: DebugRenderSystem not initialized, skipping debug visualization setup');
            return;
        }

        try
        {
            // Add world coordinate basis at origin
            // C++: DebugAddWorldBasis(Mat44(), -1.f);
            console.log('JSGame: Adding world basis...');
            this.debugRenderSystem.addWorldBasis(identityMatrix(), -1.0, "USE_DEPTH");
            console.log('JSGame: World basis added successfully');

            // Note: Vec2, Vec3, Rgba8 constants now defined at module level (top of file)

            // Add "X-Forward" label
            console.log('JSGame: Adding X-Forward label...');
            let transform = createTransformMatrix(
                {x: -Vec3.Y_BASIS.x, y: -Vec3.Y_BASIS.y, z: -Vec3.Y_BASIS.z},  // -Y_BASIS
                Vec3.X_BASIS,                                                    // X_BASIS
                Vec3.Z_BASIS,                                                    // Z_BASIS
                {x: 0.25, y: 0.0, z: 0.25}                                      // Translation
            );
            this.debugRenderSystem.addWorldText(
                "X-Forward",
                transform,
                0.25,           // textHeight
                Vec2.ONE.x,     // alignX
                Vec2.ONE.y,     // alignY
                -1.0,           // duration (permanent)
                Rgba8.RED.r, Rgba8.RED.g, Rgba8.RED.b, Rgba8.RED.a,  // Color
                "USE_DEPTH"     // mode
            );
            console.log('JSGame: X-Forward label added successfully');

            // Add "Y-Left" label
            console.log('JSGame: Adding Y-Left label...');
            transform = createTransformMatrix(
                {x: -Vec3.X_BASIS.x, y: -Vec3.X_BASIS.y, z: -Vec3.X_BASIS.z},  // -X_BASIS
                {x: -Vec3.Y_BASIS.x, y: -Vec3.Y_BASIS.y, z: -Vec3.Y_BASIS.z},  // -Y_BASIS
                Vec3.Z_BASIS,                                                    // Z_BASIS
                {x: 0.0, y: 0.25, z: 0.5}                                       // Translation
            );
            this.debugRenderSystem.addWorldText(
                "Y-Left",
                transform,
                0.25,           // textHeight
                Vec2.ZERO.x,    // alignX
                Vec2.ZERO.y,    // alignY
                -1.0,           // duration (permanent)
                Rgba8.GREEN.r, Rgba8.GREEN.g, Rgba8.GREEN.b, Rgba8.GREEN.a,  // Color
                "USE_DEPTH"     // mode
            );
            console.log('JSGame: Y-Left label added successfully');

            // Add "Z-Up" label
            console.log('JSGame: Adding Z-Up label...');
            transform = createTransformMatrix(
                {x: -Vec3.X_BASIS.x, y: -Vec3.X_BASIS.y, z: -Vec3.X_BASIS.z},  // -X_BASIS
                Vec3.Z_BASIS,                                                    // Z_BASIS
                Vec3.Y_BASIS,                                                    // Y_BASIS
                {x: 0.0, y: -0.25, z: 0.25}                                     // Translation
            );
            this.debugRenderSystem.addWorldText(
                "Z-Up",
                transform,
                0.25,           // textHeight
                1.0,            // alignX
                0.0,            // alignY
                -1.0,           // duration (permanent)
                Rgba8.BLUE.r, Rgba8.BLUE.g, Rgba8.BLUE.b, Rgba8.BLUE.a,  // Color
                "USE_DEPTH"     // mode
            );
            console.log('JSGame: Z-Up label added successfully');

            // Add screen text
            console.log('JSGame: Adding screen text...');
            // Removed static "TEST" text - will be added dynamically based on game mode in gameRender()
            console.log('JSGame: Screen text setup complete (dynamic mode-based text)');

            console.log('JSGame: Debug visualization setup complete');
        } catch (error)
        {
            console.log('JSGame: ERROR - Error setting up debug visualization:', error);
            console.log('JSGame: ERROR - Error message:', error.message);
            console.log('JSGame: ERROR - Error stack:', error.stack);
            // Don't rethrow - allow constructor to complete even if debug viz fails
        }
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
        // Phase 2b: CameraSystem removed - camera management now via CameraAPI
        // this.engine.registerSystem(null, this.cameraSystem);    // Priority: 3
        this.engine.registerSystem(null, this.audioSystem);     // Priority: 5
        this.engine.registerSystem(null, this.inputSystem);     // Priority: 10
        this.engine.registerSystem(null, this.kadiGameControl);  // Priority: 11
        // this.engine.registerSystem(null, this.newFeature);     // Priority: 10

        // === Physics System (priority: 25) - Updates entity positions ===
        this.engine.registerSystem('physicsSystem', {
            update: (gameDelta, systemDelta) =>
            {
                // Convert systemDelta from seconds to milliseconds
                const deltaTimeMs = systemDelta * 1000.0;
                this.physicsSystem.update(deltaTimeMs);
            },
            render: () =>
            {
            },
            priority: 25,
            enabled: true,
            data: {}
        });

        // === Phase 4: Entity update/render systems ===
        // Game update system (priority: 12) - Updates PlayerEntity and PropEntities
        this.engine.registerSystem('gameUpdate', {
            update: (gameDelta, systemDelta) =>
            {
                // Convert systemDelta from seconds to milliseconds
                const deltaTimeMs = systemDelta * 1000.0;

                // === HOT-RELOAD DETECTION: Check for Player class updates ===
                if (hotReloadRegistry.hasUpdated('Player', this.playerVersion))
                {
                    const newVersion = hotReloadRegistry.getVersion('Player');

                    // Get updated Player class from registry
                    const PlayerClass = hotReloadRegistry.getClass('Player');

                    // Recreate player GameObject with new class
                    this.playerGameObject = new PlayerClass();

                    // Update tracked version
                    this.playerVersion = newVersion;
                }

                // === HOT-RELOAD DETECTION: Check for Prop class updates ===
                if (hotReloadRegistry.hasUpdated('Prop', this.propVersion))
                {
                    const newVersion = hotReloadRegistry.getVersion('Prop');

                    // Get updated Prop class from registry
                    const PropClass = hotReloadRegistry.getClass('Prop');

                    // Recreate all prop GameObjects with new class (Phase 2: no rendererSystem parameter)
                    this.propGameObjects = [];
                    this.propGameObjects.push(new PropClass('cube', {x: 2, y: 2, z: 0}, 'rotate-pitch-roll'));
                    this.propGameObjects.push(new PropClass('cube', {x: -2, y: -2, z: 0}, 'pulse-color'));
                    this.propGameObjects.push(new PropClass('sphere', {x: 10, y: -5, z: 1}, 'rotate-yaw'));
                    this.propGameObjects.push(new PropClass('grid', {x: 0, y: 0, z: 0}, 'static'));

                    // Update tracked version
                    this.propVersion = newVersion;
                }

                // New component-based Player GameObject
                if (this.playerGameObject)
                {
                    this.playerGameObject.update(deltaTimeMs);
                }

                // Update Prop GameObjects (Phase 2: High-level entity API)
                if (this.propGameObjects && Array.isArray(this.propGameObjects))
                {
                    for (const prop of this.propGameObjects)
                    {
                        if (prop && typeof prop.update === 'function')
                        {
                            prop.update(deltaTimeMs);
                        }
                        else if (!prop)
                        {
                            console.log('JSGame: ERROR - Prop in propGameObjects array is null/undefined!');
                        }
                    }
                }


                // P: Pause game clock
                if (input.wasKeyJustPressed(KEYCODE_P))
                {
                    this.engine.setSystemEnabled('gameRender', false);
                }

                if (input.wasKeyJustPressed(KEYCODE_O))
                {
                    this.engine.setSystemEnabled('gameRender', true);
                }

                if (input.wasKeyJustPressed(KEYCODE_ESC))
                {
                    game.appRequestQuit();
                }
            },
            render: () =>
            {
            },
            priority: 12,
            enabled: true,
            data: {}
        });

        // Game render system (priority: 90) - Renders scene with player's camera
        let renderFrameCount = 0;  // Track render calls
        this.engine.registerSystem('gameRender', {
            update: (gameDelta, systemDelta) =>
            {
            },
            render: () =>
            {
                renderFrameCount++;

                // Add game mode-specific screen text FIRST (before any early returns)
                // This ensures text is rendered in both ATTRACT and GAME modes
                if (this.gameState === GameState.ATTRACT)
                {
                    // ATTRACT mode: Show "Attract" text
                    this.debugRenderSystem.addScreenText(
                        "Attract",
                        10,             // x (10 pixels from left)
                        10,             // y (10 pixels from bottom)
                        30.0,           // size (larger for attract mode)
                        Vec2.ZERO.x,    // alignX
                        Vec2.ZERO.y,    // alignY
                        0,             // duration (-1 = infinite, persists until cleared)
                        255, 255, 0, 255  // Yellow color
                    );
                }
                else if (this.gameState === GameState.GAME)
                {
                    // GAME mode: Show "Game" text
                    this.debugRenderSystem.addScreenText(
                        "Game",
                        10,             // x (10 pixels from left)
                        10,             // y (10 pixels from bottom)
                        30.0,           // size
                        Vec2.ZERO.x,    // alignX
                        Vec2.ZERO.y,    // alignY
                        0,             // duration (-1 = infinite, persists until cleared)
                        0, 255, 0, 255  // Green color
                    );
                }

                // Render screen text using screen camera (works in both modes)
                this.debugRenderSystem.renderScreen(this.screenCamera);

                // Only render JavaScript entities when in GAME mode
                // (CppBridgeSystem handles ATTRACT mode rendering)
                if (this.gameState !== GameState.GAME)
                {
                    return;
                }

                // Check global shouldRender flag (F1 toggle functionality)
                let shouldRenderValue = true;
                if (typeof globalThis.shouldRender !== 'undefined')
                {
                    shouldRenderValue = globalThis.shouldRender;
                }

                if (!shouldRenderValue)
                {
                    if (renderFrameCount % 60 === 0)
                    {
                        console.log('JSGame gameRender: Skipping render (shouldRender=false, F1 toggle)');
                    }
                    return;
                }

                // Use player's camera for rendering (F2 toggle: GameObject vs Entity)
                let camera;

                camera = this.playerGameObject ? this.playerGameObject.getCamera() : null;


                if (!camera)
                {
                    console.log('JSGame: ERROR - Player camera not available!');
                    return;
                }

                // Render debug visualization (use same camera as main rendering)
                this.debugRenderSystem.renderWorld(camera);

                // Phase 2: No need for begin/end camera calls - C++ handles rendering automatically
                // renderer.beginCamera(camera);  // REMOVED - replaced by EntityAPI system


                // Render Prop GameObjects (Phase 2: High-level entity API)
                // JavaScript calls prop.render() but actual rendering happens in C++ via EntityStateBuffer
                if (this.propGameObjects && Array.isArray(this.propGameObjects))
                {
                    for (const prop of this.propGameObjects)
                    {
                        if (prop && typeof prop.render === 'function')
                        {
                            prop.render();
                        }
                        else if (!prop)
                        {
                            console.log('JSGame: ERROR - Prop in propGameObjects array is null/undefined during render!');
                        }
                    }
                }


                // Phase 2: No need for end camera call - C++ handles rendering automatically
                // renderer.endCamera(camera);  // REMOVED - replaced by EntityAPI system
            },
            priority: 90,
            enabled: true,
            data: {}
        });

        // === Phase 4: Debug Render system (priority: 95) - debug visualization ===
        this.engine.registerSystem(null, this.debugRenderSystem);  // Priority: 95

        // Phase 2: RendererSystem removed - replaced by EntityAPI
        // === Phase 4: Renderer system (priority: 100) - renders LAST ===
        // this.engine.registerSystem(null, this.rendererSystem);  // Priority: 100 - REMOVED
        // this.engine.registerSystem(null, this.newFeature);

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
    setGameState(newState)
    {
        const validStates = Object.values(GameState);  // Change this line
        if (!validStates.includes(newState))
        {
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

    /**
     * Add entity to physics system
     * @param {number} entityId - Entity ID from EntityAPI
     * @param {Object} config - Physics configuration
     */
    addPhysics(entityId, config = {})
    {
        if (!this.physicsSystem)
        {
            console.log('JSGame: ERROR - PhysicsSystem not initialized');
            return;
        }

        this.physicsSystem.addEntity(entityId, config);
    }

    /**
     * Remove entity from physics system
     * @param {number} entityId - Entity ID to remove
     */
    removePhysics(entityId)
    {
        if (!this.physicsSystem)
        {
            return;
        }

        this.physicsSystem.removeEntity(entityId);
    }

    /**
     * Get physics system instance (for direct access)
     */
    getPhysicsSystem()
    {
        return this.physicsSystem;
    }
}

console.log('JSGame: Module loaded (Phase 4 ES6)');
