//----------------------------------------------------------------------------------------------------
// DevelopmentToolHandler.js
// KADI development tool invocation handlers (Phase 6a)
//----------------------------------------------------------------------------------------------------

/**
 * DevelopmentToolHandler - Handles KADI tool invocations for development tools
 *
 * Architecture:
 * - Bridges JavaScript to C++ GameScriptInterface methods
 * - Handles file I/O operations (create/read/delete scripts)
 * - Handles input injection (press/hold keycodes)
 * - Provides error handling and validation
 *
 * Responsibilities:
 * - create_script: Create new .js file via C++ bridge
 * - read_script: Read existing .js file via C++ bridge
 * - delete_script: Delete .js file via C++ bridge
 * - press_keycode: Inject single key press event via C++ bridge
 * - hold_keycode: Inject multi-key sequence events with precise timing via C++ bridge
 * - get_keyhold_status: Query status of active key hold jobs
 * - cancel_keyhold: Cancel active key hold jobs
 * - list_active_keyholds: List all active key hold jobs
 * - modify_script: Fine-grained script modifications (add/remove lines, functions, replace text)
 */
export class DevelopmentToolHandler
{
    /**
     * @param {object} game - Reference to Game interface (for C++ bridge access)
     */
    constructor(game)
    {
        this.game = game;

        console.log('DevelopmentToolHandler: Initialized');
    }

    /**
     * Main tool invocation router
     * Called by KADIGameControl when broker invokes a tool
     */
    handleToolInvoke(requestId, toolName, args)
    {
        console.log(`DevelopmentToolHandler: Tool invoked - ${toolName}`, args);

        // Parse args if it's a JSON string (C++ interface might pass JSON string)
        let parsedArgs = args;
        if (typeof args === 'string')
        {
            console.log('DevelopmentToolHandler: Parsing args from JSON string');
            try
            {
                parsedArgs = JSON.parse(args);
            }
            catch (error)
            {
                console.log('DevelopmentToolHandler: ERROR - Failed to parse args JSON:', error);
                this.sendError(requestId, `Invalid arguments: ${error.message}`);
                return;
            }
        }

        try
        {
            switch (toolName)
            {
                case 'create_script':
                    this.handleCreateScript(requestId, parsedArgs);
                    break;

                case 'read_script':
                    this.handleReadScript(requestId, parsedArgs);
                    break;

                case 'delete_script':
                    this.handleDeleteScript(requestId, parsedArgs);
                    break;

                case 'input_press_keycode':
                    this.handlePressKeycode(requestId, parsedArgs);
                    break;

                case 'input_hold_keycode':
                    this.handleHoldKeycode(requestId, parsedArgs);
                    break;

                case 'get_keyhold_status':
                    this.handleGetKeyHoldStatus(requestId, parsedArgs);
                    break;

                case 'cancel_keyhold':
                    this.handleCancelKeyHold(requestId, parsedArgs);
                    break;

                case 'list_active_keyholds':
                    this.handleListActiveKeyHolds(requestId, parsedArgs);
                    break;

                case 'modify_script':
                    this.handleModifyScript(requestId, parsedArgs);
                    break;

                default:
                    this.sendError(requestId, `Unknown tool: ${toolName}`);
            }
        }
        catch (error)
        {
            console.log(`DevelopmentToolHandler: ERROR - Error handling ${toolName}:`, error);
            this.sendError(requestId, `Exception: ${error.message}`);
        }
    }

