//----------------------------------------------------------------------------------------------------
// HighLevelEntityAPI.cpp
// Phase 2: High-Level Entity/Camera/Light API Implementation
//----------------------------------------------------------------------------------------------------

#include "Game/Framework/HighLevelEntityAPI.hpp"

#include "Engine/Renderer/RenderCommandQueue.hpp"
#include "Engine/Script/ScriptSubsystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/LogSubsystem.hpp"

// Undefine Windows min/max macros before including V8 headers
// V8 uses std::min/std::max internally which conflict with Windows macros
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// V8 JavaScript engine headers
#include "v8.h"

//----------------------------------------------------------------------------------------------------
// Construction / Destruction
//----------------------------------------------------------------------------------------------------

HighLevelEntityAPI::HighLevelEntityAPI(RenderCommandQueue* commandQueue,
                                       ScriptSubsystem* v8Subsystem,
                                       Renderer* renderer)
	: m_commandQueue(commandQueue)
	, m_scriptSubsystem(v8Subsystem)
	, m_renderer(renderer)
	, m_nextEntityId(1)      // Start entity IDs at 1 (0 reserved for invalid)
	, m_nextCameraId(1000)   // Start camera IDs at 1000 (separate namespace)
	, m_nextLightId(10000)   // Start light IDs at 10000 (separate namespace)
	, m_nextCallbackId(1)
{
	GUARANTEE_OR_DIE(m_commandQueue != nullptr, "HighLevelEntityAPI: RenderCommandQueue is nullptr!");
	GUARANTEE_OR_DIE(m_scriptSubsystem != nullptr, "HighLevelEntityAPI: ScriptSubsystem is nullptr!");
	GUARANTEE_OR_DIE(m_renderer != nullptr, "HighLevelEntityAPI: Renderer is nullptr!");

	DebuggerPrintf("HighLevelEntityAPI: Initialized (Phase 2)\n");
}

//----------------------------------------------------------------------------------------------------
HighLevelEntityAPI::~HighLevelEntityAPI()
{
	// Log any pending callbacks that were never executed
	if (!m_pendingCallbacks.empty())
	{
		DebuggerPrintf("HighLevelEntityAPI: Warning - %zu pending callbacks not executed at shutdown\n",
		               m_pendingCallbacks.size());
	}
}

//----------------------------------------------------------------------------------------------------
// Entity API Implementation
//----------------------------------------------------------------------------------------------------

