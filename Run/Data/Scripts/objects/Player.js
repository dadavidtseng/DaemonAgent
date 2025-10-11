//----------------------------------------------------------------------------------------------------
// Player.js
// Player GameObject - Demonstrates component-based architecture
// Inspired by Starship GameObject pattern
//----------------------------------------------------------------------------------------------------

import {GameObject} from '../core/GameObject.js';
import {KeyboardInputComponent} from '../components/input/KeyboardInputComponent.js';
import {FlyingMovementComponent} from '../components/movement/FlyingMovementComponent.js';
import {CameraComponent} from '../components/camera/CameraComponent.js';

/**
 * Player - Player GameObject with component composition
 *
 * Demonstrates the new component-based architecture:
 * - Extends GameObject base class
 * - Composes KeyboardInputComponent for input handling
 * - Can be extended with MovementComponent, WeaponComponent, etc.
 *
 * Implementation (Phase 1 + Phase 2):
 * - KeyboardInputComponent for input handling
 * - FlyingMovementComponent for WASD 3D movement
 * - CameraComponent for world camera management
 * - Can be toggled with F2 key
 *
 * Future Enhancements:
 * - WeaponComponent for shooting mechanics (deferred to end)
 * - HealthComponent for damage system
 *
 * Usage:
 * ```javascript
 * const player = new Player();
 * player.update(deltaTime); // Called every frame when F2 is enabled
 * ```
 */
export class Player extends GameObject
{
    constructor()
    {
        super('Player');

        console.log('Player: Constructing Player GameObject');

        // Set initial position and orientation (matches PlayerEntity.js)
        this.position = {x: -2.0, y: 0.0, z: 1.0};
        this.orientation = {yaw: 0.0, pitch: 0.0, roll: 0.0};

        // Component composition
        this.keyboardInput = new KeyboardInputComponent();
        this.addComponent(this.keyboardInput);

        this.flyingMovement = new FlyingMovementComponent(this.keyboardInput, 2.0);
        this.addComponent(this.flyingMovement);

        this.camera = new CameraComponent();
        this.addComponent(this.camera);

        // Debug logging timer
        this.logTimer = 0;
        this.logInterval = 1000; // Log every 1 second

        console.log('Player: Player GameObject created successfully');
        console.log('Player: Components attached:', Array.from(this.components.keys()));
    }

    /**
     * Update player and all components
     * @param {number} deltaTime - Time since last frame in milliseconds
     */
    update(deltaTime)
    {
        if (!this.active)
        {
            return;
        }

        // Update all components (calls KeyboardInputComponent.update)
        super.update(deltaTime);

        // Debug logging (periodic) - every 1 second
        this.logTimer += deltaTime;
        if (this.logTimer >= this.logInterval)
        {
            const inputState = this.keyboardInput.getInputState();
            console.log('Player GameObject: Active, input:', inputState);
            console.log('Player GameObject: Position:', this.position);
            console.log('Player GameObject: Orientation:', this.orientation);
            this.logTimer = 0;
        }
    }

    /**
     * Get player status for debugging
     */
    /**
     * Get player status for debugging
     */
    getPlayerStatus()
    {
        return {
            ...this.getStatus(),
            inputComponent: this.keyboardInput.getStatus(),
            movementComponent: this.flyingMovement.getStatus(),
            cameraComponent: this.camera.getStatus()
        };
    }

    /**
     * Get camera handle for rendering
     */
    getCamera()
    {
        return this.camera ? this.camera.getCamera() : null;
    }
}

// Export for ES6 module system
export default Player;

// Export to globalThis for hot-reload detection
globalThis.Player = Player;

console.log('Player: GameObject class loaded (Phase 1 + Phase 2 - Foundation + Movement/Camera)');