    /**
     * Tool: create_script
     * Create a new JavaScript file in Scripts directory
     */
    handleCreateScript(requestId, args)
    {
        console.log(`DevelopmentToolHandler: handleCreateScript - args:`, args);

        // Validate required parameters
        if (!args.filePath || typeof args.filePath !== 'string')
        {
            this.sendError(requestId, 'Invalid filePath: must be string');
            return;
        }

        if (!args.content || typeof args.content !== 'string')
        {
            this.sendError(requestId, 'Invalid content: must be string');
            return;
        }

        // Default overwrite to false if not specified
        const overwrite = args.overwrite !== undefined ? args.overwrite : false;

        try
        {
            // Call C++ bridge method (returns JavaScript object, not JSON string)
            // Note: V8 bridge automatically parses JSON strings starting with { or [
            const resultObj = this.game.createScriptFile(args.filePath, args.content, overwrite);

            if (resultObj.success)
            {
                console.log(`DevelopmentToolHandler: Script created successfully - ${args.filePath}`);
                kadi.sendToolResult(requestId, JSON.stringify({
                    success: true,
                    filePath: resultObj.filePath,
                    bytesWritten: resultObj.bytesWritten
                }));
            }
            else
            {
                this.sendError(requestId, resultObj.error || 'Unknown error from C++');
            }
        }
        catch (error)
        {
            console.log(`DevelopmentToolHandler: ERROR - create_script failed:`, error);
            this.sendError(requestId, `C++ bridge error: ${error.message}`);
        }
    }

    /**
     * Tool: read_script
     * Read an existing JavaScript file from Scripts directory
     */
    handleReadScript(requestId, args)
    {
        console.log(`DevelopmentToolHandler: handleReadScript - args:`, args);

        // Validate required parameters
        if (!args.filePath || typeof args.filePath !== 'string')
        {
            this.sendError(requestId, 'Invalid filePath: must be string');
            return;
        }

        try
        {
            // Call C++ bridge method (returns JavaScript object, not JSON string)
            // Note: V8 bridge automatically parses JSON strings starting with { or [
            const resultObj = this.game.readScriptFile(args.filePath);

            if (resultObj.success)
            {
                console.log(`DevelopmentToolHandler: Script read successfully - ${args.filePath} (${resultObj.lineCount} lines, ${resultObj.byteSize} bytes)`);
                kadi.sendToolResult(requestId, JSON.stringify({
                    success: true,
                    filePath: resultObj.filePath,
                    content: resultObj.content,
                    lineCount: resultObj.lineCount,
                    byteSize: resultObj.byteSize
                }));
            }
            else
            {
                this.sendError(requestId, resultObj.error || 'Unknown error from C++');
            }
        }
        catch (error)
        {
            console.log(`DevelopmentToolHandler: ERROR - read_script failed:`, error);
            this.sendError(requestId, `C++ bridge error: ${error.message}`);
        }
    }

    /**
     * Tool: delete_script
     * Delete a JavaScript file from Scripts directory
     */
    handleDeleteScript(requestId, args)
    {
        console.log(`DevelopmentToolHandler: handleDeleteScript - args:`, args);

        // Validate required parameters
        if (!args.filePath || typeof args.filePath !== 'string')
        {
            this.sendError(requestId, 'Invalid filePath: must be string');
            return;
        }

        try
        {
            // Call C++ bridge method (returns JavaScript object, not JSON string)
            // Note: V8 bridge automatically parses JSON strings starting with { or [
            const resultObj = this.game.deleteScriptFile(args.filePath);

            if (resultObj.success)
            {
                const existed = resultObj.existed;
                console.log(`DevelopmentToolHandler: Script deletion completed - ${args.filePath} (existed: ${existed})`);
                kadi.sendToolResult(requestId, JSON.stringify({
                    success: true,
                    filePath: resultObj.filePath,
                    existed: existed
                }));
            }
            else
            {
                this.sendError(requestId, resultObj.error || 'Unknown error from C++');
            }
        }
        catch (error)
        {
            console.log(`DevelopmentToolHandler: ERROR - delete_script failed:`, error);
            this.sendError(requestId, `C++ bridge error: ${error.message}`);
        }
    }