CallbackID HighLevelEntityAPI::CreateMesh(std::string const& meshType,
                                          Vec3 const& position,
                                          float scale,
                                          Rgba8 const& color,
                                          ScriptCallback const& callback)
{
	// Generate unique entity ID
	EntityID entityId = GenerateEntityID();

	// Generate unique callback ID
	CallbackID callbackId = GenerateCallbackID();

	DebuggerPrintf("[TRACE] HighLevelEntityAPI::CreateMesh - meshType=%s, entityId=%llu, callbackId=%llu, pos=(%.1f,%.1f,%.1f), scale=%.1f\n",
	               meshType.c_str(), entityId, callbackId, position.x, position.y, position.z, scale);

	// Store callback for later execution (after C++ processes command)
	PendingCallback pendingCallback;
	pendingCallback.callback = callback;
	pendingCallback.resultId = entityId;
	pendingCallback.ready = false;  // Will be set to true by NotifyCallbackReady()
	m_pendingCallbacks[callbackId] = pendingCallback;

	// Create mesh creation command
	MeshCreationData meshData;
	meshData.meshType = meshType;
	meshData.position = position;
	meshData.radius = scale;  // radius = uniform scale in Phase 2
	meshData.color = color;

	RenderCommand command(RenderCommandType::CREATE_MESH, entityId, meshData);

	// Submit command to queue
	bool submitted = SubmitCommand(command);
	if (!submitted)
	{
		// Queue full - log error and mark callback as ready with invalid ID
		DebuggerPrintf("HighLevelEntityAPI::CreateMesh - Queue full! Dropping mesh creation for entity %llu\n",
		               entityId);
		m_pendingCallbacks[callbackId].ready = true;
		m_pendingCallbacks[callbackId].resultId = 0;  // 0 = creation failed
	}
	else
	{
		// Success - mark callback as ready immediately (simplified Phase 2 approach)
		// In full async implementation, this would be set by command processor
		DebuggerPrintf("[TRACE] HighLevelEntityAPI::CreateMesh - Command submitted successfully to queue\n");
		m_pendingCallbacks[callbackId].ready = true;
	}

	return callbackId;
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::UpdatePosition(EntityID entityId, Vec3 const& position)
{
	// Create entity update command with position
	EntityUpdateData updateData;
	updateData.position = position;  // std::optional automatically constructed

	RenderCommand command(RenderCommandType::UPDATE_ENTITY, entityId, updateData);

	// Submit command
	bool submitted = SubmitCommand(command);
	if (!submitted)
	{
		DebuggerPrintf("HighLevelEntityAPI::UpdatePosition - Queue full! Dropping position update for entity %llu\n",
		               entityId);
	}
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::MoveBy(EntityID entityId, Vec3 const& delta)
{
	// For relative movement, we need to read current position from EntityStateBuffer
	// and calculate new absolute position.
	// For Phase 2 simplicity, we'll just submit the delta as-is and let the
	// command processor handle the addition.
	// TODO: This requires extending EntityUpdateData to support delta movement

	// PHASE 2 SIMPLIFICATION: Convert delta to absolute position update
	// This requires reading from EntityStateBuffer (not implemented yet)
	// For now, just log a warning
	DebuggerPrintf("HighLevelEntityAPI::MoveBy - Not fully implemented in Phase 2! Use UpdatePosition instead.\n");

	// Create update command with delta (will be interpreted as absolute position for now)
	EntityUpdateData updateData;
	updateData.position = delta;  // TEMPORARY: Treat as absolute until Phase 2b

	RenderCommand command(RenderCommandType::UPDATE_ENTITY, entityId, updateData);
	SubmitCommand(command);
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::UpdateOrientation(EntityID entityId, EulerAngles const& orientation)
{
	// Create entity update command with orientation
	EntityUpdateData updateData;
	updateData.orientation = orientation;

	RenderCommand command(RenderCommandType::UPDATE_ENTITY, entityId, updateData);

	bool submitted = SubmitCommand(command);
	if (!submitted)
	{
		DebuggerPrintf("HighLevelEntityAPI::UpdateOrientation - Queue full! Dropping orientation update for entity %llu\n",
		               entityId);
	}
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::UpdateColor(EntityID entityId, Rgba8 const& color)
{
	// Create entity update command with color
	EntityUpdateData updateData;
	updateData.color = color;

	RenderCommand command(RenderCommandType::UPDATE_ENTITY, entityId, updateData);

	bool submitted = SubmitCommand(command);
	if (!submitted)
	{
		DebuggerPrintf("HighLevelEntityAPI::UpdateColor - Queue full! Dropping color update for entity %llu\n",
		               entityId);
	}
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::DestroyEntity(EntityID entityId)
{
	// Create destroy command
	RenderCommand command(RenderCommandType::DESTROY_ENTITY, entityId, std::monostate{});

	bool submitted = SubmitCommand(command);
	if (!submitted)
	{
		DebuggerPrintf("HighLevelEntityAPI::DestroyEntity - Queue full! Dropping destroy for entity %llu\n",
		               entityId);
	}
}

//----------------------------------------------------------------------------------------------------
// Camera API Implementation
//----------------------------------------------------------------------------------------------------

CallbackID HighLevelEntityAPI::CreateCamera(Vec3 const& position,
                                            Vec3 const& lookAt,
                                            std::string const& type,
                                            ScriptCallback const& callback)
{
	// Generate unique camera ID
	EntityID cameraId = GenerateCameraID();

	// Generate unique callback ID
	CallbackID callbackId = GenerateCallbackID();

	// Store callback
	PendingCallback pendingCallback;
	pendingCallback.callback = callback;
	pendingCallback.resultId = cameraId;
	pendingCallback.ready = false;  // Will be set to true by NotifyCallbackReady()
	m_pendingCallbacks[callbackId] = pendingCallback;

	// Create camera creation command
	CameraCreationData cameraData;
	cameraData.position = position;
	cameraData.lookAt = lookAt;
	cameraData.type = type;

	RenderCommand command(RenderCommandType::CREATE_CAMERA, cameraId, cameraData);

	// Submit command to queue
	bool submitted = SubmitCommand(command);
	if (!submitted)
	{
		// Queue full - log error and mark callback as ready with invalid ID
		DebuggerPrintf("HighLevelEntityAPI::CreateCamera - Queue full! Dropping camera creation for camera %llu\n",
		               cameraId);
		m_pendingCallbacks[callbackId].ready = true;
		m_pendingCallbacks[callbackId].resultId = 0;  // 0 = creation failed
	}
	else
	{
		// Success - mark callback as ready immediately (simplified Phase 2 approach)
		m_pendingCallbacks[callbackId].ready = true;
	}

	return callbackId;
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::MoveCamera(EntityID cameraId, Vec3 const& position)
{
	// Create camera update command
	CameraUpdateData updateData;
	updateData.position = position;
	updateData.orientation = EulerAngles::ZERO;  // Keep existing orientation

	RenderCommand command(RenderCommandType::UPDATE_CAMERA, cameraId, updateData);

	bool submitted = SubmitCommand(command);
	if (!submitted)
	{
		DebuggerPrintf("HighLevelEntityAPI::MoveCamera - Queue full! Dropping camera move for camera %llu\n",
		               cameraId);
	}
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::MoveCameraBy(EntityID cameraId, Vec3 const& delta)
{
	// PHASE 2 SIMPLIFICATION: Delta movement not fully implemented
	DebuggerPrintf("HighLevelEntityAPI::MoveCameraBy - Not fully implemented in Phase 2! Use MoveCamera instead.\n");

	// Treat delta as absolute position temporarily
	CameraUpdateData updateData;
	updateData.position = delta;
	updateData.orientation = EulerAngles::ZERO;

	RenderCommand command(RenderCommandType::UPDATE_CAMERA, cameraId, updateData);
	SubmitCommand(command);
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::LookAtCamera(EntityID cameraId, Vec3 const& target)
{
	// PHASE 2: LookAt requires calculating orientation from current position to target
	// This is deferred to Phase 2b
	DebuggerPrintf("HighLevelEntityAPI::LookAtCamera - Not implemented in Phase 2!\n");
	DebuggerPrintf("  Camera %llu should look at (%.2f, %.2f, %.2f)\n",
	               cameraId, target.x, target.y, target.z);

	// TODO: Calculate orientation from camera position to target
	// TODO: Submit UPDATE_CAMERA command with calculated orientation
}

//----------------------------------------------------------------------------------------------------
// Light API Implementation (Phase 2b - Deferred)
//----------------------------------------------------------------------------------------------------

CallbackID HighLevelEntityAPI::CreateLight(Vec3 const& position,
                                           Rgba8 const& color,
                                           float intensity,
                                           ScriptCallback const& callback)
{
	DebuggerPrintf("HighLevelEntityAPI::CreateLight - Light API deferred to Phase 2b\n");
	return 0;  // Invalid callback ID
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::UpdateLight(EntityID lightId, Vec3 const& position, Rgba8 const& color, float intensity)
{
	DebuggerPrintf("HighLevelEntityAPI::UpdateLight - Light API deferred to Phase 2b\n");
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::DestroyLight(EntityID lightId)
{
	DebuggerPrintf("HighLevelEntityAPI::DestroyLight - Light API deferred to Phase 2b\n");
}

//----------------------------------------------------------------------------------------------------
// Callback Execution
//----------------------------------------------------------------------------------------------------

void HighLevelEntityAPI::ExecutePendingCallbacks()
{
	// Execute all ready callbacks
	// Note: This is called on JavaScript worker thread, so V8 locking is required

	// Log when we have pending callbacks (diagnostic)
	if (!m_pendingCallbacks.empty())
	{
		size_t readyCount = 0;
		for (auto const& pair : m_pendingCallbacks)
		{
			if (pair.second.ready) readyCount++;
		}

		if (readyCount > 0)
		{
			DAEMON_LOG(LogScript, eLogVerbosity::Log,
				Stringf("HighLevelEntityAPI::ExecutePendingCallbacks - Processing {} ready callbacks (out of {} total)",
					readyCount, m_pendingCallbacks.size()));
		}
	}

	for (auto it = m_pendingCallbacks.begin(); it != m_pendingCallbacks.end(); )
	{
		CallbackID callbackId = it->first;
		PendingCallback& pending = it->second;

		if (pending.ready)
		{
			// Execute callback with result ID
			ExecuteCallback(callbackId, pending.resultId);

			// Remove from pending map
			it = m_pendingCallbacks.erase(it);
		}
		else
		{
			++it;
		}
	}
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::NotifyCallbackReady(CallbackID callbackId, EntityID resultId)
{
	// Find callback in pending map
	auto it = m_pendingCallbacks.find(callbackId);
	if (it != m_pendingCallbacks.end())
	{
		// Mark as ready and update result ID
		it->second.ready = true;
		it->second.resultId = resultId;
	}
	else
	{
		DebuggerPrintf("HighLevelEntityAPI::NotifyCallbackReady - Callback %llu not found!\n", callbackId);
	}
}

//----------------------------------------------------------------------------------------------------
// Helper Methods
//----------------------------------------------------------------------------------------------------

EntityID HighLevelEntityAPI::GenerateEntityID()
{
	// Simple auto-increment (thread-safe in single-threaded JavaScript worker)
	return m_nextEntityId++;
}

//----------------------------------------------------------------------------------------------------
EntityID HighLevelEntityAPI::GenerateCameraID()
{
	return m_nextCameraId++;
}

//----------------------------------------------------------------------------------------------------
EntityID HighLevelEntityAPI::GenerateLightID()
{
	return m_nextLightId++;
}

//----------------------------------------------------------------------------------------------------
CallbackID HighLevelEntityAPI::GenerateCallbackID()
{
	return m_nextCallbackId++;
}

//----------------------------------------------------------------------------------------------------
bool HighLevelEntityAPI::SubmitCommand(RenderCommand const& command)
{
	// Submit command to queue
	bool success = m_commandQueue->Submit(command);

	if (!success)
	{
		// Queue full - backpressure triggered
		DebuggerPrintf("HighLevelEntityAPI: RenderCommandQueue FULL! Command dropped.\n");
	}

	return success;
}

//----------------------------------------------------------------------------------------------------
void HighLevelEntityAPI::ExecuteCallback(CallbackID callbackId, EntityID resultId)
{
	// Find callback
	auto it = m_pendingCallbacks.find(callbackId);
	if (it == m_pendingCallbacks.end())
	{
		DAEMON_LOG(LogScript, eLogVerbosity::Warning,
			Stringf("HighLevelEntityAPI::ExecuteCallback - Callback {} not found!", callbackId));
		return;
	}

	ScriptCallback const& callback = it->second.callback;

	// Phase 2b: Execute JavaScript callback with V8 locking
	DAEMON_LOG(LogScript, eLogVerbosity::Log,
		Stringf("HighLevelEntityAPI::ExecuteCallback - Executing callback {} with resultId {}",
			callbackId, resultId));

	// Get V8 isolate from ScriptSubsystem
	v8::Isolate* isolate = m_scriptSubsystem->GetIsolate();
	if (!isolate)
	{
		DAEMON_LOG(LogScript, eLogVerbosity::Error,
			"HighLevelEntityAPI::ExecuteCallback - V8 isolate is null!");
		return;
	}

	// Execute callback with V8 locking (CRITICAL for thread safety)
	{
		v8::Locker locker(isolate);                      // Acquire V8 lock
		v8::Isolate::Scope isolate_scope(isolate);       // Enter isolate
		v8::HandleScope handle_scope(isolate);           // Manage heap handles
		v8::TryCatch try_catch(isolate);                 // Catch exceptions

		try
		{
			// Extract v8::Function from std::any
			// ScriptSubsystem stores JavaScript functions as v8::Global<v8::Function>* (persistent handle pointer)
			// (See ScriptSubsystem.cpp:2076)

			// Extract the Global pointer from std::any
			v8::Global<v8::Function>* globalFuncPtr = nullptr;

			try
			{
				globalFuncPtr = std::any_cast<v8::Global<v8::Function>*>(callback);
			}
			catch (std::bad_any_cast const&)
			{
				DAEMON_LOG(LogScript, eLogVerbosity::Error,
					"HighLevelEntityAPI::ExecuteCallback - Failed to extract callback as v8::Global<v8::Function>*");
				return;
			}

			// Validate pointer is not null
			if (!globalFuncPtr)
			{
				DAEMON_LOG(LogScript, eLogVerbosity::Error,
					"HighLevelEntityAPI::ExecuteCallback - Callback pointer is null");
				return;
			}

			// Convert Global to Local for execution
			v8::Local<v8::Function> callbackFunc = globalFuncPtr->Get(isolate);

			// Validate function is not empty
			if (callbackFunc.IsEmpty())
			{
				DAEMON_LOG(LogScript, eLogVerbosity::Error,
					"HighLevelEntityAPI::ExecuteCallback - Callback function is empty");
				return;
			}

			// Get V8 context from ScriptSubsystem
			// CRITICAL: Must manually enter context since we're not in JavaScript execution scope
			void* contextPtr = m_scriptSubsystem->GetV8Context();
			if (!contextPtr)
			{
				DAEMON_LOG(LogScript, eLogVerbosity::Error,
					"HighLevelEntityAPI::ExecuteCallback - Failed to get V8 context from ScriptSubsystem");
				return;
			}

			// Cast void* to v8::Local<v8::Context>* (GetV8Context returns pointer to thread_local)
			v8::Local<v8::Context>* contextLocalPtr = static_cast<v8::Local<v8::Context>*>(contextPtr);
			v8::Local<v8::Context> context = *contextLocalPtr;

			// Validate context is not empty
			if (context.IsEmpty())
			{
				DAEMON_LOG(LogScript, eLogVerbosity::Error,
					"HighLevelEntityAPI::ExecuteCallback - V8 context is empty after retrieval!");
				return;
			}

			// Enter the V8 context scope
			v8::Context::Scope context_scope(context);

			// Create JavaScript number from entityId (V8 uses doubles for all numbers)
			v8::Local<v8::Number> resultIdValue = v8::Number::New(isolate, static_cast<double>(resultId));

			// Prepare arguments array
			v8::Local<v8::Value> argv[1] = { resultIdValue };

			// Call JavaScript function: callback(resultId)
			v8::MaybeLocal<v8::Value> result = callbackFunc->Call(context, v8::Undefined(isolate), 1, argv);

			// Check for JavaScript exceptions
			if (try_catch.HasCaught())
			{
				v8::Local<v8::Message> message = try_catch.Message();
				v8::String::Utf8Value error(isolate, try_catch.Exception());
				DAEMON_LOG(LogScript, eLogVerbosity::Error,
					Stringf("HighLevelEntityAPI::ExecuteCallback - JavaScript callback error: {}", *error));

				// C++ rendering continues even if JavaScript throws
				return;
			}

			// Check if Call() returned empty (should not happen if try_catch succeeded)
			if (result.IsEmpty())
			{
				DAEMON_LOG(LogScript, eLogVerbosity::Warning,
					"HighLevelEntityAPI::ExecuteCallback - Callback returned empty result");
			}

			DAEMON_LOG(LogScript, eLogVerbosity::Log,
				Stringf("HighLevelEntityAPI::ExecuteCallback - Callback {} executed successfully", callbackId));
		}
		catch (std::bad_any_cast const& e)
		{
			DAEMON_LOG(LogScript, eLogVerbosity::Error,
				Stringf("HighLevelEntityAPI::ExecuteCallback - Failed to cast callback to v8::Global<v8::Function>: {}",
					e.what()));
		}
		catch (std::exception const& e)
		{
			DAEMON_LOG(LogScript, eLogVerbosity::Error,
				Stringf("HighLevelEntityAPI::ExecuteCallback - Unexpected exception: {}", e.what()));
		}
	}
	// V8 lock automatically released here
}

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Phase 2 Simplifications:
//   - Callbacks marked as ready immediately (no actual async command processing yet)
//   - MoveBy/MoveCameraBy not fully implemented (requires reading EntityStateBuffer)
//   - LookAtCamera not implemented (requires orientation calculation)
//   - Callback execution stubs (requires V8 integration)
//
// Phase 2b Extensions:
//   - Implement MoveBy/MoveCameraBy with EntityStateBuffer reading
//   - Implement LookAtCamera with Mat44::LookAt calculation
//   - Implement full V8 callback execution with error handling
//   - Add callback timeout mechanism
//
// Error Resilience:
//   - All methods check queue submission success
//   - Failed submissions logged but don't crash
//   - Invalid callbacks logged but don't crash
//   - JavaScript callback errors will be caught (when V8 integration complete)
//----------------------------------------------------------------------------------------------------
