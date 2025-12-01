//----------------------------------------------------------------------------------------------------
// AudioInterface.js
// Wrapper for C++ audio interface (globalThis.audio)
// Part of the interface wrapper layer for C++/JavaScript bridge
//----------------------------------------------------------------------------------------------------

/**
 * AudioInterface - Clean abstraction over C++ audio system
 *
 * This wrapper provides a JavaScript-friendly interface to the C++ FMOD audio system,
 * abstracting direct globalThis access and providing safe fallbacks.
 *
 * Design Principles:
 * - Single Responsibility: Only wraps C++ audio interface
 * - Safe Fallbacks: Returns sensible defaults if C++ interface unavailable
 * - Type Safety: Clear return types and parameter documentation
 * - Testability: Can be mocked for unit testing
 *
 * C++ Interface Methods (exposed via globalThis.audio):
 *
 * SYNCHRONOUS (C++ internal use only):
 * - createOrGetSound(soundPath, dimension): soundID
 * - startSound(soundID): playbackID
 * - startSoundAdvanced(soundID, isLooped, volume, balance, speed, isPaused): playbackID
 * - stopSound(playbackID): void
 * - setSoundVolume(playbackID, volume): void
 *
 * ASYNCHRONOUS (JavaScript → C++ via AudioCommandQueue):
 * - loadSoundAsync(soundPath): Promise<soundID> via callback
 * - playSoundAsync(soundID, volume, looped): Promise<playbackID> via callback
 * - stopSoundAsync(playbackID): Promise<void> via callback
 * - setVolumeAsync(playbackID, volume): Promise<void> via callback
 * - update3DPositionAsync(playbackID, x, y, z): Promise<void> via callback
 *
 * Usage Example:
 * ```javascript
 * const audioInterface = new AudioInterface();
 * const soundID = audioInterface.createOrGetSound("Data/Audio/TestSound.mp3", "Sound2D");
 * const playbackID = audioInterface.startSound(soundID);
 * audioInterface.setSoundVolume(playbackID, 0.5);
 * ```
 */
export class AudioInterface
{
    constructor()
    {
        this.cppAudio = globalThis.audio; // C++ audio interface reference

        if (!this.cppAudio)
        {
            console.log('AudioInterface: C++ audio interface (globalThis.audio) not available');
        }
        else
        {
            console.log('AudioInterface: Successfully connected to C++ audio interface');
        }
    }

    /**
     * Load or get a sound file, returns sound ID for playback
     * @param {string} soundPath - Path to the sound file (e.g., "Data/Audio/TestSound.mp3")
     * @param {string} dimension - Sound dimension: "Sound2D" or "Sound3D"
     * @returns {number|null} Sound ID for playback, or null if failed
     */
    createOrGetSound(soundPath, dimension = "Sound2D")
    {
        if (!this.cppAudio || !this.cppAudio.createOrGetSound)
        {
            console.log('AudioInterface: createOrGetSound not available');
            return null;
        }
        return this.cppAudio.createOrGetSound(soundPath, dimension);
    }

    /**
     * Start basic sound playback
     * @param {number} soundID - Sound ID from createOrGetSound
     * @returns {number|null} Playback ID for control, or null if failed
     */
    startSound(soundID)
    {
        if (!this.cppAudio || !this.cppAudio.startSound)
        {
            console.log('AudioInterface: startSound not available');
            return null;
        }
        return this.cppAudio.startSound(soundID);
    }

    /**
     * Start sound with advanced parameters
     * @param {number} soundID - Sound ID from createOrGetSound
     * @param {boolean} isLooped - Whether sound should loop
     * @param {number} volume - Volume (0.0 to 1.0)
     * @param {number} balance - Stereo balance (-1.0 to 1.0)
     * @param {number} speed - Playback speed multiplier (0.1 to 10.0)
     * @param {boolean} isPaused - Start paused
     * @returns {number|null} Playback ID for control, or null if failed
     */
    startSoundAdvanced(soundID, isLooped = false, volume = 1.0, balance = 0.0, speed = 1.0, isPaused = false)
    {
        if (!this.cppAudio || !this.cppAudio.startSoundAdvanced)
        {
            console.log('AudioInterface: startSoundAdvanced not available');
            return null;
        }
        return this.cppAudio.startSoundAdvanced(soundID, isLooped, volume, balance, speed, isPaused);
    }

    /**
     * Stop a playing sound
     * @param {number} playbackID - Playback ID from startSound
     */
    stopSound(playbackID)
    {
        if (!this.cppAudio || !this.cppAudio.stopSound)
        {
            console.log('AudioInterface: stopSound not available');
            return;
        }
        this.cppAudio.stopSound(playbackID);
    }

