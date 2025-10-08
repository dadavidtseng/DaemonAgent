//----------------------------------------------------------------------------------------------------
// PlayerEntity.js - JavaScript Player Entity (Matches C++ Player structure)
//----------------------------------------------------------------------------------------------------
import { EntityBase } from './EntityBase.js';
import { Subsystem } from '../core/Subsystem.js';

/**
 * PlayerEntity - Camera controller matching C++ Player.cpp
 *
 * C++ Player is a camera controller, NOT a visible entity:
 * - WASD + Z/C for camera movement (commented out in C++)
 * - Mouse for camera rotation
 * - Q/E for camera roll
 * - No rendering (empty Render() method)
 *
 * This JavaScript version creates a simple camera controller system.
 */
export class PlayerEntity extends Subsystem {
    constructor(engine) {
        super('playerEntity', 11, {enabled: false}); // Disabled by default
        this.engine = engine;

        this.data = {
            player: null,
            initialized: false
        };
    }

    /**
     * Update - Create player on first frame
     */
    update(gameDelta, systemDelta) {
        if (!this.data.initialized) {
            this.createPlayer();
            this.data.initialized = true;
        }

        if (this.data.player) {
            this.data.player.update(systemDelta);
            this.updateCameraFromPlayer();
        }
    }

    /**
     * Create player entity
     */
    createPlayer() {
        console.log('PlayerEntity: Creating JavaScript player (camera controller)...');

        this.data.player = new EntityBase();
        this.data.player.m_position = { x: 0, y: 0, z: 5 };
        this.data.player.m_orientation = { yaw: 0, pitch: 0, roll: 0 };

        console.log('PlayerEntity: Player camera controller created');
    }

    /**
     * Update C++ camera from player position
     */
    updateCameraFromPlayer() {
        if (this.engine && this.engine.moveCamera) {
            const pos = this.data.player.m_position;
            this.engine.moveCamera(pos.x, pos.y, pos.z);
        }
    }
}

console.log('PlayerEntity: Module loaded');
