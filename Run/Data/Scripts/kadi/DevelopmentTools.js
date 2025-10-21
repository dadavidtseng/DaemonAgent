//----------------------------------------------------------------------------------------------------
// DevelopmentTools.js
// KADI tool schema definitions for development tools (Phase 6a)
//----------------------------------------------------------------------------------------------------

/**
 * Tool schema array for KADI registration
 * Phase 6a: File I/O and Input Injection tools
 * Follows JSON-RPC tool specification format
 */
export const DevelopmentTools = [
    {
        name: "create_script",
        description: "Create a new JavaScript file in Scripts directory with specified content",
        inputSchema: {
            type: "object",
            properties: {
                filePath: {
                    type: "string",
                    pattern: "^[^.][^/\\\\]*\\.js$",
                    description: "Relative file path within Scripts directory (must end with .js, no directory traversal)"
                },
                content: {
                    type: "string",
                    description: "JavaScript code content to write to the file"
                },
                overwrite: {
                    type: "boolean",
                    default: false,
                    description: "Whether to overwrite existing file (default: false)"
                }
            },
            required: ["filePath", "content"]
        }
    },
    {
        name: "read_script",
        description: "Read an existing JavaScript file from Scripts directory",
        inputSchema: {
            type: "object",
            properties: {
                filePath: {
                    type: "string",
                    pattern: "^[^.][^/\\\\]*\\.js$",
                    description: "Relative file path within Scripts directory (must end with .js)"
                }
            },
            required: ["filePath"]
        }
    },
    {
        name: "delete_script",
        description: "Delete a JavaScript file from Scripts directory (protected files cannot be deleted)",
        inputSchema: {
            type: "object",
            properties: {
                filePath: {
                    type: "string",
                    pattern: "^[^.][^/\\\\]*\\.js$",
                    description: "Relative file path within Scripts directory (must end with .js)"
                }
            },
            required: ["filePath"]
        }
    },
    {
        name: "press_keycode",
        description: "Inject a key press event with specified duration",
        inputSchema: {
            type: "object",
            properties: {
                keyCode: {
                    type: "integer",
                    minimum: 0,
                    maximum: 255,
                    description: "Windows virtual key code (0-255, e.g., 0x45 for 'E' key)"
                },
                durationMs: {
                    type: "integer",
                    minimum: 0,
                    maximum: 5000,
                    default: 50,
                    description: "Key hold duration in milliseconds (default: 50ms)"
                }
            },
            required: ["keyCode"]
        }
    },
    {
        name: "hold_keycode",
        description: "Inject a key hold event with duration and optional repeat behavior",
        inputSchema: {
            type: "object",
            properties: {
                keyCode: {
                    type: "integer",
                    minimum: 0,
                    maximum: 255,
                    description: "Windows virtual key code (0-255)"
                },
                durationMs: {
                    type: "integer",
                    minimum: 0,
                    maximum: 10000,
                    description: "Total key hold duration in milliseconds"
                },
                repeat: {
                    type: "boolean",
                    default: false,
                    description: "Whether to simulate keyboard auto-repeat behavior (default: false)"
                }
            },
            required: ["keyCode", "durationMs"]
        }
    },
    {
        name: "modify_script",
        description: "Fine-grained modification of JavaScript files with 5 operations: add_line, remove_line, add_function, remove_function, replace_text",
        inputSchema: {
            type: "object",
            properties: {
                filePath: {
                    type: "string",
                    pattern: "^[^.][^/\\\\]*\\.js$",
                    description: "Relative file path within Scripts directory (must end with .js)"
                },
                operation: {
                    type: "string",
                    enum: ["add_line", "remove_line", "add_function", "remove_function", "replace_text"],
                    description: "Type of modification operation to perform"
                },
                params: {
                    type: "object",
                    description: "Operation-specific parameters (varies by operation type)"
                }
            },
            required: ["filePath", "operation", "params"]
        }
    }
];

/**
 * modify_script Operation Parameter Schemas
 *
 * add_line params:
 * {
 *   lineNumber: number (1-indexed),
 *   content: string (line content, can include \n for multi-line),
 *   position: "before" | "after" | "replace"
 * }
 *
 * remove_line params (Mode 1 - By Line Number):
 * {
 *   lineNumber: number (1-indexed),
 *   count?: number (default: 1)
 * }
 *
 * remove_line params (Mode 2 - By Pattern):
 * {
 *   pattern: string (text to match),
 *   maxMatches?: number | null (null = all matches)
 * }
 *
 * add_function params:
 * {
 *   functionCode: string (complete function definition),
 *   insertionPoint: "end_of_file" | "end_of_class" | "before_function:name" | "after_function:name",
 *   className?: string (required if insertionPoint is "end_of_class")
 * }
 *
 * remove_function params:
 * {
 *   functionName: string (name of function to remove),
 *   className?: string (optional: class containing the method)
 * }
 *
 * replace_text params:
 * {
 *   search: string (text or regex pattern),
 *   replace: string (replacement text),
 *   isRegex?: boolean (default: false),
 *   maxReplacements?: number | null (null = all occurrences)
 * }
 */

/**
 * Common Virtual Key Code Reference (for KADI broker documentation)
 *
 * Common Keys:
 * - A-Z: 0x41-0x5A (65-90)
 * - 0-9: 0x30-0x39 (48-57)
 * - F1-F12: 0x70-0x7B (112-123)
 * - Enter: 0x0D (13)
 * - Escape: 0x1B (27)
 * - Space: 0x20 (32)
 * - Arrow Keys: Left=0x25, Up=0x26, Right=0x27, Down=0x28
 *
 * Complete reference: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
 */

// Export for hot-reload
globalThis.DevelopmentTools = DevelopmentTools;

console.log('DevelopmentTools: Module loaded (6 tools defined)');