    /**
     * Tool: press_keycode
     * Inject a key press event with specified duration
     */
    handlePressKeycode(requestId, args)
    {
        console.log(`DevelopmentToolHandler: handlePressKeycode - args:`, args);

        // Validate required parameters
        if (typeof args.keyCode !== 'number' || args.keyCode < 0 || args.keyCode > 255)
        {
            this.sendError(requestId, 'Invalid keyCode: must be number 0-255');
            return;
        }

        // Default duration to 50ms if not specified
        const durationMs = args.durationMs !== undefined ? args.durationMs : 50;

        if (typeof durationMs !== 'number' || durationMs < 0)
        {
            this.sendError(requestId, 'Invalid durationMs: must be number >= 0');
            return;
        }

        try
        {
            // Call C++ bridge method (returns JavaScript object, not JSON string)
            // Note: V8 bridge automatically parses JSON strings starting with { or [
            const resultObj = this.game.injectKeyPress(args.keyCode, durationMs);

            if (resultObj.success)
            {
                console.log(`DevelopmentToolHandler: Key press injected - keyCode=${args.keyCode}, duration=${durationMs}ms`);
                kadi.sendToolResult(requestId, JSON.stringify({
                    success: true,
                    keyCode: resultObj.keyCode,
                    durationMs: resultObj.durationMs
                }));
            }
            else
            {
                this.sendError(requestId, resultObj.error || 'Unknown error from C++');
            }
        }
        catch (error)
        {
            console.log(`DevelopmentToolHandler: ERROR - press_keycode failed:`, error);
            this.sendError(requestId, `C++ bridge error: ${error.message}`);
        }
    }

    /**
     * Tool: hold_keycode
     * Inject a key hold event with duration and optional repeat
     * Enhanced to support multi-key sequences with precise timing control
     */
    handleHoldKeycode(requestId, args)
    {
        console.log(`DevelopmentToolHandler: handleHoldKeycode - args:`, args);

        // Validate required keySequence parameter
        if (!args.keySequence || !Array.isArray(args.keySequence))
        {
            this.sendError(requestId, 'Invalid keySequence: must be array of key objects');
            return;
        }

        if (args.keySequence.length === 0)
        {
            this.sendError(requestId, 'Invalid keySequence: cannot be empty');
            return;
        }

        // Validate each key in the sequence
        for (let i = 0; i < args.keySequence.length; i++)
        {
            const key = args.keySequence[i];

            // Validate keyCode
            if (typeof key.keyCode !== 'number' || key.keyCode < 0 || key.keyCode > 255)
            {
                this.sendError(requestId, `Invalid keyCode in keySequence[${i}]: must be number 0-255`);
                return;
            }

            // Validate delayMs (default to 0 if not provided)
            if (key.delayMs !== undefined)
            {
                if (typeof key.delayMs !== 'number' || key.delayMs < 0)
                {
                    this.sendError(requestId, `Invalid delayMs in keySequence[${i}]: must be number >= 0`);
                    return;
                }
            }

            // Validate durationMs (required)
            if (typeof key.durationMs !== 'number' || key.durationMs < 0)
            {
                this.sendError(requestId, `Invalid durationMs in keySequence[${i}]: must be number >= 0`);
                return;
            }
        }

        try
        {
            // Call C++ bridge method with JSON string
            // The V8 bridge can only pass primitive types (string, number, bool)
            // So we serialize the args object to JSON string
            const resultObj = this.game.injectKeyHold(JSON.stringify(args));

            if (resultObj.success)
            {
                console.log(`DevelopmentToolHandler: Key sequence injected - ${args.keySequence.length} keys, primaryJobId=${resultObj.primaryJobId}`);
                kadi.sendToolResult(requestId, JSON.stringify({
                    success: true,
                    primaryJobId: resultObj.primaryJobId,
                    keyCount: resultObj.keyCount,
                    keySequence: args.keySequence.map(key => ({
                        keyCode: key.keyCode,
                        delayMs: key.delayMs || 0,
                        durationMs: key.durationMs
                    }))
                }));
            }
            else
            {
                this.sendError(requestId, resultObj.error || 'Unknown error from C++');
            }
        }
        catch (error)
        {
            console.log(`DevelopmentToolHandler: ERROR - hold_keycode failed:`, error);
            this.sendError(requestId, `C++ bridge error: ${error.message}`);
        }
    }

