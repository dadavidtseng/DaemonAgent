# Multi-Language Agent Architecture for ProtogameJS3D

**Status**: Planning Complete - Ready for Implementation
**Created**: 2025-10-30
**M5 Task Mapping**: M5-T2 (Agent-to-Agent Communication Protocol)
**Total Effort**: 13 hours estimated

---

## Table of Contents

- [Overview](#overview)
- [KĀDI Protocol Architecture](#kādi-protocol-architecture)
- [Python Agent Implementation](#python-agent-implementation)
- [TypeScript Agent Implementation](#typescript-agent-implementation)
- [Multi-Language Framework](#multi-language-framework)
- [Integration with multi-agents-collaboration.md](#integration-with-multi-agents-collaborationmd)
- [Implementation Roadmap](#implementation-roadmap)
- [Success Criteria](#success-criteria)

---

## Overview

### Objective

Create a comprehensive multi-language agent system for ProtogameJS3D using the KĀDI protocol, enabling Python, TypeScript, and other language agents to collaborate on game development tasks following the workflow patterns defined in `multi-agents-collaboration.md`.

### Key Benefits

- **Language Flexibility**: Create agents in Python, TypeScript, Go, Rust, Java, C#, or any language with WebSocket support
- **Seamless Interoperability**: Agents communicate regardless of implementation language
- **Workflow Integration**: Maps directly to Planner Agent, UI-UX-Designer Agent patterns
- **Scalable Architecture**: Add new agent types without modifying existing infrastructure
- **Type Safety**: Native schema libraries (Zod, Pydantic) provide compile-time validation

### Architecture at a Glance

```
┌─────────────────────────────────────────────────────────────┐
│                  ProtogameJS3D Game Engine                   │
│               (JavaScript via V8 Runtime)                    │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ KĀDI Protocol (WebSocket)
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                      KĀDI Broker                             │
│  • Agent-to-agent routing (M5-T2)                          │
│  • Message threading (M5-T3)                               │
│  • Event pub/sub system                                     │
│  • RabbitMQ message queue                                   │
└──────┬──────────────┬──────────────┬───────────────────────┘
       │              │              │
       ▼              ▼              ▼
┌────────────┐  ┌────────────┐  ┌────────────┐
│  Python    │  │ TypeScript │  │  Future    │
│  Agents    │  │  Agents    │  │  Agents    │
│            │  │            │  │  (Go/Rust) │
│ • Planner  │  │ • UI-UX-   │  │            │
│ • Code Gen │  │   Designer │  │            │
│ • Tester   │  │ • Validator│  │            │
└────────────┘  └────────────┘  └────────────┘
```

---

## KĀDI Protocol Architecture

### Core Protocol Design

**KĀDI Protocol** is a distributed agent communication framework built on:

- **WebSocket Transport** - Bi-directional real-time communication
- **JSON-RPC 2.0 Message Format** - Standard request/response protocol
- **Ed25519 Authentication** - Cryptographic identity verification
- **RabbitMQ Messaging Backend** - Tool invocation routing and event distribution
- **Network Isolation** - Multi-tenant logical network separation

### Protocol Message Flow

```
1. SESSION_HELLO (handshake)
   ├─> Client sends: { role, name, version, networks }
   └─> Broker responds: { nonce, requiredSteps, heartbeatInterval }

2. SESSION_AUTHENTICATE (Ed25519 signature)
   ├─> Client sends: { publicKey, signature, nonce }
   └─> Broker responds: { agentId, mailbox }

3. AGENT_REGISTER (capability registration)
   ├─> Client sends: { tools[], networks[], displayName }
   └─> Broker responds: { status: 'registered' }

4. ABILITY_REQUEST (tool invocation)
   ├─> Client sends: { toolName, toolInput }
   ├─> Broker routes via RabbitMQ to provider
   ├─> Provider executes and returns result
   └─> Broker routes result back to client

5. EVENT_PUBLISH/SUBSCRIBE (pub/sub events)
   ├─> Pattern-based routing (e.g., "user.*")
   └─> RabbitMQ topic exchanges per network
```

### Key Protocol Features

| Feature | Description | Implementation |
|---------|-------------|----------------|
| **Authentication** | Ed25519 signature of nonce | Python: `cryptography` lib, TypeScript: `node:crypto` |
| **Tool Discovery** | Network-scoped tool registry | Broker maintains provider mappings |
| **Event System** | Pattern-based pub/sub | RabbitMQ topic exchanges with wildcard routing |
| **Network Isolation** | Logical multi-tenancy | Separate exchanges per network ID |
| **Session Persistence** | 5-hour TTL | Broker reconnection with same agent ID |
| **Heartbeat Monitoring** | 90-second grace period | Client must ping every 30s |

### Message Format Example

```json
{
  "jsonrpc": "2.0",
  "method": "kadi.ability.request",
  "params": {
    "toolName": "add",
    "toolInput": {
      "a": 5,
      "b": 3
    }
  },
  "id": 123
}
```

---

## Python Agent Implementation

### Module Organization

```
agents/python/calculator-agent/
├── agent.py                 # Main agent implementation
├── tools/
│   ├── __init__.py
│   ├── math_tools.py        # Tool implementations
│   └── schemas.py           # Pydantic schema definitions
├── pyproject.toml           # Dependencies
├── README.md                # Setup and usage
└── .env.example             # Configuration template
```

### Complete Python Agent Example

```python
"""
Simple KĀDI Agent in Python
============================

This agent demonstrates the complete KĀDI protocol implementation
for creating a calculator service.

Dependencies:
- kadi-core-py: KĀDI protocol client library
- pydantic: Schema validation and serialization
"""

import asyncio
from kadi import KadiClient
from pydantic import BaseModel, Field

# Step 1: Define Tool Schemas using Pydantic
class AddInput(BaseModel):
    """Input schema for addition operation."""
    a: float = Field(..., description="First number")
    b: float = Field(..., description="Second number")

class AddOutput(BaseModel):
    """Output schema for addition operation."""
    result: float = Field(..., description="Sum of a and b")

class MultiplyInput(BaseModel):
    """Input schema for multiplication operation."""
    a: float = Field(..., description="First number")
    b: float = Field(..., description="Second number")

class MultiplyOutput(BaseModel):
    """Output schema for multiplication operation."""
    result: float = Field(..., description="Product of a and b")


async def main():
    # Step 2: Create KĀDI Client
    client = KadiClient({
        'name': 'calculator',
        'version': '1.0.0',
        'role': 'agent',
        'broker': 'ws://localhost:8765',  # Python broker default port
        'networks': ['global', 'math']
    })

    # Step 3: Register Tools Using Decorator
    @client.tool(description="Add two numbers")
    async def add(params: AddInput) -> AddOutput:
        """
        Add two numbers and publish calculation event.

        Args:
            params: AddInput with a and b fields

        Returns:
            AddOutput with result field
        """
        result = params.a + params.b

        # Publish event when calculation completes
        await client.publish_event('math.calculation', {
            'operation': 'add',
            'operands': [params.a, params.b],
            'result': result
        })

        return AddOutput(result=result)

    @client.tool(description="Multiply two numbers")
    async def multiply(params: MultiplyInput) -> MultiplyOutput:
        """
        Multiply two numbers and publish calculation event.

        Args:
            params: MultiplyInput with a and b fields

        Returns:
            MultiplyOutput with result field
        """
        result = params.a * params.b

        await client.publish_event('math.calculation', {
            'operation': 'multiply',
            'operands': [params.a, params.b],
            'result': result
        })

        return MultiplyOutput(result=result)

    # Step 4: Subscribe to Events
    def on_calculation(event_data):
        """Handle calculation events from any agent."""
        print(f"📊 Calculation performed: {event_data}")

    client.subscribe_to_event('math.*', on_calculation)

    # Step 5: Connect and Serve
    print("🚀 Starting calculator agent...")

    try:
        agent_id = await client.connect()
        print(f"✅ Connected with agent ID: {agent_id}")

        # Serve indefinitely (blocks until interrupted)
        await client.serve('broker')

    except Exception as e:
        print(f"❌ Agent failed: {e}")
        raise


if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n👋 Shutting down calculator agent...")
```

### Python Dependencies (pyproject.toml)

```toml
[project]
name = "calculator-agent"
version = "1.0.0"
description = "KĀDI calculator agent example"
dependencies = [
    "kadi-core-py @ git+https://gitlab.com/humin-game-lab/kadi/kadi-core-py.git",
    "pydantic>=2.0.0",
    "websockets>=12.0"
]

[build-system]
requires = ["setuptools>=68.0"]
build-backend = "setuptools.build_meta"

[tool.pytest.ini_options]
asyncio_mode = "auto"
```

### Python Agent Key Features

- ✅ **Pydantic Schema Integration** - Automatic JSON Schema generation from Python type hints
- ✅ **Async-first Architecture** - Built on `asyncio` for non-blocking I/O
- ✅ **Decorator Pattern** - `@client.tool()` decorator for clean tool registration
- ✅ **Type Safety** - Full type hints with runtime validation via Pydantic
- ✅ **Event-Driven** - Pub/sub system for agent coordination

---

## TypeScript Agent Implementation

### Module Organization

```
agents/typescript/calculator-agent/
├── src/
│   ├── index.ts             # Main agent implementation
│   ├── tools/
│   │   ├── math.ts          # Tool implementations
│   │   └── schemas.ts       # Zod schema definitions
│   └── types/
│       └── index.ts         # TypeScript type definitions
├── package.json             # Dependencies
├── tsconfig.json            # TypeScript configuration
├── README.md                # Setup and usage
└── .env.example             # Configuration template
```

### Complete TypeScript Agent Example

```typescript
/**
 * Simple KĀDI Agent in TypeScript
 * ================================
 *
 * This agent demonstrates the complete KĀDI protocol implementation
 * for creating a calculator service.
 *
 * Dependencies:
 * - @kadi.build/core: KĀDI protocol client library
 * - zod: Schema validation and type inference
 */

import { KadiClient, z } from '@kadi.build/core';

// Step 1: Define Tool Schemas using Zod
const addInputSchema = z.object({
  a: z.number().describe('First number'),
  b: z.number().describe('Second number')
});

const addOutputSchema = z.object({
  result: z.number().describe('Sum of a and b')
});

const multiplyInputSchema = z.object({
  a: z.number().describe('First number'),
  b: z.number().describe('Second number')
});

const multiplyOutputSchema = z.object({
  result: z.number().describe('Product of a and b')
});

// Step 2: Create KĀDI Client
const client = new KadiClient({
  name: 'calculator',
  version: '1.0.0',
  role: 'agent',
  broker: 'ws://localhost:8080',  // TypeScript broker default port
  networks: ['global', 'math']
});

// Step 3: Register Tools
client.registerTool({
  name: 'add',
  description: 'Add two numbers',
  input: addInputSchema,
  output: addOutputSchema
}, async (params: z.infer<typeof addInputSchema>) => {
  /**
   * Add two numbers and publish calculation event.
   */
  const result = params.a + params.b;

  // Publish event when calculation completes
  client.publishEvent('math.calculation', {
    operation: 'add',
    operands: [params.a, params.b],
    result
  });

  return { result };
});

client.registerTool({
  name: 'multiply',
  description: 'Multiply two numbers',
  input: multiplyInputSchema,
  output: multiplyOutputSchema
}, async (params: z.infer<typeof multiplyInputSchema>) => {
  /**
   * Multiply two numbers and publish calculation event.
   */
  const result = params.a * params.b;

  client.publishEvent('math.calculation', {
    operation: 'multiply',
    operands: [params.a, params.b],
    result
  });

  return { result };
});

// Step 4: Subscribe to Events
client.subscribeToEvent('math.*', (data) => {
  console.log('📊 Calculation performed:', data);
});

// Step 5: Connect and Serve
async function main() {
  console.log('🚀 Starting calculator agent...');

  try {
    const agentId = await client.serve('broker');
    console.log(`✅ Connected with agent ID: ${agentId}`);
  } catch (error) {
    console.error('❌ Agent failed:', error);
    process.exit(1);
  }
}

// Step 6: Run Agent
main();

// Graceful shutdown
process.on('SIGTERM', async () => {
  console.log('\n👋 Shutting down calculator agent...');
  await client.disconnect();
  process.exit(0);
});

export default client;
```

### TypeScript Dependencies (package.json)

```json
{
  "name": "calculator-agent",
  "version": "1.0.0",
  "type": "module",
  "description": "KĀDI calculator agent example",
  "main": "dist/index.js",
  "scripts": {
    "start": "tsx src/index.ts",
    "build": "tsc",
    "dev": "tsx watch src/index.ts"
  },
  "dependencies": {
    "@kadi.build/core": "^0.1.0",
    "zod": "^3.22.4"
  },
  "devDependencies": {
    "@types/node": "^20.0.0",
    "typescript": "^5.3.0",
    "tsx": "^4.7.0"
  }
}
```

### TypeScript Agent Key Features

- ✅ **Zod Schema Integration** - 77% less code than JSON Schema, full type inference
- ✅ **Type Safety** - Full TypeScript type safety from schemas to runtime
- ✅ **Modern ESM** - ES module support with top-level await
- ✅ **Developer Experience** - Hot-reload with `tsx watch`
- ✅ **Event-Driven** - Pub/sub system for agent coordination

---

## Multi-Language Framework

### Universal Implementation Checklist

For ANY language to create a KĀDI agent, implement these components:

#### 1. **WebSocket Connection Layer**

```
Required Features:
✓ Connect to broker URL (ws:// or wss://)
✓ Send JSON-RPC 2.0 messages
✓ Receive JSON-RPC 2.0 responses
✓ Handle connection lifecycle (connect, disconnect, error)
✓ Automatic reconnection with exponential backoff
```

#### 2. **Ed25519 Cryptography**

```
Required Operations:
✓ Generate Ed25519 keypair
✓ Serialize public key to SPKI DER format (base64)
✓ Sign message with private key
✓ Base64 encode signature
```

**Language Library Recommendations:**

| Language | WebSocket Library | Ed25519 Library | JSON Schema Library |
|----------|-------------------|-----------------|---------------------|
| **Go** | `gorilla/websocket` | `crypto/ed25519` | `jsonschema-gen` |
| **Rust** | `tokio-tungstenite` | `ed25519-dalek` | `schemars` |
| **Java** | `javax.websocket` | `BouncyCastle` | `jsonschema-generator` |
| **C#** | `System.Net.WebSockets` | `NSec` | `NJsonSchema` |
| **Ruby** | `faye-websocket` | `ed25519` gem | `json-schema` |
| **PHP** | `ratchet/pawl` | `sodium_crypto_sign_*` | `justinrainbow/json-schema` |

#### 3. **KĀDI Protocol Messages**

```typescript
// Message Constants (all languages must implement)
const KadiMessages = {
  SESSION_HELLO: 'kadi.session.hello',
  SESSION_AUTHENTICATE: 'kadi.session.authenticate',
  SESSION_HEARTBEAT: 'kadi.session.ping',
  AGENT_REGISTER: 'kadi.agent.register',
  ABILITY_REQUEST: 'kadi.ability.request',
  ABILITY_LIST: 'kadi.ability.list',
  EVENT_PUBLISH: 'kadi.event.publish',
  EVENT_SUBSCRIBE: 'kadi.event.subscribe',
  EVENT_UNSUBSCRIBE: 'kadi.event.unsubscribe'
};
```

#### 4. **Tool Registry with JSON Schema**

```
Required Features:
✓ Register tool with name, description, schemas
✓ Store tool handlers (functions)
✓ Validate input against JSON Schema (optional but recommended)
✓ Execute tool and return result
✓ Convert native schema format → JSON Schema
```

#### 5. **Event System (Optional but Recommended)**

```
Pattern-based pub/sub:
✓ Subscribe to event patterns ("user.*", "system.error")
✓ Publish events to channels
✓ Local event hub for client-side event handling
```

### Cross-Language Best Practices

```
DO:
✓ Use JSON-RPC 2.0 for all messages
✓ Use JSON Schema for tool definitions
✓ Use Ed25519 for authentication
✓ Use WebSocket for transport
✓ Use pattern-based event routing
✓ Use semantic versioning for agents

DON'T:
✗ Send language-specific objects over wire
✗ Use language-specific serialization formats
✗ Assume specific error handling mechanisms
✗ Hard-code broker URLs (use configuration)
✗ Skip schema validation (security risk)
```

---

## Integration with multi-agents-collaboration.md

### Workflow Type Mapping

Based on the `multi-agents-collaboration.md` workflow, here's how KĀDI agents integrate:

#### **Requirement Planning Type** → **Planner Agent**

```python
# Planner Agent - Python Implementation

from kadi import KadiClient
from pydantic import BaseModel
from typing import List, Dict, Any

class PlanRequest(BaseModel):
    """User requirement input."""
    task: str
    context: Dict[str, Any] = {}

class ExecutionStep(BaseModel):
    """Single step in execution plan."""
    agent: str
    tool: str
    input: Dict[str, Any]
    depends_on: List[int] = []

class ExecutionPlan(BaseModel):
    """Complete execution plan."""
    task_id: str
    steps: List[ExecutionStep]

class PlannerAgent:
    """
    Orchestrates multi-agent workflows by:
    - Breaking down complex tasks into subtasks
    - Assigning tasks to specialized agents
    - Tracking progress and dependencies
    """

    def __init__(self):
        self.client = KadiClient({
            'name': 'planner-agent',
            'role': 'agent',
            'broker': 'ws://localhost:8765',
            'networks': ['global', 'planning']
        })

    @client.tool(description="Create execution plan from user request")
    async def create_plan(self, params: PlanRequest) -> ExecutionPlan:
        """
        Analyze user request and create step-by-step execution plan.

        Workflow:
        1. Discover available agents in network
        2. Break down task into atomic steps
        3. Assign steps to appropriate agents
        4. Define dependencies between steps
        5. Return structured execution plan
        """
        # 1. Discover available agents
        agents = await self.client.broker_protocol.discover_agents(['global'])

        # 2. Analyze task and break down into steps
        steps = self.analyze_task(params.task, agents)

        # 3. Create execution plan
        plan = ExecutionPlan(
            task_id=self.generate_task_id(),
            steps=steps
        )

        # 4. Store plan in shared context
        await self.client.publish_event('planning.created', {
            'task_id': plan.task_id,
            'num_steps': len(plan.steps)
        })

        return plan

    @client.tool(description="Execute multi-agent workflow")
    async def execute_plan(self, params: Dict[str, Any]) -> Dict[str, Any]:
        """
        Execute workflow by coordinating multiple agents.

        Coordination Patterns:
        - Sequential: Steps run one after another
        - Parallel: Independent steps run concurrently
        - Iterative: Steps repeat with refinement
        """
        plan_id = params['plan_id']
        plan = await self.load_plan(plan_id)

        results = []

        # Execute each step (sequential for now, parallel in M5-T8)
        for step in plan.steps:
            # Load remote agent ability
            agent = await self.client.load(step.agent, 'broker')

            # Invoke tool on remote agent
            result = await agent.invoke_tool(step.tool, step.input)
            results.append(result)

            # Publish progress event
            await self.client.publish_event('workflow.progress', {
                'plan_id': plan_id,
                'step': step.agent,
                'status': 'completed'
            })

        return {'results': results}
```

#### **Frontend Task Processing** → **UI-UX-Designer Agent**

```typescript
// UI-UX-Designer Agent - TypeScript Implementation

import { KadiClient, z } from '@kadi.build/core';

const client = new KadiClient({
  name: 'ui-ux-designer',
  role: 'agent',
  broker: 'ws://localhost:8080',
  networks: ['global', 'design']
});

// Design component schema
const designComponentSchema = z.object({
  component: z.string().describe('Component name (e.g., "login-form")'),
  style: z.enum(['material', 'fluent', 'tailwind']).optional(),
  theme: z.enum(['light', 'dark']).optional(),
  requirements: z.array(z.string()).optional()
});

const designOutputSchema = z.object({
  design: z.object({
    layout: z.string(),
    colors: z.record(z.string()),
    typography: z.record(z.string()),
    components: z.array(z.object({
      name: z.string(),
      props: z.record(z.any())
    }))
  }),
  figma_url: z.string().optional(),
  assets: z.array(z.string())
});

client.registerTool({
  name: 'design_component',
  description: 'Design UI/UX component with best practices',
  input: designComponentSchema,
  output: designOutputSchema
}, async (params) => {
  console.log(`🎨 Designing component: ${params.component}`);

  // 1. Call AI design system (GPT-4, Claude, etc.)
  const design = await generateDesign(params.component, params.style);

  // 2. Export to Figma (optional)
  const figmaUrl = await exportToFigma(design);

  // 3. Generate design tokens and assets
  const assets = await generateDesignAssets(design);

  // 4. Publish design event for code-generator agent
  client.publishEvent('design.created', {
    component: params.component,
    design_id: design.id,
    ready_for_implementation: true
  });

  return {
    design,
    figma_url: figmaUrl,
    assets
  };
});

// Subscribe to planner requests
client.subscribeToEvent('planning.*', async (event) => {
  if (event.requires_design) {
    console.log('📐 Received design task from planner:', event);
  }
});

await client.serve('broker');
```

### Multi-Agent Collaboration Flow

```
User: "Create a login form with Material Design"
│
▼
┌──────────────────────────────────────────┐
│         Planner Agent (Python)            │
│  1. Classify as "Requirement Planning"   │
│  2. Break down into subtasks:            │
│     - Design login form UI               │
│     - Generate form component code       │
│     - Create form validation tests       │
│  3. Discover available agents            │
│  4. Create execution plan                │
└─────────────┬────────────────────────────┘
              │
              │ publish event: planning.created
              │
     ┌────────┴─────────┬─────────────┐
     │                  │             │
     ▼                  ▼             ▼
┌─────────────┐  ┌──────────────┐  ┌──────────────┐
│UI-UX-       │  │Code-Generator│  │Test-Generator│
│Designer     │  │Agent (TS)    │  │Agent (Py)    │
│Agent (TS)   │  │              │  │              │
├─────────────┤  ├──────────────┤  ├──────────────┤
│1. design_   │  │1. generate_  │  │1. create_    │
│  component()│→ │  code()      │→ │  tests()     │
│             │  │              │  │              │
│2. publish:  │  │2. publish:   │  │2. publish:   │
│  design.    │  │  code.       │  │  test.       │
│  created    │  │  generated   │  │  completed   │
└─────────────┘  └──────────────┘  └──────────────┘
     │                  │                │
     └──────────────────┴────────────────┘
                        │
                        │ All events aggregated
                        │
                        ▼
              ┌──────────────────┐
              │  Event Hub       │
              │  (RabbitMQ)      │
              │                  │
              │  Workflow Status:│
              │  ✓ Design        │
              │  ✓ Code          │
              │  ✓ Tests         │
              └──────────────────┘
```

---

## Implementation Roadmap

### Phase 1: Foundation (2h)

**Deliverables:**
- ✅ This planning document
- ✅ Agent directory structure
- ✅ Base configuration templates

**Tasks:**
1. Create `agents/` directory structure
2. Set up Python and TypeScript project scaffolding
3. Configure broker connection settings

### Phase 2: Python Agents (3h)

**Deliverables:**
- ✅ Python calculator agent (example)
- ✅ Python Planner agent (workflow orchestrator)

**Tasks:**
1. Implement Python calculator agent with Pydantic schemas
2. Test Ed25519 authentication flow
3. Implement Planner agent with task breakdown logic
4. Create agent discovery mechanism

### Phase 3: TypeScript Agents (3h)

**Deliverables:**
- ✅ TypeScript calculator agent (example)
- ✅ TypeScript UI-UX-Designer agent

**Tasks:**
1. Implement TypeScript calculator agent with Zod schemas
2. Test cross-language communication (Python ↔ TypeScript)
3. Implement UI-UX-Designer agent with design tools
4. Create event-driven coordination examples

### Phase 4: Multi-Language Guide (2h)

**Deliverables:**
- ✅ Implementation guide for other languages
- ✅ Agent registry catalog

**Tasks:**
1. Document Go, Rust, Java, C# implementation patterns
2. Create library recommendation matrix
3. Build agent registry with capabilities
4. Write deployment guide

### Phase 5: Integration Testing (2h)

**Deliverables:**
- ✅ Integration test suite
- ✅ Example workflows

**Tasks:**
1. Create Python ↔ TypeScript communication tests
2. Test event routing and pub/sub
3. Verify authentication and security
4. Document game creation workflow example

### Phase 6: Documentation (1h)

**Deliverables:**
- ✅ Updated project documentation
- ✅ Agent ecosystem overview

**Tasks:**
1. Update `CLAUDE.md` with multi-agent section
2. Create `Docs/multi-agent-system.md`
3. Write agent README files
4. Create deployment guide

---

## Success Criteria

### Technical Requirements

- ✅ Python agent can invoke TypeScript agent tools
- ✅ TypeScript agent can invoke Python agent tools
- ✅ Event-driven workflow coordination works across languages
- ✅ Ed25519 authentication verified and secure
- ✅ JSON Schema validation prevents invalid tool inputs
- ✅ Heartbeat monitoring prevents zombie agents

### Functional Requirements

- ✅ Planner agent successfully breaks down complex tasks
- ✅ UI-UX-Designer agent generates valid design specifications
- ✅ Agents discover each other dynamically via broker
- ✅ Event pub/sub enables loose coupling
- ✅ Multi-step workflows execute correctly

### Documentation Requirements

- ✅ Clear step-by-step guides for Python and TypeScript
- ✅ Language-agnostic implementation guide
- ✅ Integration with `multi-agents-collaboration.md` workflow
- ✅ Deployment and operations documentation
- ✅ Troubleshooting guide for common issues

### Performance Requirements

- ✅ Agent-to-agent latency < 50ms for local network
- ✅ Tool invocation overhead < 10ms
- ✅ Event delivery latency < 20ms
- ✅ Support 10+ concurrent agents without degradation

---

## Alignment with M5 Milestone

### Primary Mapping: M5-T2 (Agent-to-Agent Communication Protocol)

This multi-language agent work directly implements M5-T2:

- ✅ Standardized communication protocol (KĀDI)
- ✅ Multiple agents exchange messages
- ✅ Coordinate actions across language boundaries
- ✅ Share state through events
- ✅ KĀDI broker infrastructure

### Supporting Mappings

**M5-T5: Natural Language Game Design Processing**
- Planner Agent processes user natural language requests
- Task breakdown from conversational input
- Intent classification (Planning/Discussion/Execution)

**M5-T6: Workflow Decomposition Algorithms**
- Planner Agent implements task decomposition
- Dependency graph creation
- Agent assignment heuristics

**M5-T8: Coordination Patterns**
- Sequential workflow (pipeline pattern)
- Event-driven coordination
- Future: Parallel and iterative patterns

---

## Next Steps

1. **Create Python calculator agent** - Validate KĀDI protocol implementation
2. **Create TypeScript calculator agent** - Verify cross-language communication
3. **Test agent-to-agent invocation** - Python calls TypeScript tools
4. **Implement Planner agent** - Workflow orchestration
5. **Implement UI-UX-Designer agent** - Specialized design tasks
6. **Write integration tests** - Ensure reliability
7. **Document deployment** - Production readiness

---

**Document Version**: 1.0
**Status**: Planning Complete - Ready for Implementation
**Next Review**: After Phase 2 completion
**Contact**: [Project Team]

---

*Built with KĀDI protocol for seamless multi-language agent collaboration* 🚀