    /**
     * Set volume of a playing sound
     * @param {number} playbackID - Playback ID from startSound
     * @param {number} volume - Volume (0.0 to 1.0)
     */
    setSoundVolume(playbackID, volume)
    {
        if (!this.cppAudio || !this.cppAudio.setSoundVolume)
        {
            console.log('AudioInterface: setSoundVolume not available');
            return;
        }
        this.cppAudio.setSoundVolume(playbackID, volume);
    }

    /**
     * Check if C++ audio interface is available
     * @returns {boolean} True if C++ interface is connected
     */
    isAvailable()
    {
        return this.cppAudio !== undefined && this.cppAudio !== null;
    }

    /**
     * Get interface status for debugging
     */
    getStatus()
    {
        return {
            available: this.isAvailable(),
            cppInterfaceType: typeof this.cppAudio,
            hasMethods: this.cppAudio ? {
                createOrGetSound: typeof this.cppAudio.createOrGetSound === 'function',
                startSound: typeof this.cppAudio.startSound === 'function',
                startSoundAdvanced: typeof this.cppAudio.startSoundAdvanced === 'function',
                stopSound: typeof this.cppAudio.stopSound === 'function',
                setSoundVolume: typeof this.cppAudio.setSoundVolume === 'function',
                loadSoundAsync: typeof this.cppAudio.loadSoundAsync === 'function',
                playSoundAsync: typeof this.cppAudio.playSoundAsync === 'function',
                stopSoundAsync: typeof this.cppAudio.stopSoundAsync === 'function',
                setVolumeAsync: typeof this.cppAudio.setVolumeAsync === 'function',
                update3DPositionAsync: typeof this.cppAudio.update3DPositionAsync === 'function'
            } : null
        };
    }

    // ========================================
    // ASYNC METHODS (Phase 2: AudioCommandQueue)
    // ========================================
    // These methods call C++ async operations and return callbackIds
    // AudioAPI wraps these with Promise-based interface and callback registry

    /**
     * Load sound asynchronously via AudioCommandQueue
     * @param {string} soundPath - Path to the sound file
     * @returns {number} CallbackID for tracking this operation
     */
    loadSoundAsync(soundPath)
    {
        if (!this.cppAudio || !this.cppAudio.loadSoundAsync)
        {
            console.log('AudioInterface: loadSoundAsync not available');
            return 0;
        }
        return this.cppAudio.loadSoundAsync(soundPath);
    }

    /**
     * Play sound asynchronously via AudioCommandQueue
     * @param {number} soundID - Sound ID from loadSoundAsync
     * @param {number} volume - Volume (0.0 to 1.0)
     * @param {boolean} looped - Whether sound should loop
     * @returns {number} CallbackID for tracking this operation
     */
    playSoundAsync(soundID, volume = 1.0, looped = false)
    {
        if (!this.cppAudio || !this.cppAudio.playSoundAsync)
        {
            console.log('AudioInterface: playSoundAsync not available');
            return 0;
        }
        return this.cppAudio.playSoundAsync(soundID, volume, looped);
    }

    /**
     * Stop sound asynchronously via AudioCommandQueue
     * @param {number} playbackID - Playback ID from playSoundAsync
     * @returns {number} CallbackID for tracking this operation
     */
    stopSoundAsync(playbackID)
    {
        if (!this.cppAudio || !this.cppAudio.stopSoundAsync)
        {
            console.log('AudioInterface: stopSoundAsync not available');
            return 0;
        }
        return this.cppAudio.stopSoundAsync(playbackID);
    }

    /**
     * Set volume asynchronously via AudioCommandQueue
     * @param {number} playbackID - Playback ID
     * @param {number} volume - Volume (0.0 to 1.0)
     * @returns {number} CallbackID for tracking this operation
     */
    setVolumeAsync(playbackID, volume)
    {
        if (!this.cppAudio || !this.cppAudio.setVolumeAsync)
        {
            console.log('AudioInterface: setVolumeAsync not available');
            return 0;
        }
        return this.cppAudio.setVolumeAsync(playbackID, volume);
    }

    /**
     * Update 3D position asynchronously via AudioCommandQueue
     * @param {number} playbackID - Playback ID
     * @param {number} x - X coordinate
     * @param {number} y - Y coordinate
     * @param {number} z - Z coordinate
     * @returns {number} CallbackID for tracking this operation
     */
    update3DPositionAsync(playbackID, x, y, z)
    {
        if (!this.cppAudio || !this.cppAudio.update3DPositionAsync)
        {
            console.log('AudioInterface: update3DPositionAsync not available');
            return 0;
        }
        return this.cppAudio.update3DPositionAsync(playbackID, x, y, z);
    }
}

// Export for ES6 module system
export default AudioInterface;

// Export to globalThis for hot-reload detection
globalThis.AudioInterface = AudioInterface;

console.log('AudioInterface: Wrapper loaded (Interface Layer)');