    /**
     * Tool: get_keyhold_status
     * Get status of a key hold job by ID
     */
    handleGetKeyHoldStatus(requestId, args)
    {
        console.log(`DevelopmentToolHandler: handleGetKeyHoldStatus - args:`, args);

        // Validate required parameters
        if (typeof args.jobId !== 'number' || args.jobId < 1)
        {
            this.sendError(requestId, 'Invalid jobId: must be number >= 1');
            return;
        }

        try
        {
            const resultObj = this.game.getKeyHoldStatus(args.jobId);

            if (resultObj.success)
            {
                console.log(`DevelopmentToolHandler: Got key hold status - jobId=${args.jobId}, status=${resultObj.status}`);
                kadi.sendToolResult(requestId, JSON.stringify(resultObj));
            }
            else
            {
                this.sendError(requestId, resultObj.error || 'Unknown error from C++');
            }
        }
        catch (error)
        {
            console.log(`DevelopmentToolHandler: ERROR - get_keyhold_status failed:`, error);
            this.sendError(requestId, `C++ bridge error: ${error.message}`);
        }
    }

    /**
     * Tool: cancel_keyhold
     * Cancel an active key hold job by ID
     */
    handleCancelKeyHold(requestId, args)
    {
        console.log(`DevelopmentToolHandler: handleCancelKeyHold - args:`, args);

        // Validate required parameters
        if (typeof args.jobId !== 'number' || args.jobId < 1)
        {
            this.sendError(requestId, 'Invalid jobId: must be number >= 1');
            return;
        }

        try
        {
            const resultObj = this.game.cancelKeyHold(args.jobId);

            if (resultObj.success)
            {
                console.log(`DevelopmentToolHandler: Cancelled key hold - jobId=${args.jobId}, cancelled=${resultObj.cancelled}`);
                kadi.sendToolResult(requestId, JSON.stringify(resultObj));
            }
            else
            {
                this.sendError(requestId, resultObj.error || 'Unknown error from C++');
            }
        }
        catch (error)
        {
            console.log(`DevelopmentToolHandler: ERROR - cancel_keyhold failed:`, error);
            this.sendError(requestId, `C++ bridge error: ${error.message}`);
        }
    }

    /**
     * Tool: list_active_keyholds
     * List all active key hold jobs
     */
    handleListActiveKeyHolds(requestId, args)
    {
        console.log(`DevelopmentToolHandler: handleListActiveKeyHolds - args:`, args);

        try
        {
            const resultObj = this.game.listActiveKeyHolds();

            if (resultObj.success)
            {
                console.log(`DevelopmentToolHandler: Listed active key holds - count=${resultObj.count}`);
                kadi.sendToolResult(requestId, JSON.stringify(resultObj));
            }
            else
            {
                this.sendError(requestId, resultObj.error || 'Unknown error from C++');
            }
        }
        catch (error)
        {
            console.log(`DevelopmentToolHandler: ERROR - list_active_keyholds failed:`, error);
            this.sendError(requestId, `C++ bridge error: ${error.message}`);
        }
    }

