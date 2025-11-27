//----------------------------------------------------------------------------------------------------
// ClockInterface.js
// Wrapper for C++ clock interface (globalThis.clock)
// Part of the interface wrapper layer for C++/JavaScript bridge
//----------------------------------------------------------------------------------------------------

/**
 * ClockInterface - Clean abstraction over C++ clock system
 *
 * This wrapper provides a JavaScript-friendly interface to the C++ clock system,
 * abstracting direct globalThis access and providing safe fallbacks.
 *
 * Design Principles:
 * - Single Responsibility: Only wraps C++ clock interface
 * - Safe Fallbacks: Returns sensible defaults if C++ interface unavailable
 * - Type Safety: Clear return types and parameter documentation
 * - Testability: Can be mocked for unit testing
 *
 * C++ Interface Methods (exposed via globalThis.clock):
 * - createClock(): clockHandle
 * - destroyClock(clockHandle): void
 * - pause(clockHandle): void
 * - unpause(clockHandle): void
 * - togglePause(clockHandle): void
 * - isPaused(clockHandle): boolean
 * - stepSingleFrame(clockHandle): void
 * - setTimeScale(clockHandle, scale): void
 * - reset(clockHandle): void
 * - getTimeScale(clockHandle): number
 * - getDeltaSeconds(clockHandle): number
 * - getTotalSeconds(clockHandle): number
 * - getFrameCount(clockHandle): number
 *
 * Usage Example:
 * ```javascript
 * const clockInterface = new ClockInterface();
 * const clock = clockInterface.createClock();
 * clockInterface.pause(clock);
 * clockInterface.setTimeScale(clock, 0.5);  // Slow motion
 * const delta = clockInterface.getDeltaSeconds(clock);
 * ```
 */
export class ClockInterface
{
    constructor()
    {
        this.cppClock = globalThis.clock; // C++ clock interface reference

        if (!this.cppClock)
        {
            console.log('ClockInterface: C++ clock interface (globalThis.clock) not available');
        }
        else
        {
            console.log('ClockInterface: Successfully connected to C++ clock interface');
        }
    }

    /**
     * Create a new clock
     * @returns {number} Clock handle or null if creation failed
     */
    createClock()
    {
        if (!this.cppClock || !this.cppClock.createClock)
        {
            console.log('ClockInterface: createClock not available');
            return null;
        }
        return this.cppClock.createClock();
    }

    /**
     * Destroy a clock
     * @param {number} clockHandle - Clock handle to destroy
     */
    destroyClock(clockHandle)
    {
        if (!this.cppClock || !this.cppClock.destroyClock)
        {
            console.log('ClockInterface: destroyClock not available');
            return;
        }
        this.cppClock.destroyClock(clockHandle);
    }

    /**
     * Pause the clock, stopping time progression
     * @param {number} clockHandle - Clock handle
     */
    pause(clockHandle)
    {
        if (!this.cppClock || !this.cppClock.pause)
        {
            console.log('ClockInterface: pause not available');
            return;
        }
        return this.cppClock.pause(clockHandle);
    }

    /**
     * Unpause the clock, resuming time progression
     * @param {number} clockHandle - Clock handle
     */
    unpause(clockHandle)
    {
        if (!this.cppClock || !this.cppClock.unpause)
        {
            console.log('ClockInterface: unpause not available');
            return;
        }
        return this.cppClock.unpause(clockHandle);
    }

    /**
     * Toggle pause state of the clock
     * @param {number} clockHandle - Clock handle
     */
    togglePause(clockHandle)
    {
        if (!this.cppClock || !this.cppClock.togglePause)
        {
            console.log('ClockInterface: togglePause not available');
            return;
        }
        return this.cppClock.togglePause(clockHandle);
    }

    /**
     * Check if the clock is currently paused
     * @param {number} clockHandle - Clock handle
     * @returns {boolean} True if paused, false otherwise
     */
    isPaused(clockHandle)
    {
        if (!this.cppClock || !this.cppClock.isPaused)
        {
            console.log('ClockInterface: isPaused not available');
            return false;
        }
        return this.cppClock.isPaused(clockHandle);
    }

