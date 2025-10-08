//----------------------------------------------------------------------------------------------------
// PlayerEntity.js - JavaScript Player Entity with Camera Control
//----------------------------------------------------------------------------------------------------
import { EntityBase } from './EntityBase.js';

/**
 * PlayerEntity - Full camera controller migrated from C++ Player.cpp
 *
 * Extends EntityBase (C++ Entity class)
 * Manages:
 * - World camera creation and configuration
 * - WASD + Z/C for camera movement
 * - Mouse + Right stick for camera rotation
 * - Q/E + Triggers for camera roll
 * - Shift + A button for speed boost
 */
export class PlayerEntity extends EntityBase {
    constructor(game = null) {
        super(game); // Call EntityBase constructor

        // Create and configure world camera
        this.worldCamera = null;
        this.initializeCamera();

        // Override base EntityBase properties with Player-specific initial values
        this.m_position = { x: -2.0, y: 0.0, z: 1.0 };
        this.m_orientation = { yaw: 0.0, pitch: 0.0, roll: 0.0 };
        this.m_velocity = { x: 0.0, y: 0.0, z: 0.0 };
        this.m_angularVelocity = { yaw: 0.0, pitch: 0.0, roll: 0.0 };

        // Movement constants
        this.moveSpeed = 2.0;
        this.mouseSensitivity = 0.125;
        this.rollSpeed = 90.0;

        console.log('PlayerEntity: Initialized with camera');
    }

    /**
     * Initialize world camera (matches C++ Player constructor)
     */
    initializeCamera() {
        // Debug: Check if cameraInterface is available
        console.log('PlayerEntity: initializeCamera() called');
        console.log('PlayerEntity: typeof cameraInterface =', typeof cameraInterface);
        console.log('PlayerEntity: cameraInterface =', cameraInterface);
        console.log('PlayerEntity: typeof cameraInterface.createCamera =', typeof cameraInterface.createCamera);

        // Create camera through CameraScriptInterface
        console.log('PlayerEntity: About to call cameraInterface.createCamera()...');
        try {
            this.worldCamera = cameraInterface.createCamera();
            console.log('PlayerEntity: createCamera() returned:', this.worldCamera);
            console.log('PlayerEntity: typeof worldCamera =', typeof this.worldCamera);
        } catch (error) {
            console.error('PlayerEntity: ERROR in createCamera():', error);
            throw error;
        }

        // SetPerspectiveGraphicView(2.f, 60.f, 0.1f, 100.f)
        console.log('PlayerEntity: Setting perspective view (aspect=2.0, fov=60.0, near=0.1, far=100.0)');
        cameraInterface.setPerspectiveView(this.worldCamera, 2.0, 60.0, 0.1, 100.0);

        // SetNormalizedViewport(AABB2::ZERO_TO_ONE)
        console.log('PlayerEntity: Setting normalized viewport (0, 0, 1, 1)');
        cameraInterface.setNormalizedViewport(this.worldCamera, 0.0, 0.0, 1.0, 1.0);

        // SetCameraToRenderTransform (custom matrix)
        // Mat44 c2r matching C++ Player.cpp lines 26-35
        const c2r = [
            0.0, 0.0, 1.0, 0.0,  // Ix=0, Iy=0, Iz=1
            -1.0, 0.0, 0.0, 0.0, // Jx=-1, Jy=0, Jz=0
            0.0, 1.0, 0.0, 0.0,  // Kx=0, Ky=1, Kz=0
            0.0, 0.0, 0.0, 1.0   // Tx=0, Ty=0, Tz=0, Tw=1
        ];
        console.log('PlayerEntity: Setting camera-to-render transform matrix');
        cameraInterface.setCameraToRenderTransform(this.worldCamera, ...c2r);

        // Set initial position and orientation
        console.log('PlayerEntity: Setting initial camera position=(-2,0,1), orientation=(0,0,0)');
        console.log('PlayerEntity: m_position =', JSON.stringify(this.m_position));
        console.log('PlayerEntity: m_orientation =', JSON.stringify(this.m_orientation));
        this.updateCameraTransform();

        console.log('PlayerEntity: World camera created and configured');
    }

