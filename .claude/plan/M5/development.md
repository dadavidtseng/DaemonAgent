# M5: Advanced AI Agent Capabilities

**Status**: ⏳ In Progress (2/9 tasks active - M5-T1 + M5-T2)
**Timeline**: October 28 - November 10, 2025 (2 weeks)
**Started**: October 28, 2025
**Last Synced**: November 3, 2025 (from Notion)

---

## 📑 Table of Contents

### Quick Navigation
- [Milestone Overview](#milestone-overview)
- [Overall Progress Summary](#overall-progress-summary)
- [Task Breakdown](#task-breakdown)
  - [Task M5-T1: Game Template System & AI Agent Demo](#m5-t1-game-template-system--ai-agent-platformer-demo)
  - [Task M5-T2: Agent-to-Agent Communication Protocol](#m5-t2-agent-to-agent-communication-protocol)
  - [Task M5-T3: Message Routing and Threading](#m5-t3-message-routing-and-threading)
  - [Task M5-T5: Natural Language Game Design Processing](#m5-t5-natural-language-game-design-processing)
  - [Task M5-T6: Workflow Decomposition Algorithms](#m5-t6-workflow-decomposition-algorithms)
  - [Task M5-T7: Context Preservation](#m5-t7-context-preservation)
  - [Task M5-T8: Coordination Patterns](#m5-t8-coordination-patterns---pipelineparalleliterative)
  - [Task M5-T9: Learning Mechanisms and Memory](#m5-t9-learning-mechanisms-and-memory)
  - [Task M5-T10: Architecture Analysis Presentation](#m5-t10-architecture-analysis-powerpoint-presentation)
- [Integration Architecture](#integration-architecture)
- [Critical Dependencies](#critical-dependencies)
- [Risk Assessment](#risk-assessment)
- [Success Metrics](#success-metrics)
- [References and Documentation](#references-and-documentation)

---

## Milestone Overview

### Objective
Build advanced multi-agent collaboration infrastructure enabling natural language game design, autonomous workflow decomposition, and coordinated agent execution patterns.

### Timeline
- **Start Date**: October 28, 2025
- **End Date**: November 10, 2025 (2 weeks)
- **Current Status**: Not Started (BLOCKED by M4-T5)
- **Days Available**: 14 days

### Success Criteria
1. ❌ Template system enables rapid game creation from predefined patterns
2. ❌ AI agent demo shows Claude creating platformer game via natural language
3. ❌ Multiple AI agents can communicate and coordinate through KĀDI broker
4. ❌ Natural language game design requests are decomposed into actionable tasks
5. ❌ Agents maintain shared context across multi-turn conversations
6. ❌ Three coordination patterns implemented: pipeline, parallel, and iterative
7. ❌ Agent memory system preserves learning across sessions
8. ❌ Demo: Claude designs a game through conversational workflow

### Dependencies

**Milestone Dependencies:**
- **M4-T5 (MCP Server)** - ✅ CRITICAL BLOCKER - Must complete before M5 starts
- M4-T6 (KĀDI JavaScript Library) - ✅ COMPLETE
- M4-T7 (Basic AI Agent Integration) - ✅ COMPLETE
- M4-T8 (Async Architecture) - ⏳ ALMOST DONE

**External Dependencies:**
- KĀDI broker with MCP support - ⏳ Required
- Claude Desktop configured and connected - ❌ Blocked by M4-T5
- RabbitMQ message routing - ✅ Available
- Multi-agent test environment - ❌ Not yet created

**Critical Blocker:**
- **M4-T5 MUST be completed by Oct 27, 2025 for M5 to start on time**
- Without MCP server, Claude Desktop cannot connect
- Entire M5 milestone is blocked without this foundation

---

## Overall Progress Summary

### Completed (0/9 tasks - 0%)
- ❌ No tasks completed yet

### In Progress (2/9 tasks - 22%)
- ⏳ **M5-T1**: Game template system & AI agent demo (0h/8h) - **Status: Added** (merged M4-T10 + M4-T11)
- ⏳ **M5-T2**: Agent-to-agent communication protocol (0h/12h) - **Status: In Progress** (HIGH PRIORITY)

### Not Started (7/9 tasks - 78%)
- ❌ **M5-T3**: Message routing and threading (0h/6h) - **Status: Not Started** (Medium Priority)
- ❌ **M5-T5**: Natural language game design processing (0h/8h) - **Status: Not Started** (High Priority)
- ❌ **M5-T6**: Workflow decomposition algorithms (0h/4h) - **Status: Not Started** (Low Priority)
- ❌ **M5-T7**: Context preservation (0h/10h) - **Status: Not Started** (High Priority)
- ❌ **M5-T8**: Coordination patterns (0h/15h) - **Status: Not Started** (High Priority)
- ❌ **M5-T9**: Learning mechanisms and memory (0h/6h) - **Status: Not Started** (Low Priority)
- ❌ **M5-T10**: Architecture analysis PowerPoint presentation (0h/6h) - **Status: Added** (High Priority, Due: Nov 5)

### Hours Summary
- **Completed**: 0h
- **In Progress**: 20h (M5-T1: 8h + M5-T2: 12h)
- **Remaining**: 55h (7 tasks)
- **Total M5 Effort**: 75 hours
- **Current Completion**: 0%

**Note**: Task statuses updated from Notion on Nov 3, 2025. M5-T2 is now active in addition to M5-T1.

---

## Task Breakdown

### M5-T1: Game Template System & AI Agent Platformer Demo

**Status**: Not Started (Merged from M4-T10 + M4-T11)
**Expected Hours**: 8h (3h template system + 5h demo)
**Priority**: HIGH (Foundation for AI agent game creation)
**Type**: Feature + Demo
**Dependencies**: M4-T5 (MCP Server) must be complete

#### Objective
Create a unified workflow combining a template-based game creation system with a comprehensive demonstration of AI agent capabilities. This task merges the original M4-T10 (template system) and M4-T11 (platformer demo) into a single, coherent implementation that showcases AI-driven game development.

#### Technical Approach

**Part 1: Template System (3h)**

Create a flexible template system for quickly generating new game projects from JavaScript through KADI tools.

**Template Architecture:**
```javascript
// Template Definition Format (JSON-based)
{
  "templateId": "platformer-basic",
  "name": "Basic Platformer Template",
  "description": "Simple platformer with player, platforms, and enemies",
  "version": "1.0",
  "entities": [
    {
      "type": "player",
      "component": "PlayerController",
      "properties": {
        "position": [0, 0, 0],
        "jumpForce": 10,
        "moveSpeed": 5
      }
    },
    {
      "type": "platform",
      "component": "StaticPlatform",
      "count": 5,
      "properties": {
        "positions": [[0, -2, 0], [5, -1, 0], ...],
        "scale": [2, 0.5, 1]
      }
    },
    {
      "type": "enemy",
      "component": "EnemyAI",
      "count": 3,
      "properties": {
        "patrolPath": [...],
        "speed": 2
      }
    }
  ],
  "systems": ["InputSystem", "PhysicsSystem", "CameraSystem"],
  "gameRules": {
    "winCondition": "reach_goal",
    "loseCondition": "touch_enemy"
  }
}
```

**Part 2: AI Agent Demo (5h)**

Demonstrate end-to-end AI agent capability by having Claude create a platformer game through tool invocations.

**Demo Workflow:**
1. **Agent Connection** (Manual Setup)
   - Claude Desktop connects via MCP
   - Tool discovery (`tools/list`)
   - Verify game control tools available

2. **Game Creation via Natural Language** (AI-Driven)
   - User: "Create a simple platformer game"
   - Claude uses `create_game_from_template` tool
   - Claude spawns entities using `spawn_cube` tools
   - Claude queries state using `get_game_state` tool
   - Claude adjusts positions using `move_cube` tool

3. **Documentation and Recording** (2h)
   - Screen recording of full workflow
   - Document setup steps for reproduction
   - Create presentation slides
   - Write thesis section on AI agent demo

#### Implementation Steps

**Phase 1: Template Definition (1h)**
1. Design JSON schema for game templates
2. Identify common game components (player, platforms, enemies, collectibles)
3. Create template parameter system (customizable values)
4. Define 2-3 basic templates (platformer, top-down, simple puzzle)

**Phase 2: Template Loader Implementation (1h)**
1. Create `TemplateLoader.js` in game engine
2. Implement JSON template parser
3. Add template instantiation logic (spawn entities from template)
4. Create validation for template syntax
5. Test template loading and entity creation

**Phase 3: KĀDI Tool Integration (1h)**
1. Add `create_game_from_template` tool schema in `DevelopmentTools.js`
2. Implement tool handler in `DevelopmentToolHandler.js`
3. Connect handler to TemplateLoader
4. Test template creation through KADI MCP
5. Verify error handling for invalid templates

**Phase 4: Demo Setup (2h)**
1. Configure Claude Desktop with MCP server (M4-T5 should be complete)
2. Test tool discovery from Claude Desktop
3. Create demo script (sequence of natural language commands)
4. Prepare test environment with clean game state
5. Rehearse demo workflow

**Phase 5: Demo Execution and Documentation (3h)**
1. Record full demo: User prompt → Claude → Game created
2. Capture screenshots at each step
3. Document any issues or edge cases discovered
4. Write setup instructions for reproduction
5. Create presentation materials for thesis
6. Document lessons learned

#### Files to Create/Modify

**Game Engine JavaScript:**
- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\templates\TemplateLoader.js` (NEW)
  - Template parsing and validation
  - Entity instantiation from templates
  - Error handling

- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\templates\basic-platformer.json` (NEW)
  - Basic platformer template definition

- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\DevelopmentTools.js` (MODIFY)
  - Add `create_game_from_template` tool schema

- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\DevelopmentToolHandler.js` (MODIFY)
  - Add handler for template creation tool
  - Integrate with TemplateLoader

**Documentation:**
- `C:\p4\Personal\SD\ProtogameJS3D\Docs\demo-setup.md` (NEW)
  - Setup instructions for Claude Desktop
  - Demo script and expected outputs
  - Troubleshooting guide

- `C:\p4\Personal\SD\ProtogameJS3D\Docs\demo-recording.md` (NEW)
  - Links to demo videos
  - Screenshots and step-by-step walkthrough
  - Analysis of AI agent behavior

#### Acceptance Criteria

**Template System:**
- ✅ JSON-based template format defined and validated
- ✅ Template loader can parse and instantiate templates
- ✅ `create_game_from_template` tool accessible via KADI MCP
- ✅ At least 2 working templates (platformer + one other)
- ✅ Error handling prevents crashes from invalid templates

**AI Agent Demo:**
- ✅ Claude Desktop successfully connects to game engine via MCP
- ✅ Claude can discover and invoke game control tools
- ✅ Full demo workflow: Natural language → Working platformer game
- ✅ Demo completes in < 5 minutes from start to playable game
- ✅ Demo recorded and documented with screenshots
- ✅ Reproducible by following setup instructions

#### Risk Assessment

**Medium Risk**: Depends on M4-T5 (MCP server) completion
- **Mitigation**: M4-T5 should be complete before M5 starts
- **Contingency**: Use KĀDI test client instead of Claude Desktop for fallback demo

**Low Risk**: Template system implementation is straightforward
- **Mitigation**: JSON parsing and validation are well-understood
- **Contingency**: Start with single template if time limited

**Medium Risk**: Demo may reveal unexpected integration issues
- **Mitigation**: Thorough testing before recording
- **Contingency**: Document issues as "future work" if critical

#### Success Metrics
- Template loading time: < 1 second
- Demo completion time: < 5 minutes (natural language → playable game)
- Tool invocation success rate: > 95%
- Demo reproducibility: 100% (anyone can follow instructions and reproduce)

---

### M5-T2: Agent-to-Agent Communication Protocol

**Status**: In Progress
**Expected Hours**: 12h
**Priority**: HIGH (Foundation for all multi-agent features)
**Type**: Feature
**Dependencies**: M4-T5 (MCP Server) must be complete

#### Objective
Establish standardized communication protocols enabling multiple AI agents to exchange messages, coordinate actions, and share state through the KĀDI broker infrastructure.

#### Technical Approach

**Architecture Design:**
```
Agent A (Claude Desktop via MCP)
  → KĀDI Broker (Message Router)
    → RabbitMQ (Message Queue)
      → KĀDI Broker (Delivery)
        → Agent B (Game Engine via KĀDI)
```

**Message Protocol:**
```typescript
interface AgentMessage {
  messageId: string;          // Unique message identifier
  threadId: string;           // Conversation thread
  fromAgent: string;          // Source agent session ID
  toAgent: string | "all";    // Target agent or broadcast
  messageType: "request" | "response" | "notification" | "query";
  payload: {
    action?: string;          // Requested action
    data: any;                // Message data
    context?: ContextRef;     // Shared context reference
  };
  timestamp: number;
  priority: "low" | "normal" | "high" | "urgent";
}
```

**Communication Patterns:**
1. **Direct Messaging**: Agent A → Agent B (point-to-point)
2. **Broadcasting**: Agent A → All agents in network
3. **Request-Response**: Agent A requests → Agent B responds
4. **Publish-Subscribe**: Agents subscribe to topics, receive relevant messages

#### Implementation Steps

**Phase 1: Protocol Definition (3h)**
1. Define AgentMessage schema with JSON Schema validation
2. Create message type enums and constants
3. Document protocol specification
4. Create TypeScript interfaces for type safety

**Phase 2: KĀDI Broker Enhancement (4h)**
1. Extend KĀDI broker to support agent-to-agent routing
2. Implement message threading system
3. Add message priority queues
4. Create broadcast/multicast support
5. Test with multiple connected agents

**Phase 3: JavaScript Integration (3h)**
1. Create `AgentCommunication.js` in game engine
2. Implement message sending/receiving APIs
3. Add message validation and error handling
4. Create event emitter for incoming messages
5. Test bidirectional communication

**Phase 4: Testing and Documentation (2h)**
1. Test Agent A → Agent B communication
2. Test broadcasting to multiple agents
3. Test message threading preservation
4. Document API and usage examples

#### Files to Create/Modify

**KĀDI Broker (TypeScript):**
- `C:\p4\Personal\SD\kadi-broker\src\services\AgentRouter.ts` (NEW)
  - Agent-to-agent message routing logic
  - Thread management
  - Priority queue handling

**Game Engine JavaScript:**
- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\AgentCommunication.js` (NEW)
  - Agent communication API for JavaScript
  - Message sending/receiving
  - Event handling for incoming messages

**Test Files:**
- `C:\p4\Personal\SD\kadi-broker\tests\agent-communication.test.ts` (NEW)
  - Unit tests for agent routing
  - Integration tests with multiple agents

#### Acceptance Criteria
- ❌ Two agents can exchange direct messages successfully
- ❌ Broadcasting messages reach all agents in network
- ❌ Message threading preserves conversation context
- ❌ Priority messages processed before normal messages
- ❌ Error handling prevents message loss
- ❌ Communication latency < 50ms for local network

#### Risk Assessment
**Medium Risk**: Requires KĀDI broker modifications
- **Mitigation**: KĀDI broker already has routing infrastructure
- **Contingency**: Simplify to direct messaging only (no broadcast)

---

### M5-T3: Message Routing and Threading

**Status**: Not Started
**Expected Hours**: 6h
**Priority**: MEDIUM
**Type**: Feature
**Dependencies**: M5-T2 (Agent Communication Protocol)

#### Objective
Implement intelligent message routing with conversation threading, enabling multi-turn dialogues and complex coordination workflows between agents.

#### Technical Approach

**Threading Model:**
```typescript
interface ConversationThread {
  threadId: string;              // UUID for thread
  initiatorAgent: string;        // Who started the conversation
  participants: string[];        // All agents in thread
  rootMessageId: string;         // First message in thread
  messages: AgentMessage[];      // Ordered message history
  state: "active" | "paused" | "completed";
  metadata: {
    purpose: string;             // Thread purpose (e.g., "game design")
    createdAt: number;
    lastActivityAt: number;
  };
}
```

**Routing Strategies:**
1. **Thread-based Routing**: Messages with same threadId go to same agents
2. **Role-based Routing**: Messages routed based on agent roles (designer, implementer, etc.)
3. **Load Balancing**: Distribute messages across multiple agents of same type
4. **Fallback Routing**: Reroute if primary agent unavailable

#### Implementation Steps

**Phase 1: Thread Management (3h)**
1. Create `ThreadManager` class in KĀDI broker
2. Implement thread creation, lookup, and lifecycle
3. Add thread state management (active/paused/completed)
4. Persist thread metadata to RabbitMQ

**Phase 2: Intelligent Routing (2h)**
1. Implement routing decision logic based on thread context
2. Add role-based routing for specialized agents
3. Create fallback routing when agents disconnect
4. Test routing with complex multi-agent scenarios

**Phase 3: Integration and Testing (1h)**
1. Integrate ThreadManager with AgentRouter
2. Test multi-turn conversations with threading
3. Verify message ordering within threads
4. Document routing algorithms

#### Files to Create/Modify

**KĀDI Broker:**
- `C:\p4\Personal\SD\kadi-broker\src\services\ThreadManager.ts` (NEW)
  - Thread lifecycle management
  - Message history tracking

- `C:\p4\Personal\SD\kadi-broker\src\services\AgentRouter.ts` (MODIFY)
  - Add thread-aware routing
  - Integrate ThreadManager

**Game Engine:**
- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\AgentCommunication.js` (MODIFY)
  - Add thread management API
  - Support multi-turn conversations

#### Acceptance Criteria
- ❌ Conversation threads preserve message ordering
- ❌ Multiple threads can exist simultaneously without interference
- ❌ Thread state transitions work correctly (active → completed)
- ❌ Role-based routing directs messages to correct specialized agents
- ❌ Fallback routing prevents message loss when agents disconnect

#### Risk Assessment
**Low Risk**: Built on top of M5-T1 foundation
- **Mitigation**: Use existing RabbitMQ message ordering guarantees
- **Contingency**: Simplify to single-threaded conversations initially

---

### M5-T5: Natural Language Game Design Processing

**Status**: Not Started
**Expected Hours**: 8h
**Priority**: HIGH
**Type**: Feature
**Dependencies**: M5-T2 (Agent Communication), M5-T3 (Message Threading)

#### Objective
Enable Claude to understand natural language game design requests, extract structured requirements, and create actionable game specifications that can be used by other agents.

#### Technical Approach

**NLP Pipeline:**
```
User Natural Language Input
  → Claude (via MCP)
    → Intent Recognition (game design vs. modification vs. query)
      → Entity Extraction (game type, mechanics, entities, rules)
        → Structured Design Document
          → Shared Context (GameDesignDoc)
```

**Intent Categories:**
1. **New Game Design**: "Create a platformer with jumping and enemies"
2. **Design Modification**: "Add power-ups to the game"
3. **Design Query**: "What entities are in the current game?"
4. **Implementation Request**: "Implement the player movement system"

**Extraction Patterns:**
- **Game Genre**: platformer, shooter, puzzle, RPG, racing, etc.
- **Core Mechanics**: jumping, shooting, collecting, avoiding, timing, etc.
- **Entities**: player, enemies, platforms, items, obstacles, etc.
- **Rules**: win conditions, lose conditions, scoring, progression, etc.

#### Implementation Steps

**Phase 1: Tool Creation for Claude (3h)**
1. Create `process_game_design` tool in KĀDI JavaScript library
2. Define input schema for natural language processing
3. Implement structured output format (GameDesignDoc)
4. Register tool with KĀDI broker

**Tool Schema:**
```javascript
{
  name: "process_game_design",
  description: "Process natural language game design request into structured specification",
  inputSchema: {
    type: "object",
    properties: {
      userInput: { type: "string" },
      intent: {
        type: "string",
        enum: ["new_game", "modify_game", "query_design", "implement"]
      },
      extractedData: {
        type: "object",
        properties: {
          genre: { type: "string" },
          mechanics: { type: "array", items: { type: "string" } },
          entities: { type: "array" },
          rules: { type: "array" }
        }
      }
    }
  }
}
```

**Phase 2: Processing Logic (3h)**
1. Implement `GameDesignProcessor.js` handler
2. Parse and validate extracted design data
3. Create or update GameDesignDoc in shared context
4. Generate confirmation message back to Claude
5. Handle edge cases and validation errors

**Phase 3: Integration with Agent Communication (2h)**
1. Connect processing to agent communication system
2. Store design documents in game state
3. Notify other agents when design changes
4. Test end-to-end: Claude input → structured design → agent notifications

#### Files to Create/Modify

**Game Engine JavaScript:**
- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\tools\GameDesignTools.js` (NEW)
  - `process_game_design` tool schema

- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\handlers\GameDesignProcessor.js` (NEW)
  - Natural language processing logic
  - Structured data extraction
  - Context population

- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\models\GameDesignDoc.js` (NEW)
  - GameDesignDoc class definition
  - Validation and serialization

**Integration:**
- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\KADIGameControl.js` (MODIFY)
  - Register GameDesignTools
  - Integrate GameDesignProcessor handler

#### Acceptance Criteria
- ❌ Claude can describe game in natural language
- ❌ System extracts genre, mechanics, entities, rules correctly
- ❌ Structured GameDesignDoc created and accessible to other agents
- ❌ Invalid/ambiguous input returns helpful error messages
- ❌ Design updates trigger notifications to subscribed agents
- ❌ Demo: "Create a platformer with player, enemies, and platforms" → structured design

#### Risk Assessment
**Low Risk**: Relies on Claude's built-in NLP capabilities
- **Mitigation**: Claude does heavy lifting; we just structure the output
- **Contingency**: Use predefined templates if extraction fails

---

### M5-T6: Workflow Decomposition Algorithms

**Status**: Not Started
**Expected Hours**: 4h
**Priority**: LOW
**Type**: Research
**Dependencies**: M5-T5 (Game Design Processing)

#### Objective
Research and implement algorithms to decompose high-level game design specifications into sequential, actionable tasks that can be distributed to specialized agents.

#### Technical Approach

**Decomposition Strategy:**
```
GameDesignDoc (High-Level)
  → Workflow Decomposition Algorithm
    → Task Dependency Graph
      → Ordered Task List
        → Agent Assignment
```

**Example Decomposition:**
```
Input: "Platformer with player, enemies, platforms"

Output Task List:
1. Create player entity definition → Architect Agent
2. Create enemy entity definition → Architect Agent
3. Create platform entity definition → Architect Agent
4. Implement player movement system → Implementation Agent
5. Implement enemy AI system → Implementation Agent
6. Implement collision detection → Implementation Agent
7. Create level layout → Level Designer Agent
8. Test gameplay integration → Testing Agent
```

**Algorithms to Evaluate:**
1. **Rule-Based Decomposition**: Predefined templates for each game genre
2. **Dependency Graph Analysis**: Topological sort of component dependencies
3. **LLM-Assisted Decomposition**: Claude suggests task breakdown
4. **Hybrid Approach**: Combine rules + LLM for flexibility

#### Implementation Steps

**Phase 1: Research and Design (1h)**
1. Survey task decomposition literature
2. Define decomposition rule templates
3. Create dependency graph data structure
4. Document decomposition algorithm

**Phase 2: Implementation (2h)**
1. Create `WorkflowDecomposer.js` class
2. Implement rule-based decomposition for common patterns
3. Add dependency graph builder
4. Create task assignment heuristics (which agent for which task)

**Phase 3: Testing and Refinement (1h)**
1. Test with various game design specifications
2. Validate dependency ordering is correct
3. Refine rules based on test results
4. Document decomposition patterns

#### Files to Create/Modify

**Game Engine JavaScript:**
- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\workflow\WorkflowDecomposer.js` (NEW)
  - Decomposition algorithms
  - Dependency graph generation
  - Task assignment logic

- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\workflow\TaskTemplate.js` (NEW)
  - Predefined task templates for common patterns
  - Genre-specific decomposition rules

**Documentation:**
- `C:\p4\Personal\SD\ProtogameJS3D\Docs\workflow-decomposition.md` (NEW)
  - Algorithm documentation
  - Decomposition patterns
  - Examples and case studies

#### Acceptance Criteria
- ❌ Algorithm decomposes simple platformer design into 5-10 tasks
- ❌ Task dependencies are correctly ordered (no circular dependencies)
- ❌ Tasks assigned to appropriate specialized agents
- ❌ Complex designs (20+ components) handled without errors
- ❌ Decomposition time < 1 second for typical designs

#### Risk Assessment
**Low Risk**: Research task with flexible scope
- **Mitigation**: Start with simple rule-based approach
- **Contingency**: Manual task definition if automation insufficient

---

### M5-T7: Context Preservation

**Status**: Not Started
**Expected Hours**: 10h
**Priority**: HIGH
**Type**: Feature
**Dependencies**: M5-T3 (Message Routing and Threading)

#### Objective
Implement persistent storage and retrieval mechanisms ensuring agent context, conversation history, and design artifacts survive across sessions and system restarts.

#### Technical Approach

**Persistence Architecture:**
```
Shared Context (In-Memory)
  → Serialization Layer
    → Storage Backend (File System / Redis / Database)
      → Deserialization Layer
        → Restored Context (On Restart)
```

**What to Persist:**
1. **Conversation Threads**: Full message history with threading
2. **Shared Contexts**: All GameDesignDocs and artifacts
3. **Agent State**: Each agent's current state and tasks
4. **Decision Logs**: What decisions were made and why
5. **Generated Artifacts**: Code files, asset references, configurations

**Storage Options:**
1. **File System**: JSON files for human readability (development)
2. **Redis**: In-memory with persistence for production speed
3. **SQLite**: Lightweight relational database for complex queries
4. **Hybrid**: Redis for active contexts + File System for archives

#### Implementation Steps

**Phase 1: Serialization Layer (3h)**
1. Create serialization utilities for all context types
2. Implement JSON schema validation
3. Add compression for large contexts
4. Create versioned format for backward compatibility

**Phase 2: Storage Backend (4h)**
1. Implement File System storage adapter
2. Create Redis storage adapter (optional)
3. Add automatic backup/snapshot system
4. Implement incremental saves (only save changes)

**Phase 3: Restoration and Recovery (2h)**
1. Implement context restoration on startup
2. Add migration logic for format upgrades
3. Create recovery mechanisms for corrupted data
4. Test crash recovery scenarios

**Phase 4: Integration and Testing (1h)**
1. Integrate persistence into ContextManager
2. Add auto-save triggers (time-based + event-based)
3. Test restoration after system restart
4. Performance testing for large contexts

#### Files to Create/Modify

**KĀDI Broker:**
- `C:\p4\Personal\SD\kadi-broker\src\persistence\ContextSerializer.ts` (NEW)
  - Serialization/deserialization logic
  - Schema validation

- `C:\p4\Personal\SD\kadi-broker\src\persistence\StorageAdapter.ts` (NEW)
  - Abstract storage interface
  - File System implementation
  - Redis implementation (optional)

- `C:\p4\Personal\SD\kadi-broker\src\services\ContextManager.ts` (MODIFY)
  - Add persistence integration
  - Auto-save triggers
  - Restoration logic

**Storage Locations:**
- `C:\p4\Personal\SD\kadi-broker\data\contexts\*.json` (NEW)
  - Persisted context files
- `C:\p4\Personal\SD\kadi-broker\data\threads\*.json` (NEW)
  - Persisted conversation threads

#### Acceptance Criteria
- ❌ Contexts automatically saved every 5 minutes or on significant changes
- ❌ System restarts without data loss
- ❌ Conversation history fully recoverable after crash
- ❌ Large contexts (1000+ messages) handled efficiently
- ❌ Storage format human-readable for debugging
- ❌ Performance: Save operation < 100ms, Load < 200ms

#### Risk Assessment
**Medium Risk**: Data integrity critical for multi-session workflows
- **Mitigation**: Use proven serialization libraries (JSON Schema)
- **Contingency**: Start with simple file-based storage, add Redis later

---

### M5-T8: Coordination Patterns - Pipeline/Parallel/Iterative

**Status**: Not Started
**Expected Hours**: 15h
**Priority**: HIGH
**Type**: Feature
**Dependencies**: M5-T2 (Agent Communication), M5-T3 (Message Routing), M5-T6 (Workflow Decomposition)

#### Objective
Implement three fundamental multi-agent coordination patterns enabling complex game development workflows: pipeline (sequential), parallel (concurrent), and iterative (refinement).

#### Technical Approach

**Pattern 1: Pipeline (Sequential Workflow)**
```
User Request → Designer Agent → Architect Agent → Implementation Agent → Testing Agent → Done

Example: Game Design Workflow
1. Designer Agent: Creates game design specification
2. Architect Agent: Designs code structure and APIs
3. Implementation Agent: Writes actual code
4. Testing Agent: Validates implementation
```

**Pattern 2: Parallel (Concurrent Execution)**
```
                    ┌→ Agent A (Task 1) ─┐
Coordinator Agent ──┼→ Agent B (Task 2) ─┤→ Aggregator Agent → Done
                    └→ Agent C (Task 3) ─┘

Example: Parallel Entity Creation
1. Coordinator: Distributes entity creation tasks
2. Agent A: Creates player entity (parallel)
3. Agent B: Creates enemy entity (parallel)
4. Agent C: Creates platform entity (parallel)
5. Aggregator: Combines all entities into game
```

**Pattern 3: Iterative (Refinement Loop)**
```
Initial Design → Implementation → Review → Refine → Implementation → Review → Accept

Example: Level Design Iteration
1. Level Designer: Creates initial level layout
2. Playtester Agent: Tests level difficulty
3. Level Designer: Refines based on feedback (loop until acceptable)
4. Accept final design
```

#### Implementation Steps

**Phase 1: Pipeline Pattern (5h)**
1. Create `PipelineCoordinator.js` orchestrator
2. Implement sequential task execution
3. Add error handling and rollback
4. Create pipeline configuration DSL
5. Test with multi-stage workflow

**Pipeline Configuration:**
```javascript
const gameDesignPipeline = {
  name: "game_design_workflow",
  stages: [
    { agent: "designer", task: "create_design_spec" },
    { agent: "architect", task: "design_code_structure" },
    { agent: "implementer", task: "write_code" },
    { agent: "tester", task: "validate_implementation" }
  ],
  sharedContext: "game_project_123"
};
```

**Phase 2: Parallel Pattern (5h)**
1. Create `ParallelCoordinator.js` orchestrator
2. Implement task distribution to multiple agents
3. Add result aggregation and synchronization
4. Handle partial failures (some tasks succeed, some fail)
5. Test with concurrent entity creation

**Parallel Configuration:**
```javascript
const parallelEntityCreation = {
  name: "parallel_entity_workflow",
  coordinator: "entity_coordinator",
  tasks: [
    { agent: "implementer_1", task: "create_player_entity" },
    { agent: "implementer_2", task: "create_enemy_entity" },
    { agent: "implementer_3", task: "create_platform_entity" }
  ],
  aggregator: "entity_aggregator",
  waitForAll: true,  // or false for partial completion
  sharedContext: "game_project_123"
};
```

**Phase 3: Iterative Pattern (5h)**
1. Create `IterativeCoordinator.js` orchestrator
2. Implement iteration loop with exit conditions
3. Add feedback collection and refinement
4. Create convergence detection (when to stop iterating)
5. Test with level design refinement

**Iterative Configuration:**
```javascript
const iterativeLevelDesign = {
  name: "iterative_level_workflow",
  initialAgent: "level_designer",
  reviewAgent: "playtester",
  maxIterations: 5,
  exitCondition: (feedback) => feedback.score >= 8,
  sharedContext: "level_design_123"
};
```

#### Files to Create/Modify

**Game Engine JavaScript:**
- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\coordination\PipelineCoordinator.js` (NEW)
  - Sequential workflow orchestration

- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\coordination\ParallelCoordinator.js` (NEW)
  - Concurrent task distribution and aggregation

- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\coordination\IterativeCoordinator.js` (NEW)
  - Refinement loop orchestration

- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\coordination\CoordinationFactory.js` (NEW)
  - Factory for creating coordinators
  - Configuration parsing

**KĀDI Broker:**
- `C:\p4\Personal\SD\kadi-broker\src\services\AgentRouter.ts` (MODIFY)
  - Add support for coordination patterns
  - Handle broadcast and selective routing

#### Acceptance Criteria
- ❌ Pipeline pattern executes stages sequentially with shared context
- ❌ Parallel pattern distributes tasks and aggregates results correctly
- ❌ Iterative pattern loops with feedback until exit condition met
- ❌ Error handling prevents pipeline/parallel failures from hanging
- ❌ All patterns support cancellation and timeout
- ❌ Demo: Create game using pipeline (design → architect → implement)

#### Risk Assessment
**High Risk**: Complex orchestration with multiple failure modes
- **Mitigation**: Start with simple implementations, add robustness iteratively
- **Contingency**: Implement pipeline first (simplest), defer parallel/iterative if needed

---

### M5-T9: Learning Mechanisms and Memory

**Status**: Not Started
**Expected Hours**: 6h
**Priority**: LOW
**Type**: Research
**Dependencies**: M5-T7 (Context Preservation)

#### Objective
Research and prototype lightweight learning mechanisms enabling agents to improve performance over time by remembering successful patterns, common errors, and user preferences.

#### Technical Approach

**Learning Categories:**
1. **Pattern Recognition**: Remember successful game design patterns
2. **Error Avoidance**: Remember common mistakes and how to avoid them
3. **User Preferences**: Remember user's preferred game styles, mechanics, entities
4. **Optimization**: Remember which coordination patterns work best for which tasks

**Memory Architecture:**
```typescript
interface AgentMemory {
  agentId: string;
  memories: Memory[];
}

interface Memory {
  memoryId: string;
  type: "pattern" | "error" | "preference" | "optimization";
  trigger: string;              // What situation triggers this memory
  content: any;                 // What to remember
  confidence: number;           // 0-1, how confident in this memory
  useCount: number;             // How many times used successfully
  createdAt: number;
  lastUsedAt: number;
}

// Example: Pattern Memory
{
  type: "pattern",
  trigger: "platformer + jumping",
  content: {
    suggestedEntities: ["player", "platform", "enemy", "item"],
    suggestedMechanics: ["jump", "move", "collect", "avoid"]
  },
  confidence: 0.9,
  useCount: 15
}
```

#### Implementation Steps

**Phase 1: Memory Storage (2h)**
1. Design memory data schema
2. Create `AgentMemory.js` class
3. Implement memory CRUD operations
4. Integrate with context persistence (M5-T6)

**Phase 2: Learning Algorithms (3h)**
1. Implement pattern extraction from successful workflows
2. Add error logging and error avoidance suggestions
3. Create user preference learning from repeated interactions
4. Implement confidence scoring and memory decay

**Phase 3: Memory Retrieval and Application (1h)**
1. Create memory query API (search by trigger/type)
2. Implement memory-based suggestions to agents
3. Test memory improves agent performance over time
4. Document learning mechanisms

#### Files to Create/Modify

**Game Engine JavaScript:**
- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\memory\AgentMemory.js` (NEW)
  - Memory storage and retrieval
  - Learning algorithms

- `C:\p4\Personal\SD\ProtogameJS3D\Run\Data\Scripts\kadi\memory\MemoryQuery.js` (NEW)
  - Memory search and ranking
  - Confidence-based filtering

**KĀDI Broker:**
- `C:\p4\Personal\SD\kadi-broker\src\persistence\StorageAdapter.ts` (MODIFY)
  - Add memory persistence
  - Efficient memory indexing

#### Acceptance Criteria
- ❌ Agent remembers successful patterns from previous sessions
- ❌ Agent suggests avoiding common errors based on memory
- ❌ User preferences learned after 3+ interactions
- ❌ Memory confidence increases with successful reuse
- ❌ Memory retrieval < 10ms for typical queries
- ❌ Demo: Agent suggests entities based on previous platformer designs

#### Risk Assessment
**Low Risk**: Research task with optional implementation
- **Mitigation**: Start with simple key-value memory, add sophistication later
- **Contingency**: Defer to M6 if time limited

---

### M5-T10: Architecture Analysis PowerPoint Presentation

**Status**: Added
**Expected Hours**: 6h
**Priority**: HIGH (Due: November 5, 2025)
**Type**: Documentation / Presentation
**Dependencies**: None (can run in parallel with other M5 tasks)

#### Objective
Create a comprehensive architecture analysis presentation for thesis defense preparation, demonstrating the dual-language C++/JavaScript architecture of ProtogameJS3D and DaemonEngine. This 10-15 minute presentation will explain the system's design to the thesis professor, with emphasis on the M4-T8 async architecture refactoring achievement.

#### Technical Approach

**Presentation Structure** (10-15 minutes, 11 slides):
1. **Dual-Language Architecture Overview** - High-level C++ + V8 JavaScript integration model
2. **Execution Flow and Main Loop** - Runtime sequence from C++ to JavaScript
3. **Async Architecture Refactor (M4-T8)** - Major achievement: Entity/Camera systems moved to Engine
4. **Script Interface Layer** - Type-safe C++/JavaScript communication bridge
5. **Data Flow Example** - JavaScript → C++ → Rendering pipeline
6. **Hot-Reload System** - Development workflow advantage

**Content Focus Areas**:
- **Current State Architecture**: Both high-level overview and detailed code examples
- **M4-T8 Refactoring Emphasis**: Before/after comparison, SOLID principles applied
- **Cross-Repository Design**: Engine vs. Game separation
- **Async State Management**: StateBuffer template, worker/renderer threads
- **Script Interface Pattern**: IScriptableObject implementation

#### Implementation Steps

**Phase 1: Content Preparation (2h)**
1. Extract key C++ code sections (App.cpp, EntityAPI, CameraAPI, Script Interfaces)
2. Extract key JavaScript code sections (JSEngine.js, JSGame.js, Prop.js)
3. Create 6 mermaid diagrams (architecture, sequence, flowcharts)
4. Select 8-10 code snippets with detailed analysis
5. Document M4-T8 before/after refactoring

**Phase 2: PowerPoint Creation (2h)**
1. Design 11-slide structure using user's template
2. Write comprehensive speaker notes (150-200 words per slide)
3. Embed diagram references and code snippet callouts
4. Ensure 10-15 minute presentation timing

**Phase 3: Supporting Documentation (1.5h)**
1. Create separate mermaid diagrams document
2. Create code analysis document with line-by-line explanations
3. Create full technical documentation version

**Phase 4: Review and Polish (30min)**
1. Practice presentation timing
2. Verify technical accuracy
3. Final formatting and polish

#### Deliverables

**Primary Deliverable**:
- **PowerPoint File** (.pptx) with embedded speaker notes, diagram references, code callouts

**Supporting Deliverables**:
- **Mermaid Diagrams Document** (.md) - 6 standalone architecture diagrams
- **Code Analysis Document** (.md) - 8-10 snippets with detailed line-by-line analysis
- **Full Documentation** (.md) - Comprehensive technical reference

**Diagram Types**:
- High-level architecture overview (C++ + V8 + Script Interface)
- Execution flow sequence diagram (BeginFrame → Update → Render)
- Async architecture components (Engine systems, StateBuffer)
- Script interface layer architecture (IScriptableObject pattern)
- Data flow sequence (JavaScript → EntityAPI → Rendering)
- Hot-reload system flowchart (FileWatcher → ScriptReloader → V8)

#### Files to Create/Modify

**Presentation Files**:
- `C:\p4\Personal\SD\ProtogameJS3D\Docs\presentations\M5-T10-Architecture-Presentation.pptx` (NEW)
  - 11 slides with comprehensive speaker notes
  - Professional academic style

**Documentation Files**:
- `C:\p4\Personal\SD\ProtogameJS3D\Docs\presentations\M5-T10-Architecture-Diagrams.md` (NEW)
  - 6 mermaid diagrams with explanatory text

- `C:\p4\Personal\SD\ProtogameJS3D\Docs\presentations\M5-T10-Code-Analysis.md` (NEW)
  - Selected code snippets with detailed analysis
  - M4-T8 refactoring before/after comparisons

- `C:\p4\Personal\SD\ProtogameJS3D\Docs\presentations\M5-T10-Full-Documentation.md` (NEW)
  - Complete reference document
  - All diagrams + code + extended technical details

#### Acceptance Criteria
- ✅ 11 slides covering 6 main architecture topics
- ✅ 10-15 minute presentation timing when delivered with speaker notes
- ✅ Professional academic presentation style appropriate for thesis defense
- ✅ All diagrams are high-quality mermaid syntax
- ✅ All code snippets are current and technically accurate
- ✅ M4-T8 async refactoring correctly explained with before/after comparison
- ✅ Execution flow sequence matches actual runtime behavior
- ✅ Clear explanation suitable for professor unfamiliar with project
- ✅ All deliverables ready by next Tuesday (presentation day)

#### Risk Assessment
**Medium Risk**: Time constraint (due next Tuesday, only 6h estimated)
- **Probability**: MEDIUM (depends on daily schedule)
- **Mitigation**: High priority task, can work in parallel with other M5 tasks
- **Contingency**: Focus on core slides first, polish supporting docs if time permits

**Low Risk**: Well-defined scope and documented architecture
- **Mitigation**: All necessary architecture files and documentation already exist
- **Contingency**: N/A - straightforward documentation task

#### Success Metrics
- **Presentation Metrics**: 11 slides, 150-200 words speaker notes per slide
- **Timing Metrics**: 10-15 minutes when delivered
- **Diagram Metrics**: 6 professional mermaid diagrams
- **Code Metrics**: 8-10 examples with detailed analysis
- **Quality Metrics**: 100% technical accuracy, academic-grade style
- **Delivery Metrics**: All deliverables complete by next Tuesday

---

## Integration Architecture

### Multi-Agent System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     Claude Desktop (MCP Client)                  │
│  - Natural language game design                                  │
│  - High-level decision making                                    │
│  - User interaction                                              │
└────────────────┬────────────────────────────────────────────────┘
                 │ MCP Protocol
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                        KĀDI Broker                               │
│  - Agent-to-agent routing (M5-T1)                               │
│  - Message threading (M5-T2)                                     │
│  - Shared context management (M5-T3)                            │
│  - Context persistence (M5-T6)                                   │
│  - Coordination orchestration (M5-T7)                           │
└────┬────────────────┬────────────────┬─────────────────────────┘
     │ KĀDI Protocol  │ KĀDI Protocol  │ KĀDI Protocol
     ▼                ▼                ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│ Game Engine │ │Future Agent │ │Future Agent │
│             │ │   (M6)      │ │   (M6)      │
│ - Game      │ │ - Code      │ │ - Level     │
│   design    │ │   architect │ │   designer  │
│   processing│ │ - Code      │ │ - Asset     │
│   (M5-T4)   │ │   implement │ │   coord     │
│ - Workflow  │ └─────────────┘ └─────────────┘
│   decomp    │
│   (M5-T5)   │
│ - Memory    │
│   (M5-T8)   │
└─────────────┘
```

### Workflow Example: Conversational Game Design

```
1. User → Claude: "Create a simple platformer game"
2. Claude → KĀDI Broker → Game Engine: process_game_design(userInput)
3. Game Engine: Creates GameDesignDoc in SharedContext
4. Game Engine → KĀDI Broker: Notify "design_created"
5. KĀDI Broker → Workflow Decomposer: Decompose design into tasks
6. Workflow Decomposer: Creates task list in SharedContext
7. Pipeline Coordinator: Initiates sequential workflow
8. Each specialized agent executes assigned task
9. Results aggregated in SharedContext
10. Claude → User: "Game created successfully!"
```

---

## Critical Dependencies

### Prerequisites from M4
1. **M4-T5 (MCP Server)** - ⚠️ CRITICAL BLOCKER
   - Required for Claude Desktop connection
   - Blocks entire M5 milestone
   - Must complete by Oct 27, 2025

2. **M4-T6 (KĀDI JavaScript Library)** - ✅ COMPLETE
   - Provides tool infrastructure
   - Game control tools available

3. **M4-T7 (Basic AI Agent Integration)** - ✅ COMPLETE
   - Tool schemas and handlers ready
   - Agent communication groundwork laid

### External Systems Required
1. **KĀDI Broker** - Running at ws://kadi.build:8080
2. **RabbitMQ** - Message queue backend
3. **Claude Desktop** - MCP client for user interaction
4. **Storage** - File system or Redis for persistence

### Task Dependencies Within M5
```
M5-T1 (Template System & Demo) - Independent (depends on M4-T5)
M5-T10 (PowerPoint Presentation) - Independent

M5-T2 (Agent Communication)
  → M5-T3 (Message Routing)
    → M5-T5 (Game Design Processing)
      → M5-T6 (Workflow Decomposition)
        → M5-T8 (Coordination Patterns)

M5-T3 (Message Routing)
  → M5-T7 (Context Preservation)
    → M5-T9 (Learning & Memory)
```

**Critical Path**: M5-T2 → M5-T3 → M5-T5 → M5-T6 → M5-T8 (45h on critical path)
**Independent Paths**: M5-T1 (8h), M5-T10 (6h) - can run in parallel with critical path

---

## Risk Assessment

### Timeline Risks

**CRITICAL: M4-T5 Delay**
- **Impact**: If M4-T5 not complete by Oct 27, M5 cannot start
- **Probability**: HIGH (currently postponed)
- **Mitigation**: M4-T5 must be absolute priority on Oct 27
- **Contingency**: Compress M5 scope if delayed start

**MEDIUM: M5 Scope Manageable But Tight**
- **Impact**: 75h of work in 2 weeks = 37.5h/week with dual tasks active
- **Probability**: MEDIUM
- **Mitigation**: Prioritize critical path (M5-T2, T3, T5, T6, T8) and independent tasks (M5-T1, T10)
- **Contingency**: Defer M5-T6 (research) and M5-T9 (learning) to M6 if needed

**MEDIUM: Integration Complexity**
- **Impact**: Multi-agent systems notoriously difficult to debug
- **Probability**: MEDIUM
- **Mitigation**: Build incrementally, test each component
- **Contingency**: Simplify coordination patterns if needed

### Technical Risks

**MEDIUM: Message Routing Reliability**
- **Risk**: Message loss or ordering issues with RabbitMQ
- **Mitigation**: Use RabbitMQ message acknowledgments
- **Contingency**: Add message retry and deduplication

**MEDIUM: Context Conflict Resolution**
- **Risk**: Concurrent updates causing context corruption
- **Mitigation**: Optimistic locking with version numbers
- **Contingency**: Lock-based serialization if needed

**LOW: Storage Performance**
- **Risk**: Large contexts slow down persistence
- **Mitigation**: Incremental saves, compression
- **Contingency**: Upgrade to Redis if file system too slow

---

## Success Metrics

### Technical Metrics
- **Agent Communication Latency**: Target < 50ms (M5-T1)
- **Context Read Latency**: Target < 20ms (M5-T3)
- **Context Write Latency**: Target < 50ms (M5-T3)
- **Persistence Save Time**: Target < 100ms (M5-T6)
- **Persistence Load Time**: Target < 200ms (M5-T6)
- **Workflow Decomposition Time**: Target < 1s (M5-T5)

### Functional Metrics
- **Agent-to-Agent Messages**: 100% delivery rate
- **Context Conflict Rate**: < 1% of updates
- **Pipeline Success Rate**: > 95% complete without errors
- **Parallel Task Distribution**: Handle 10+ concurrent tasks
- **Iterative Convergence**: < 5 iterations for typical refinements

### Quality Metrics
- **Code Documentation**: All APIs have JSDoc
- **Error Handling**: All async operations have try/catch
- **Test Coverage**: 80%+ for critical path components
- **Recovery Mechanisms**: System survives agent disconnection

### Demonstration Metrics
- **Demo Completeness**: Full conversational game design workflow
- **User Experience**: Natural language → working game in < 5 minutes
- **Agent Coordination**: Visible pipeline/parallel coordination

---

## References and Documentation

### Internal Documentation
- [Project Root CLAUDE.md](../../../CLAUDE.md)
- [M4 Development Plan](../M4/development.md)
- [JavaScript Module CLAUDE.md](../../../Run/Data/Scripts/CLAUDE.md)
- [Task Pointer](../task-pointer.md)

### External Documentation
- [KĀDI Broker README](C:\p4\Personal\SD\kadi-broker\README.md)
- [MCP Protocol Specification](https://github.com/anthropics/mcp-specification)
- [RabbitMQ Documentation](https://www.rabbitmq.com/documentation.html)

### Research References
- Multi-Agent Systems: Wooldridge, "An Introduction to MultiAgent Systems"
- Workflow Patterns: van der Aalst, "Workflow Patterns"
- Context Management: Distributed Systems literature

### Thesis Resources
- Notion Thesis Plan: https://www.notion.so/Thesis-Proposal-Plan-262a1234359080c1bce0caf15245b6fc
- Milestone M5: (Add Notion link when available)

---

**Document Version**: 1.0 (Created Oct 26, 2025)
**Status**: M5 Not Started (Blocked by M4-T5)
**Next Review**: October 28, 2025 (M5 planned start date)
**Critical Dependency**: M4-T5 must complete before M5 can begin