    /**
     * Advance the clock by a single frame while paused
     * @param {number} clockHandle - Clock handle
     */
    stepSingleFrame(clockHandle)
    {
        if (!this.cppClock || !this.cppClock.stepSingleFrame)
        {
            console.log('ClockInterface: stepSingleFrame not available');
            return;
        }
        return this.cppClock.stepSingleFrame(clockHandle);
    }

    /**
     * Set time scale multiplier
     * @param {number} clockHandle - Clock handle
     * @param {number} scale - Time scale (1.0 = normal, 0.5 = slow motion, 2.0 = fast forward)
     */
    setTimeScale(clockHandle, scale)
    {
        if (!this.cppClock || !this.cppClock.setTimeScale)
        {
            console.log('ClockInterface: setTimeScale not available');
            return;
        }
        return this.cppClock.setTimeScale(clockHandle, scale);
    }

    /**
     * Reset the clock to initial state
     * @param {number} clockHandle - Clock handle
     */
    reset(clockHandle)
    {
        if (!this.cppClock || !this.cppClock.reset)
        {
            console.log('ClockInterface: reset not available');
            return;
        }
        return this.cppClock.reset(clockHandle);
    }

    /**
     * Get current time scale multiplier
     * @param {number} clockHandle - Clock handle
     * @returns {number} Current time scale
     */
    getTimeScale(clockHandle)
    {
        if (!this.cppClock || !this.cppClock.getTimeScale)
        {
            console.log('ClockInterface: getTimeScale not available');
            return 1.0;
        }
        return this.cppClock.getTimeScale(clockHandle);
    }

    /**
     * Get time elapsed since last frame in seconds
     * @param {number} clockHandle - Clock handle
     * @returns {number} Delta time in seconds
     */
    getDeltaSeconds(clockHandle)
    {
        if (!this.cppClock || !this.cppClock.getDeltaSeconds)
        {
            console.log('ClockInterface: getDeltaSeconds not available');
            return 0.0;
        }
        return this.cppClock.getDeltaSeconds(clockHandle);
    }

    /**
     * Get total accumulated time in seconds
     * @param {number} clockHandle - Clock handle
     * @returns {number} Total time in seconds
     */
    getTotalSeconds(clockHandle)
    {
        if (!this.cppClock || !this.cppClock.getTotalSeconds)
        {
            console.log('ClockInterface: getTotalSeconds not available');
            return 0.0;
        }
        return this.cppClock.getTotalSeconds(clockHandle);
    }

    /**
     * Get total number of frames processed by this clock
     * @param {number} clockHandle - Clock handle
     * @returns {number} Frame count
     */
    getFrameCount(clockHandle)
    {
        if (!this.cppClock || !this.cppClock.getFrameCount)
        {
            console.log('ClockInterface: getFrameCount not available');
            return 0;
        }
        return this.cppClock.getFrameCount(clockHandle);
    }

    /**
     * Check if C++ clock interface is available
     * @returns {boolean} True if C++ interface is connected
     */
    isAvailable()
    {
        return this.cppClock !== undefined && this.cppClock !== null;
    }

    /**
     * Get interface status for debugging
     */
    getStatus()
    {
        return {
            available: this.isAvailable(),
            cppInterfaceType: typeof this.cppClock,
            hasMethods: this.cppClock ? {
                createClock: typeof this.cppClock.createClock === 'function',
                destroyClock: typeof this.cppClock.destroyClock === 'function',
                pause: typeof this.cppClock.pause === 'function',
                unpause: typeof this.cppClock.unpause === 'function',
                togglePause: typeof this.cppClock.togglePause === 'function',
                isPaused: typeof this.cppClock.isPaused === 'function',
                stepSingleFrame: typeof this.cppClock.stepSingleFrame === 'function',
                setTimeScale: typeof this.cppClock.setTimeScale === 'function',
                reset: typeof this.cppClock.reset === 'function',
                getTimeScale: typeof this.cppClock.getTimeScale === 'function',
                getDeltaSeconds: typeof this.cppClock.getDeltaSeconds === 'function',
                getTotalSeconds: typeof this.cppClock.getTotalSeconds === 'function',
                getFrameCount: typeof this.cppClock.getFrameCount === 'function'
            } : null
        };
    }
}

// Export for ES6 module system
export default ClockInterface;

// Export to globalThis for hot-reload detection
globalThis.ClockInterface = ClockInterface;

console.log('ClockInterface: Wrapper loaded (Interface Layer)');