    /**
     * Tool: modify_script (Phase 6b)
     * Fine-grained modification of existing JavaScript files
     * Supports 5 operations: add_line, remove_line, add_function, remove_function, replace_text
     */
    handleModifyScript(requestId, args)
    {
        console.log(`DevelopmentToolHandler: handleModifyScript - args:`, args);

        // Validate required parameters
        if (!args.filePath || typeof args.filePath !== 'string')
        {
            this.sendError(requestId, 'Invalid filePath: must be string');
            return;
        }

        if (!args.operation || typeof args.operation !== 'string')
        {
            this.sendError(requestId, 'Invalid operation: must be string');
            return;
        }

        if (!args.params || typeof args.params !== 'object')
        {
            this.sendError(requestId, 'Invalid params: must be object');
            return;
        }

        // Route to specific operation handler
        try
        {
            let result;
            switch (args.operation)
            {
                case 'add_line':
                    result = this.modifyScript_AddLine(args.filePath, args.params);
                    break;

                case 'remove_line':
                    result = this.modifyScript_RemoveLine(args.filePath, args.params);
                    break;

                case 'add_function':
                    result = this.modifyScript_AddFunction(args.filePath, args.params);
                    break;

                case 'remove_function':
                    result = this.modifyScript_RemoveFunction(args.filePath, args.params);
                    break;

                case 'replace_text':
                    result = this.modifyScript_ReplaceText(args.filePath, args.params);
                    break;

                default:
                    this.sendError(requestId, `Unknown modify_script operation: ${args.operation}`);
                    return;
            }

            // Send result
            kadi.sendToolResult(requestId, JSON.stringify(result));
        }
        catch (error)
        {
            console.log(`DevelopmentToolHandler: ERROR - modify_script failed:`, error);
            this.sendError(requestId, `modify_script error: ${error.message}`);
        }
    }

    /**
     * Operation: add_line
     * Insert line(s) at specific position
     */
    modifyScript_AddLine(filePath, params)
    {
        const { lineNumber, content, position } = params;

        // Validate parameters
        if (typeof lineNumber !== 'number' || lineNumber < 1)
        {
            return { success: false, error: 'Invalid lineNumber: must be number >= 1' };
        }

        if (typeof content !== 'string')
        {
            return { success: false, error: 'Invalid content: must be string' };
        }

        if (!['before', 'after', 'replace'].includes(position))
        {
            return { success: false, error: 'Invalid position: must be "before", "after", or "replace"' };
        }

        try
        {
            // Read file
            const readResult = this.game.readScriptFile(filePath);
            if (!readResult.success)
            {
                return { success: false, error: readResult.error || 'Failed to read file' };
            }

            // Split into lines
            const lines = readResult.content.split('\n');

            // Validate line number
            if (lineNumber > lines.length + 1)
            {
                return { success: false, error: `Invalid line number ${lineNumber}: file has ${lines.length} lines` };
            }

            // Convert to 0-indexed
            const index = lineNumber - 1;

            // Insert based on position
            let linesModified = 1;
            switch (position)
            {
                case 'before':
                    lines.splice(index, 0, content);
                    break;

                case 'after':
                    lines.splice(index + 1, 0, content);
                    break;

                case 'replace':
                    lines[index] = content;
                    break;
            }

            // Handle multi-line content
            if (content.includes('\n'))
            {
                const contentLines = content.split('\n');
                linesModified = contentLines.length;
            }

            // Write back
            const newContent = lines.join('\n');
            const writeResult = this.game.createScriptFile(filePath, newContent, true);

            if (!writeResult.success)
            {
                return { success: false, error: writeResult.error || 'Failed to write file' };
            }

            return {
                success: true,
                filePath: filePath,
                operation: 'add_line',
                linesModified: linesModified,
                timestamp: Date.now()
            };
        }
        catch (error)
        {
            return { success: false, error: `add_line exception: ${error.message}` };
        }
    }

