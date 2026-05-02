//----------------------------------------------------------------------------------------------------
// BuildTools.js
// KADI tool schema definitions for build and versioning tools
//----------------------------------------------------------------------------------------------------

export const BuildTools = [
    {
        name: "get_version",
        description: "Get the current DaemonAgent version from the VERSION file (semver format).",
        inputSchema: {
            type: "object",
            properties: {},
            required: []
        }
    }
];

// Export for hot-reload
globalThis.BuildTools = BuildTools;

console.log(`BuildTools: Module loaded (${BuildTools.length} tools defined)`);
