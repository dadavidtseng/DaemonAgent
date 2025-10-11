// Clock.js
// JavaScript wrapper for ClockScriptInterface
// Provides clean JavaScript API for C++ Clock time management functionality

import {ClockInterface} from '../interfaces/ClockInterface.js';

/**
 * Clock - JavaScript wrapper for game clock control
 *
 * Provides time management functionality including:
 * - Pause/unpause control
 * - Time scaling (slow motion, fast forward)
 * - Frame stepping for debugging
 * - Time queries (delta, total, frame count)
 *
 * Usage:
 *   const gameClock = new Clock();
 *   gameClock.pause();
 *   gameClock.setTimeScale(0.5);  // Slow motion
 *   const delta = gameClock.getDeltaSeconds();
 */
export class Clock
{
    constructor()
    {
        this.clockInterface = new ClockInterface();

        if (!this.clockInterface.isAvailable())
        {
            throw new Error('Clock: ClockInterface not available');
        }

        // Create C++ clock instance (as child of system clock)
        this.handle = this.clockInterface.createClock();

        console.log(`Clock: Created C++ clock instance, handle = ${this.handle}`);
    }

    /**
     * Destroy the C++ clock instance
     * Should be called when the Clock is no longer needed
     */
    destroy()
    {
        if (this.handle)
        {
            this.clockInterface.destroyClock(this.handle);
            this.handle = null;
            console.log('Clock: Destroyed C++ clock instance');
        }
    }

    // === PAUSE CONTROL ===

    /**
     * Pause the clock, stopping time progression
     */
    pause()
    {
        return this.clockInterface.pause(this.handle);
    }

    /**
     * Unpause the clock, resuming time progression
     */
    unpause()
    {
        return this.clockInterface.unpause(this.handle);
    }

    /**
     * Toggle pause state of the clock
     */
    togglePause()
    {
        return this.clockInterface.togglePause(this.handle);
    }

    /**
     * Check if the clock is currently paused
     * @returns {boolean} True if paused, false otherwise
     */
    isPaused()
    {
        return this.clockInterface.isPaused(this.handle);
    }

    // === TIME CONTROL ===

    /**
     * Advance the clock by a single frame while paused
     * Useful for frame-by-frame debugging
     */
    stepSingleFrame()
    {
        return this.clockInterface.stepSingleFrame(this.handle);
    }

    /**
     * Set time scale multiplier
     * @param {number} scale - Time scale (1.0 = normal, 0.5 = slow motion, 2.0 = fast forward)
     */
    setTimeScale(scale)
    {
        return this.clockInterface.setTimeScale(this.handle, scale);
    }

    /**
     * Reset the clock to initial state
     */
    reset()
    {
        return this.clockInterface.reset(this.handle);
    }

    // === TIME QUERIES ===

    /**
     * Get current time scale multiplier
     * @returns {number} Current time scale
     */
    getTimeScale()
    {
        return this.clockInterface.getTimeScale(this.handle);
    }

    /**
     * Get time elapsed since last frame in seconds
     * @returns {number} Delta time in seconds
     */
    getDeltaSeconds()
    {
        return this.clockInterface.getDeltaSeconds(this.handle);
    }

    /**
     * Get total accumulated time in seconds
     * @returns {number} Total time in seconds
     */
    getTotalSeconds()
    {
        return this.clockInterface.getTotalSeconds(this.handle);
    }

    /**
     * Get total number of frames processed by this clock
     * @returns {number} Frame count
     */
    getFrameCount()
    {
        return this.clockInterface.getFrameCount(this.handle);
    }

    // === CONVENIENCE METHODS ===

    /**
     * Get comprehensive time information in a single call
     * @returns {object} Time info containing delta, total, frameCount, timeScale, isPaused
     */
    getTimeInfo()
    {
        return {
            deltaSeconds: this.getDeltaSeconds(),
            totalSeconds: this.getTotalSeconds(),
            frameCount: this.getFrameCount(),
            timeScale: this.getTimeScale(),
            isPaused: this.isPaused()
        };
    }
}

// Export to globalThis for hot-reload detection
globalThis.Clock = Clock;

console.log('Clock: Component loaded (JavaScript wrapper for ClockScriptInterface)');