    /**
     * Operation: remove_line
     * Remove line(s) by line number or pattern
     */
    modifyScript_RemoveLine(filePath, params)
    {
        try
        {
            // Read file
            const readResult = this.game.readScriptFile(filePath);
            if (!readResult.success)
            {
                return { success: false, error: readResult.error || 'Failed to read file' };
            }

            const lines = readResult.content.split('\n');
            let linesModified = 0;
            let newLines;

            // Mode 1: Remove by line number
            if (params.lineNumber !== undefined)
            {
                const { lineNumber, count = 1 } = params;

                // Validate parameters
                if (typeof lineNumber !== 'number' || lineNumber < 1 || lineNumber > lines.length)
                {
                    return { success: false, error: `Invalid lineNumber ${lineNumber}: file has ${lines.length} lines` };
                }

                if (typeof count !== 'number' || count < 1)
                {
                    return { success: false, error: 'Invalid count: must be number >= 1' };
                }

                // Convert to 0-indexed
                const index = lineNumber - 1;

                // Remove lines
                lines.splice(index, count);
                linesModified = count;
                newLines = lines;
            }
            // Mode 2: Remove by pattern
            else if (params.pattern !== undefined)
            {
                const { pattern, maxMatches = null } = params;

                if (typeof pattern !== 'string')
                {
                    return { success: false, error: 'Invalid pattern: must be string' };
                }

                // Filter lines that don't match pattern
                newLines = lines.filter(line => {
                    if (maxMatches !== null && linesModified >= maxMatches)
                    {
                        return true; // Keep line (reached max)
                    }

                    const matches = line.includes(pattern);
                    if (matches)
                    {
                        linesModified++;
                        return false; // Remove line
                    }
                    return true; // Keep line
                });
            }
            else
            {
                return { success: false, error: 'Must provide either lineNumber or pattern' };
            }

            // Write back
            const newContent = newLines.join('\n');
            const writeResult = this.game.createScriptFile(filePath, newContent, true);

            if (!writeResult.success)
            {
                return { success: false, error: writeResult.error || 'Failed to write file' };
            }

            return {
                success: true,
                filePath: filePath,
                operation: 'remove_line',
                linesModified: linesModified,
                timestamp: Date.now()
            };
        }
        catch (error)
        {
            return { success: false, error: `remove_line exception: ${error.message}` };
        }
    }

    /**
     * Operation: add_function
     * Add new function or method to class/file
     */
    modifyScript_AddFunction(filePath, params)
    {
        const { functionCode, insertionPoint, className } = params;

        // Validate parameters
        if (typeof functionCode !== 'string')
        {
            return { success: false, error: 'Invalid functionCode: must be string' };
        }

        if (typeof insertionPoint !== 'string')
        {
            return { success: false, error: 'Invalid insertionPoint: must be string' };
        }

        try
        {
            // Read file
            const readResult = this.game.readScriptFile(filePath);
            if (!readResult.success)
            {
                return { success: false, error: readResult.error || 'Failed to read file' };
            }

            const lines = readResult.content.split('\n');
            let insertIndex = -1;

            // Find insertion point
            if (insertionPoint === 'end_of_file')
            {
                insertIndex = lines.length;
            }
            else if (insertionPoint === 'end_of_class' && className)
            {
                insertIndex = this.findClassEnd(lines, className);
                if (insertIndex === -1)
                {
                    return { success: false, error: `Class not found: ${className}` };
                }
            }
            else if (insertionPoint.startsWith('before_function:'))
            {
                const targetFunc = insertionPoint.split(':')[1];
                insertIndex = this.findFunctionStart(lines, targetFunc);
                if (insertIndex === -1)
                {
                    return { success: false, error: `Function not found: ${targetFunc}` };
                }
            }
            else if (insertionPoint.startsWith('after_function:'))
            {
                const targetFunc = insertionPoint.split(':')[1];
                const funcStart = this.findFunctionStart(lines, targetFunc);
                if (funcStart === -1)
                {
                    return { success: false, error: `Function not found: ${targetFunc}` };
                }
                insertIndex = this.findFunctionEnd(lines, funcStart) + 1;
            }
            else
            {
                return { success: false, error: `Invalid insertion point: ${insertionPoint}` };
            }

            if (insertIndex === -1)
            {
                return { success: false, error: `Could not find insertion point: ${insertionPoint}` };
            }

            // Insert function (with blank lines for formatting)
            lines.splice(insertIndex, 0, '', functionCode, '');

            // Write back
            const newContent = lines.join('\n');
            const writeResult = this.game.createScriptFile(filePath, newContent, true);

            if (!writeResult.success)
            {
                return { success: false, error: writeResult.error || 'Failed to write file' };
            }

            const linesModified = functionCode.split('\n').length;

            return {
                success: true,
                filePath: filePath,
                operation: 'add_function',
                linesModified: linesModified,
                timestamp: Date.now()
            };
        }
        catch (error)
        {
            return { success: false, error: `add_function exception: ${error.message}` };
        }
    }