    /**
     * Update - Override EntityBase::update()
     * Matches C++ Player::Update(float deltaSeconds)
     */
    update(deltaSeconds) {
        // Handle input (matches C++ Player::Update lines 47-106)
        this.handleInput(deltaSeconds);

        // Update position
        this.m_position.x += this.m_velocity.x * deltaSeconds;
        this.m_position.y += this.m_velocity.y * deltaSeconds;
        this.m_position.z += this.m_velocity.z * deltaSeconds;

        // Update orientation
        this.m_orientation.roll += this.m_angularVelocity.roll * deltaSeconds;
        this.m_orientation.roll = this.clamp(this.m_orientation.roll, -45.0, 45.0);
        this.m_orientation.pitch = this.clamp(this.m_orientation.pitch, -85.0, 85.0);

        // Sync to C++ camera
        this.updateCameraTransform();
    }

    /**
     * Render - Override EntityBase::render()
     * Matches C++ Player::Render() const (empty in C++)
     */
    render() {
        // Player doesn't render anything (it's just a camera controller)
        // C++ Player::Render() is empty
    }

    /**
     * Handle input (matches C++ Player::Update input handling)
     */
    handleInput(deltaSeconds) {
        const controller = input.getController(0);

        // Reset position on H key or START button (lines 49-56)
        if (input.wasKeyJustPressed('H') || controller.wasButtonJustPressed('START')) {
            if (this.m_game && !this.m_game.isAttractMode()) {
                this.m_position = { x: 0.0, y: 0.0, z: 0.0 };
                this.m_orientation = { yaw: 0.0, pitch: 0.0, roll: 0.0 };
            }
        }

        // Calculate forward, left, up vectors from orientation (use EntityBase methods)
        const forward = this.getForwardVector();
        const left = this.getLeftVector();

        // Reset velocity
        this.m_velocity = { x: 0.0, y: 0.0, z: 0.0 };

        // Left stick movement (lines 66-67)
        const leftStick = controller.getLeftStick();

        // Defensive: Ensure leftStick.x and leftStick.y are valid numbers
        const leftStickX = (typeof leftStick.x === 'number' && !isNaN(leftStick.x)) ? leftStick.x : 0;
        const leftStickY = (typeof leftStick.y === 'number' && !isNaN(leftStick.y)) ? leftStick.y : 0;

        this.m_velocity.x += (leftStickY * forward.x - leftStickX * left.x) * this.moveSpeed;
        this.m_velocity.y += (leftStickY * forward.y - leftStickX * left.y) * this.moveSpeed;
        this.m_velocity.z += (leftStickY * forward.z - leftStickX * left.z) * this.moveSpeed;

        // Keyboard movement (lines 69-74)
        if (input.isKeyDown('W')) {
            this.m_velocity.x += forward.x * this.moveSpeed;
            this.m_velocity.y += forward.y * this.moveSpeed;
            this.m_velocity.z += forward.z * this.moveSpeed;
        }
        if (input.isKeyDown('S')) {
            this.m_velocity.x -= forward.x * this.moveSpeed;
            this.m_velocity.y -= forward.y * this.moveSpeed;
            this.m_velocity.z -= forward.z * this.moveSpeed;
        }
        if (input.isKeyDown('A')) {
            this.m_velocity.x += left.x * this.moveSpeed;
            this.m_velocity.y += left.y * this.moveSpeed;
            this.m_velocity.z += left.z * this.moveSpeed;
        }
        if (input.isKeyDown('D')) {
            this.m_velocity.x -= left.x * this.moveSpeed;
            this.m_velocity.y -= left.y * this.moveSpeed;
            this.m_velocity.z -= left.z * this.moveSpeed;
        }
        if (input.isKeyDown('Z') || controller.isButtonDown('LSHOULDER')) {
            this.m_velocity.z -= this.moveSpeed;
        }
        if (input.isKeyDown('C') || controller.isButtonDown('RSHOULDER')) {
            this.m_velocity.z += this.moveSpeed;
        }

        // Speed boost (line 76)
        let speedMultiplier = 1.0;
        if (input.isKeyDown('SHIFT') || controller.isButtonDown('A')) {
            speedMultiplier = 10.0;
        }

        this.m_velocity.x *= speedMultiplier;
        this.m_velocity.y *= speedMultiplier;
        this.m_velocity.z *= speedMultiplier;

        // Right stick rotation (lines 80-82)
        const rightStick = controller.getRightStick();

        // Defensive: Ensure rightStick.x and rightStick.y are valid numbers
        const rightStickX = (typeof rightStick.x === 'number' && !isNaN(rightStick.x)) ? rightStick.x : 0;
        const rightStickY = (typeof rightStick.y === 'number' && !isNaN(rightStick.y)) ? rightStick.y : 0;

        this.m_orientation.yaw -= rightStickX * this.mouseSensitivity;
        this.m_orientation.pitch -= rightStickY * this.mouseSensitivity;

        // Mouse rotation (lines 84-86)
        const mouseDelta = input.getCursorClientDelta();

        // Defensive: Ensure mouseDelta.x and mouseDelta.y are valid numbers
        const deltaX = (typeof mouseDelta.x === 'number' && !isNaN(mouseDelta.x)) ? mouseDelta.x : 0;
        const deltaY = (typeof mouseDelta.y === 'number' && !isNaN(mouseDelta.y)) ? mouseDelta.y : 0;

        this.m_orientation.yaw -= deltaX * this.mouseSensitivity;
        this.m_orientation.pitch += deltaY * this.mouseSensitivity;

        // Defensive: Ensure orientation never becomes NaN or null
        if (typeof this.m_orientation.yaw !== 'number' || isNaN(this.m_orientation.yaw)) {
            console.error('PlayerEntity: yaw became NaN! Resetting to 0');
            this.m_orientation.yaw = 0.0;
        }
        if (typeof this.m_orientation.pitch !== 'number' || isNaN(this.m_orientation.pitch)) {
            console.error('PlayerEntity: pitch became NaN! Resetting to 0');
            this.m_orientation.pitch = 0.0;
        }

        // Roll controls (lines 88-106)
        this.m_angularVelocity.roll = 0.0;

        const leftTrigger = controller.getLeftTrigger();
        const rightTrigger = controller.getRightTrigger();

        // Defensive: Ensure trigger values are valid numbers
        const leftTriggerValue = (typeof leftTrigger === 'number' && !isNaN(leftTrigger)) ? leftTrigger : 0;
        const rightTriggerValue = (typeof rightTrigger === 'number' && !isNaN(rightTrigger)) ? rightTrigger : 0;

        if (leftTriggerValue > 0.0) {
            this.m_angularVelocity.roll -= this.rollSpeed;
        }
        if (rightTriggerValue > 0.0) {
            this.m_angularVelocity.roll += this.rollSpeed;
        }
        if (input.isKeyDown('Q')) {
            this.m_angularVelocity.roll = this.rollSpeed;
        }
        if (input.isKeyDown('E')) {
            this.m_angularVelocity.roll = -this.rollSpeed;
        }
    }

