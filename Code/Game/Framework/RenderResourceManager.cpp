//----------------------------------------------------------------------------------------------------
// RenderResourceManager.cpp
// Game Framework Module - Render Resource Management Implementation
//
// Author: Phase 5 - Entity/Render Separation
// Date: 2025-11-29
//----------------------------------------------------------------------------------------------------

#include "Game/Framework/RenderResourceManager.hpp"

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Renderer/VertexUtils.hpp"

//----------------------------------------------------------------------------------------------------
// Construction / Destruction
//----------------------------------------------------------------------------------------------------

RenderResourceManager::RenderResourceManager()
	: m_nextVBHandle(1)  // Start from 1 (0 = invalid handle)
{
}

//----------------------------------------------------------------------------------------------------
RenderResourceManager::~RenderResourceManager()
{
	// Cleanup: Clear all resource mappings
	// Vertex data automatically cleaned up by std::unordered_map destructors
	m_entityToVBHandle.clear();
	m_handleToVertices.clear();
	m_meshTypeToHandle.clear();
}

//----------------------------------------------------------------------------------------------------
// Entity Resource Management
//----------------------------------------------------------------------------------------------------

int RenderResourceManager::RegisterEntity(EntityID entityId, std::string const& meshType, float radius, Rgba8 const& color)
{
	// Create geometry for mesh type if it doesn't exist yet
	int vbHandle = CreateGeometryForMeshType(meshType, radius, color);

	if (vbHandle != 0)
	{
		// Map entity to vertex buffer handle
		m_entityToVBHandle[entityId] = vbHandle;
	}

	return vbHandle;
}

//----------------------------------------------------------------------------------------------------
void RenderResourceManager::UnregisterEntity(EntityID entityId)
{
	// Remove entity from mapping
	// Note: We don't delete vertex data (shared between entities with same meshType)
	m_entityToVBHandle.erase(entityId);
}

//----------------------------------------------------------------------------------------------------
VertexList_PCU const* RenderResourceManager::GetVerticesForEntity(EntityID entityId) const
{
	// Find vertex buffer handle for entity
	auto handleIt = m_entityToVBHandle.find(entityId);
	if (handleIt == m_entityToVBHandle.end())
	{
		return nullptr;  // Entity not found
	}

	int vbHandle = handleIt->second;

	// Find vertex data by handle
	auto vertsIt = m_handleToVertices.find(vbHandle);
	if (vertsIt == m_handleToVertices.end())
	{
		return nullptr;  // Vertex data not found
	}

	return &vertsIt->second;
}

//----------------------------------------------------------------------------------------------------
// Vertex Buffer Creation (Private)
//----------------------------------------------------------------------------------------------------

int RenderResourceManager::CreateGeometryForMeshType(std::string const& meshType, float radius, Rgba8 const& color)
{
	// Check if geometry already exists for this mesh type (share vertex buffers)
	auto handleIt = m_meshTypeToHandle.find(meshType);
	if (handleIt != m_meshTypeToHandle.end())
	{
		// Geometry already exists, return existing handle (vertex buffer sharing)
		return handleIt->second;
	}

	// Create vertex list
	VertexList_PCU verts;

	if (meshType == "cube")
	{
		// Create cube geometry
		Vec3 const frontBottomLeft(0.5f, -0.5f, -0.5f);
		Vec3 const frontBottomRight(0.5f, 0.5f, -0.5f);
		Vec3 const frontTopLeft(0.5f, -0.5f, 0.5f);
		Vec3 const frontTopRight(0.5f, 0.5f, 0.5f);
		Vec3 const backBottomLeft(-0.5f, 0.5f, -0.5f);
		Vec3 const backBottomRight(-0.5f, -0.5f, -0.5f);
		Vec3 const backTopLeft(-0.5f, 0.5f, 0.5f);
		Vec3 const backTopRight(-0.5f, -0.5f, 0.5f);

		AddVertsForQuad3D(verts, frontBottomLeft, frontBottomRight, frontTopLeft, frontTopRight, color);          // +X Red
		AddVertsForQuad3D(verts, backBottomLeft, backBottomRight, backTopLeft, backTopRight, color);             // -X -Red (Cyan)
		AddVertsForQuad3D(verts, frontBottomRight, backBottomLeft, frontTopRight, backTopLeft, color);          // -Y -Green (Magenta)
		AddVertsForQuad3D(verts, backBottomRight, frontBottomLeft, backTopRight, frontTopLeft, color);        // +Y Green
		AddVertsForQuad3D(verts, frontTopLeft, frontTopRight, backTopRight, backTopLeft, color);                 // +Z Blue
		AddVertsForQuad3D(verts, backBottomRight, backBottomLeft, frontBottomLeft, frontBottomRight, color);   // -Z -Blue (Yellow)
	}
	else if (meshType == "sphere")
	{
		// Create sphere geometry
		AddVertsForSphere3D(verts, Vec3::ZERO, radius, color, AABB2::ZERO_TO_ONE, 32, 16);
	}
	else if (meshType == "grid")
	{
		// Create grid geometry with multiple colored lines
		// Based on original Prop::InitializeLocalVertsForGrid() implementation
		// Grid lies in XY plane (horizontal when looking down Z-axis)
		float gridLineLength = 100.f;

		for (int i = -(int)gridLineLength / 2; i < (int)gridLineLength / 2; i++)
		{
			float lineWidth = 0.05f;
			if (i == 0)
			{
				lineWidth = 0.3f;  // Center lines are thicker
			}

			// Create line boxes in X direction (X-axis lines)
			AABB3 boundsX(
				Vec3(-gridLineLength / 2.f, -lineWidth / 2.f + (float)i, -lineWidth / 2.f),
				Vec3(gridLineLength / 2.f, lineWidth / 2.f + (float)i, lineWidth / 2.f)
			);

			// Create line boxes in Y direction (Y-axis lines)
			AABB3 boundsY(
				Vec3(-lineWidth / 2.f + (float)i, -gridLineLength / 2.f, -lineWidth / 2.f),
				Vec3(lineWidth / 2.f + (float)i, gridLineLength / 2.f, lineWidth / 2.f)
			);

			// Determine line colors
			Rgba8 colorX = Rgba8::DARK_GREY;
			Rgba8 colorY = Rgba8::DARK_GREY;

			if (i % 5 == 0)  // Every 5th line is colored
			{
				colorX = Rgba8::RED;    // X-axis lines are red
				colorY = Rgba8::GREEN;  // Y-axis lines are green
			}

			// Add geometry for both X and Y direction lines
			AddVertsForAABB3D(verts, boundsX, colorX);
			AddVertsForAABB3D(verts, boundsY, colorY);
		}
	}
	else if (meshType == "plane")
	{
		// Create plane geometry (simple quad)
		float halfSize = radius;
		Vec3  bottomLeft(-halfSize, -halfSize, 0.0f);
		Vec3  bottomRight(halfSize, -halfSize, 0.0f);
		Vec3  topLeft(-halfSize, halfSize, 0.0f);
		Vec3  topRight(halfSize, halfSize, 0.0f);
		AddVertsForQuad3D(verts, bottomLeft, bottomRight, topLeft, topRight, color);
	}
	else
	{
		return 0;  // Unknown mesh type
	}

	// Check if geometry was created
	if (verts.empty())
	{
		return 0;
	}

	// Allocate handle and store vertex data
	int handle = m_nextVBHandle++;
	m_handleToVertices[handle] = verts;
	m_meshTypeToHandle[meshType] = handle;

	return handle;
}