    /**
     * Operation: remove_function
     * Delete function or method by name
     */
    modifyScript_RemoveFunction(filePath, params)
    {
        const { functionName, className } = params;

        // Validate parameters
        if (typeof functionName !== 'string')
        {
            return { success: false, error: 'Invalid functionName: must be string' };
        }

        try
        {
            // Read file
            const readResult = this.game.readScriptFile(filePath);
            if (!readResult.success)
            {
                return { success: false, error: readResult.error || 'Failed to read file' };
            }

            const lines = readResult.content.split('\n');

            // Find function
            const funcStart = this.findFunctionStart(lines, functionName, className);
            if (funcStart === -1)
            {
                return { success: false, error: `Function not found: ${functionName}` };
            }

            // Find function end
            const funcEnd = this.findFunctionEnd(lines, funcStart);
            if (funcEnd === -1)
            {
                return { success: false, error: `Could not find function end: ${functionName}` };
            }

            // Remove function lines
            const removedCount = funcEnd - funcStart + 1;
            lines.splice(funcStart, removedCount);

            // Write back
            const newContent = lines.join('\n');
            const writeResult = this.game.createScriptFile(filePath, newContent, true);

            if (!writeResult.success)
            {
                return { success: false, error: writeResult.error || 'Failed to write file' };
            }

            return {
                success: true,
                filePath: filePath,
                operation: 'remove_function',
                linesModified: removedCount,
                timestamp: Date.now()
            };
        }
        catch (error)
        {
            return { success: false, error: `remove_function exception: ${error.message}` };
        }
    }

    /**
     * Operation: replace_text
     * Find and replace text patterns (literal or regex)
     */
    modifyScript_ReplaceText(filePath, params)
    {
        const { search, replace, isRegex = false, maxReplacements = null } = params;

        // Validate parameters
        if (typeof search !== 'string')
        {
            return { success: false, error: 'Invalid search: must be string' };
        }

        if (typeof replace !== 'string')
        {
            return { success: false, error: 'Invalid replace: must be string' };
        }

        try
        {
            // Read file
            const readResult = this.game.readScriptFile(filePath);
            if (!readResult.success)
            {
                return { success: false, error: readResult.error || 'Failed to read file' };
            }

            let fileContent = readResult.content;
            let replacementCount = 0;

            if (isRegex)
            {
                // Regex mode
                try
                {
                    const regex = new RegExp(search, 'g');
                    fileContent = fileContent.replace(regex, (match) => {
                        if (maxReplacements !== null && replacementCount >= maxReplacements)
                        {
                            return match; // Don't replace
                        }
                        replacementCount++;
                        return replace;
                    });
                }
                catch (error)
                {
                    return { success: false, error: `Invalid regex pattern: ${error.message}` };
                }
            }
            else
            {
                // Literal mode
                if (maxReplacements === null)
                {
                    // Replace all
                    const beforeLength = fileContent.length;
                    fileContent = fileContent.replaceAll(search, replace);

                    // Estimate replacement count
                    if (search.length > 0)
                    {
                        const afterLength = fileContent.length;
                        const lengthDiff = afterLength - beforeLength;
                        const replaceLengthDiff = replace.length - search.length;
                        if (replaceLengthDiff !== 0)
                        {
                            replacementCount = Math.abs(Math.floor(lengthDiff / replaceLengthDiff));
                        }
                        else
                        {
                            // Same length replacement - count occurrences
                            replacementCount = (beforeLength - fileContent.replace(new RegExp(search.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'g'), '').length) / search.length;
                        }
                    }
                }
                else
                {
                    // Replace with limit
                    let remaining = maxReplacements;
                    while (remaining > 0 && fileContent.includes(search))
                    {
                        fileContent = fileContent.replace(search, replace);
                        replacementCount++;
                        remaining--;
                    }
                }
            }

            // Write back
            const writeResult = this.game.createScriptFile(filePath, fileContent, true);

            if (!writeResult.success)
            {
                return { success: false, error: writeResult.error || 'Failed to write file' };
            }

            return {
                success: true,
                filePath: filePath,
                operation: 'replace_text',
                linesModified: replacementCount,
                timestamp: Date.now()
            };
        }
        catch (error)
        {
            return { success: false, error: `replace_text exception: ${error.message}` };
        }
    }

