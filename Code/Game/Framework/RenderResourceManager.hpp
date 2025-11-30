//----------------------------------------------------------------------------------------------------
// RenderResourceManager.hpp
// Game Framework Module - Render Resource Management
//
// Purpose:
//   Manages rendering resources (vertex buffers, textures, materials) separately from game state.
//   Maintains EntityID → Rendering Resource mappings for clean separation of concerns.
//
// Design Rationale:
//   - EntityState should only contain game logic state (position, color, meshType)
//   - Rendering resources (VBO handles, textures) belong in main thread rendering layer
//   - Separates concerns: Game state vs Render resources (SOLID principles)
//
// Thread Safety:
//   - Main thread only (owns all GPU resources)
//   - No thread safety needed (single-threaded rendering)
//
// Author: Phase 5 - Entity/Render Separation
// Date: 2025-11-29
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Entity/EntityID.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include <unordered_map>
#include <string>

//----------------------------------------------------------------------------------------------------
// RenderResourceManager Class
//
// Manages rendering resources separately from game state EntityState.
//
// Responsibilities:
//   - Map EntityID → Vertex Buffer Handle (for entity rendering)
//   - Map MeshType → Shared Vertex Data (for resource sharing)
//   - Create/destroy rendering resources based on entity lifecycle
//
// Usage Pattern:
//
//   Main Thread (ProcessRenderCommands):
//     manager->RegisterEntity(entityId, "cube");  // Create render resources
//
//   Main Thread (RenderEntities):
//     VertexList_PCU const* verts = manager->GetVerticesForEntity(entityId);
//     if (verts) renderer->DrawVertexArray(verts->size(), verts->data());
//
//   Main Thread (Entity Destruction):
//     manager->UnregisterEntity(entityId);  // Cleanup render resources
//
// Design Notes:
//   - Vertex buffer sharing: Multiple entities with same meshType share vertex data
//   - Lazy creation: Vertex data created on first RegisterEntity() call for each meshType
//   - Main thread ownership: All methods assume main thread context (GPU access)
//----------------------------------------------------------------------------------------------------
class RenderResourceManager
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	RenderResourceManager();
	~RenderResourceManager();

	// Non-copyable (owns GPU resources)
	RenderResourceManager(RenderResourceManager const&)            = delete;
	RenderResourceManager& operator=(RenderResourceManager const&) = delete;

	//------------------------------------------------------------------------------------------------
	// Entity Resource Management (Main Thread)
	//------------------------------------------------------------------------------------------------

	// Register entity for rendering (creates vertex buffer if needed)
	// Parameters:
	//   entityId - Unique entity identifier
	//   meshType - Mesh type string ("cube", "sphere", "grid", etc.)
	//   radius - Mesh size/scale parameter
	//   color - Mesh color (for procedural geometry)
	// Returns: Vertex buffer handle (0 = failure)
	int RegisterEntity(EntityID entityId, std::string const& meshType, float radius, Rgba8 const& color);

	// Unregister entity (cleanup render resources)
	// Parameters: entityId - Entity to remove
	void UnregisterEntity(EntityID entityId);

	// Get vertex data for entity rendering
	// Parameters: entityId - Entity to query
	// Returns: Pointer to vertex list (nullptr if not found)
	VertexList_PCU const* GetVerticesForEntity(EntityID entityId) const;

	//------------------------------------------------------------------------------------------------
	// Statistics / Debugging
	//------------------------------------------------------------------------------------------------

	// Get total registered entities
	size_t GetEntityCount() const { return m_entityToVBHandle.size(); }

	// Get unique mesh type count
	size_t GetMeshTypeCount() const { return m_meshTypeToHandle.size(); }

private:
	//------------------------------------------------------------------------------------------------
	// Vertex Buffer Creation (Main Thread)
	//------------------------------------------------------------------------------------------------

	// Create geometry for mesh type (internal helper)
	// Parameters:
	//   meshType - Mesh type string
	//   radius - Mesh size/scale
	//   color - Mesh color
	// Returns: Vertex buffer handle (0 = failure)
	int CreateGeometryForMeshType(std::string const& meshType, float radius, Rgba8 const& color);

	//------------------------------------------------------------------------------------------------
	// Resource Storage
	//------------------------------------------------------------------------------------------------

	// Map: EntityID → Vertex Buffer Handle
	// Allows fast lookup of VBO for entity rendering
	std::unordered_map<EntityID, int> m_entityToVBHandle;

	// Map: Vertex Buffer Handle → Shared Vertex Data
	// Enables direct lookup of vertex data by handle
	std::unordered_map<int, VertexList_PCU> m_handleToVertices;

	// Map: MeshType → Vertex Buffer Handle
	// Tracks which handle corresponds to each mesh type (for sharing)
	std::unordered_map<std::string, int> m_meshTypeToHandle;

	// Next available vertex buffer handle (for handle allocation)
	int m_nextVBHandle;
};

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Why Separate from EntityState?
//   - EntityState is game logic state (copied between threads via double-buffering)
//   - Vertex buffer handles are GPU resources (main thread only, no copying)
//   - Mixing concerns violates Single Responsibility Principle
//
// Resource Sharing Strategy:
//   - Multiple entities with same meshType share vertex data
//   - Example: 100 cubes → 1 shared vertex buffer instead of 100 duplicates
//   - Memory savings: O(n) → O(k) where k = unique mesh types
//
// Future Extensions:
//   - Texture management (EntityID → TextureHandle)
//   - Material management (EntityID → Material)
//   - Shader management (MeshType → Shader)
//   - LOD management (EntityID → LODGroup)
//   - Instanced rendering (MeshType → InstancedBatch)
//----------------------------------------------------------------------------------------------------