    /**
     * Update C++ camera transform (matches C++ Player::Update line 109)
     */
    updateCameraTransform() {
        // Log camera updates occasionally (every 60 frames) to avoid spam
        if (!this.cameraUpdateCount) this.cameraUpdateCount = 0;
        this.cameraUpdateCount++;

        if (this.cameraUpdateCount % 60 === 0) {
            console.log(`PlayerEntity: Update Camera Transform (frame ${this.cameraUpdateCount})`);
            console.log('  position:', JSON.stringify(this.m_position));
            console.log('  orientation:', JSON.stringify(this.m_orientation));
        }

        cameraInterface.setCameraPositionAndOrientation(
            this.worldCamera,
            this.m_position.x, this.m_position.y, this.m_position.z,
            this.m_orientation.yaw, this.m_orientation.pitch, this.m_orientation.roll
        );
    }

    /**
     * Get camera handle for rendering
     */
    getCamera() {
        return this.worldCamera;
    }

    /**
     * Clamp value between min and max
     */
    clamp(value, min, max) {
        return Math.max(min, Math.min(max, value));
    }

    /**
     * Cleanup - Destroy camera when entity is destroyed
     * Matches C++ Player::~Player()
     */
    destroy() {
        if (this.worldCamera) {
            cameraInterface.destroyCamera(this.worldCamera);
            this.worldCamera = null;
            console.log('PlayerEntity: World camera destroyed');
        }
    }
}

console.log('PlayerEntity: Module loaded (extends EntityBase, full C++ Player.cpp migration)');