    // ==========================================
    // Helper Methods for Pattern Matching
    // ==========================================

    /**
     * Find the closing brace of a class
     */
    findClassEnd(lines, className)
    {
        let classStart = -1;
        let braceDepth = 0;

        // Find class start
        for (let i = 0; i < lines.length; i++)
        {
            if (lines[i].match(new RegExp(`class\\s+${className}`)))
            {
                classStart = i;
                break;
            }
        }

        if (classStart === -1) return -1;

        // Find matching closing brace
        for (let i = classStart; i < lines.length; i++)
        {
            const line = lines[i];
            braceDepth += (line.match(/\{/g) || []).length;
            braceDepth -= (line.match(/\}/g) || []).length;

            if (braceDepth === 0 && i > classStart)
            {
                return i; // Found closing brace
            }
        }

        return -1;
    }

    /**
     * Find the start of a function
     */
    findFunctionStart(lines, functionName, className = null)
    {
        // If className specified, first find class boundaries
        let searchStart = 0;
        let searchEnd = lines.length;

        if (className)
        {
            searchStart = -1;
            for (let i = 0; i < lines.length; i++)
            {
                if (lines[i].match(new RegExp(`class\\s+${className}`)))
                {
                    searchStart = i;
                    break;
                }
            }

            if (searchStart === -1) return -1;

            searchEnd = this.findClassEnd(lines, className);
            if (searchEnd === -1) return -1;
        }

        // Search for function within bounds
        for (let i = searchStart; i < searchEnd; i++)
        {
            const line = lines[i];
            // Match: function functionName( or functionName( or functionName()
            if (line.match(new RegExp(`(function\\s+)?${functionName}\\s*\\(`)))
            {
                return i;
            }
        }

        return -1;
    }

    /**
     * Find the closing brace of a function
     */
    findFunctionEnd(lines, startIndex)
    {
        let braceDepth = 0;
        let inFunction = false;

        for (let i = startIndex; i < lines.length; i++)
        {
            const line = lines[i];
            braceDepth += (line.match(/\{/g) || []).length;
            braceDepth -= (line.match(/\}/g) || []).length;

            if (braceDepth > 0) inFunction = true;

            if (inFunction && braceDepth === 0)
            {
                return i; // Found closing brace
            }
        }

        return -1;
    }

    /**
     * Helper: Send error result to KADI broker
     */
    sendError(requestId, errorMessage)
    {
        console.log(`DevelopmentToolHandler: ERROR - ${errorMessage}`);
        kadi.sendToolResult(requestId, JSON.stringify({
            success: false,
            error: errorMessage
        }));
    }
}

// Export for hot-reload
globalThis.DevelopmentToolHandler = DevelopmentToolHandler;

console.log('DevelopmentToolHandler: Module loaded');
