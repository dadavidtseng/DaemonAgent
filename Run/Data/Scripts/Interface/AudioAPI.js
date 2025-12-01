//----------------------------------------------------------------------------------------------------
// AudioAPI.js
// High-level async audio API following EntityAPI/CameraAPI pattern
//----------------------------------------------------------------------------------------------------

import { AudioInterface } from './AudioInterface.js';

/**
 * AudioAPI - High-level async audio operations with callback handling
 *
 * Follows the same pattern as EntityAPI and CameraAPI:
 * - Stores callbacks in callbackRegistry
 * - C++ enqueues results to CallbackQueue
 * - JSEngine routes callbacks via handleCallback()
 *
 * Usage:
 * ```javascript
 * const soundId = await AudioAPI.loadSoundAsync("Data/Audio/TestSound.mp3");
 * const playbackId = await AudioAPI.playSoundAsync(soundId);
 * ```
 */
export class AudioAPI
{
    constructor()
    {
        this.audioInterface = new AudioInterface();
        this.callbackRegistry = new Map();  // callbackId -> callback function
        this.nextLocalCallbackId = 1;  // For generating unique callback IDs

        console.log('AudioAPI: Initialized with callback registry');
    }

    /**
     * Load sound asynchronously via AudioCommandQueue
     * @param {string} soundPath - Path to the sound file
     * @returns {Promise<number>} Promise resolving to soundID
     */
    async loadSoundAsync(soundPath)
    {
        return new Promise((resolve, reject) =>
        {
            try
            {
                // Call C++ async method (returns callbackId)
                const callbackId = this.audioInterface.loadSoundAsync(soundPath);

                if (!callbackId || callbackId === 0)
                {
                    reject(new Error('Failed to submit loadSoundAsync command'));
                    return;
                }

                // Register callback for when C++ completes the operation
                this.callbackRegistry.set(callbackId, (soundId, errorMessage) =>
                {
                    if (errorMessage && errorMessage.length > 0)
                    {
                        reject(new Error(errorMessage));
                    }
                    else
                    {
                        resolve(soundId);
                    }
                });

                console.log(`AudioAPI: Registered callback ${callbackId} for loadSoundAsync('${soundPath}')`);
            }
            catch (error)
            {
                reject(error);
            }
        });
    }

    /**
     * Play sound asynchronously via AudioCommandQueue
     * @param {number} soundID - Sound ID from loadSoundAsync
     * @param {number} volume - Volume (0.0 to 1.0)
     * @param {boolean} looped - Whether sound should loop
     * @returns {Promise<number>} Promise resolving to playbackID
     */
    async playSoundAsync(soundID, volume = 1.0, looped = false)
    {
        return new Promise((resolve, reject) =>
        {
            try
            {
                // Call C++ async method (returns callbackId)
                const callbackId = this.audioInterface.playSoundAsync(soundID, volume, looped);

                if (!callbackId || callbackId === 0)
                {
                    reject(new Error('Failed to submit playSoundAsync command'));
                    return;
                }

                // Register callback for when C++ completes the operation
                this.callbackRegistry.set(callbackId, (playbackId, errorMessage) =>
                {
                    if (errorMessage && errorMessage.length > 0)
                    {
                        reject(new Error(errorMessage));
                    }
                    else
                    {
                        resolve(playbackId);
                    }
                });

                console.log(`AudioAPI: Registered callback ${callbackId} for playSoundAsync(soundId=${soundID})`);
            }
            catch (error)
            {
                reject(error);
            }
        });
    }

    /**
     * Stop sound asynchronously via AudioCommandQueue
     * @param {number} playbackID - Playback ID from playSoundAsync
     * @returns {Promise<void>}
     */
    async stopSoundAsync(playbackID)
    {
        return new Promise((resolve, reject) =>
        {
            try
            {
                // Call C++ async method (returns callbackId)
                const callbackId = this.audioInterface.stopSoundAsync(playbackID);

                if (!callbackId || callbackId === 0)
                {
                    reject(new Error('Failed to submit stopSoundAsync command'));
                    return;
                }

                // Register callback for when C++ completes the operation
                this.callbackRegistry.set(callbackId, (resultId, errorMessage) =>
                {
                    if (errorMessage && errorMessage.length > 0)
                    {
                        reject(new Error(errorMessage));
                    }
                    else
                    {
                        resolve();
                    }
                });

                console.log(`AudioAPI: Registered callback ${callbackId} for stopSoundAsync(playbackId=${playbackID})`);
            }
            catch (error)
            {
                reject(error);
            }
        });
    }

    /**
     * Set volume asynchronously via AudioCommandQueue
     * @param {number} playbackID - Playback ID
     * @param {number} volume - Volume (0.0 to 1.0)
     * @returns {Promise<void>}
     */
    async setVolumeAsync(playbackID, volume)
    {
        return new Promise((resolve, reject) =>
        {
            try
            {
                // Call C++ async method (returns callbackId)
                const callbackId = this.audioInterface.setVolumeAsync(playbackID, volume);

                if (!callbackId || callbackId === 0)
                {
                    reject(new Error('Failed to submit setVolumeAsync command'));
                    return;
                }

                // Register callback for when C++ completes the operation
                this.callbackRegistry.set(callbackId, (resultId, errorMessage) =>
                {
                    if (errorMessage && errorMessage.length > 0)
                    {
                        reject(new Error(errorMessage));
                    }
                    else
                    {
                        resolve();
                    }
                });

                console.log(`AudioAPI: Registered callback ${callbackId} for setVolumeAsync(playbackId=${playbackID}, volume=${volume})`);
            }
            catch (error)
            {
                reject(error);
            }
        });
    }

    /**
     * Handle callback from C++ (Phase 2: AudioCommandQueue)
     * Called by JSEngine.executeCallback() when callback dequeued from CallbackQueue
     *
     * @param {number} callbackId - Callback ID from C++
     * @param {number} resultId - Result ID (soundID or playbackID, or 0 if failed)
     * @param {string} errorMessage - Error message (empty if success)
     */
    handleCallback(callbackId, resultId, errorMessage)
    {
        // Look up callback function in registry
        const callback = this.callbackRegistry.get(callbackId);

        if (!callback)
        {
            // Callback already executed or not registered (hot-reload or duplicate enqueue)
            console.log(`AudioAPI: No callback registered for callbackId ${callbackId} (may be hot-reload)`);
            return;
        }

        // Remove callback from registry (one-time use)
        this.callbackRegistry.delete(callbackId);

        // Invoke JavaScript callback
        try
        {
            callback(resultId, errorMessage);
        }
        catch (error)
        {
            console.log(`AudioAPI: Error executing callback ${callbackId}:`, error);
        }
    }

    /**
     * Check if C++ audio interface is available
     * @returns {boolean} True if C++ interface is connected
     */
    isAvailable()
    {
        return this.audioInterface.isAvailable();
    }
}

// Export singleton instance
export const audioAPI = new AudioAPI();

// Export to globalThis for JSEngine callback routing
globalThis.audioAPI = audioAPI;

console.log('AudioAPI: Module loaded (Phase 2: AudioCommandQueue)');
