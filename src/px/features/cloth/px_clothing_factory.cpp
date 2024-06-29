// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "pch.h"
#include "px/features/cloth/px_clothing_factory.h"
#include <animation/skinning.h>
#include "application.h"

NODISCARD std::tuple<dx_vertex_buffer_group_view, dx_vertex_buffer_group_view, dx_index_buffer_view, submesh_info> physics::px_cloth_render_component::getRenderData(const px_cloth_component& cloth)
{
	CPU_PROFILE_BLOCK("Get cloth render data");

	uint32 numVertices = cloth.numX * cloth.numZ;
	uint32 numTriangles = (cloth.numX - 1) * (cloth.numZ - 1) * 2;

	auto [positionVertexBuffer, positionPtr] = dxContext.createDynamicVertexBuffer(sizeof(vec3), numVertices);
	memcpy(positionPtr, cloth.getPositions(), numVertices * sizeof(vec3));

	dx_vertex_buffer_group_view vb = era_engine::animation::skinCloth(positionVertexBuffer, cloth.numX, cloth.numZ);
	submesh_info sm{};
	sm.baseVertex = 0;
	sm.firstIndex = 0;
	sm.numIndices = numTriangles * 3;
	sm.numVertices = numVertices;

	dx_vertex_buffer_group_view prev = prevFrameVB;
	prevFrameVB = vb;

	if (!indexBuffer)
	{
		::std::vector<indexed_triangle16> triangles;
		triangles.reserve(numTriangles);
		for (uint32 y = 0; y < cloth.numZ - 1; ++y)
		{
			for (uint32 x = 0; x < cloth.numX - 1; ++x)
			{
				uint16 tlIndex = y * cloth.numX + x;
				uint16 trIndex = tlIndex + 1;
				uint16 blIndex = tlIndex + cloth.numX;
				uint16 brIndex = blIndex + 1;

				triangles.push_back({ tlIndex, blIndex, brIndex });
				triangles.push_back({ tlIndex, brIndex, trIndex });
			}
		}

		indexBuffer = createIndexBuffer(sizeof(uint16), (uint32)triangles.size() * 3, triangles.data());
	}

	return { vb, prev, indexBuffer, sm };
}

physics::px_cloth_component::px_cloth_component(uint32 handle, int nX, int nZ, const vec3& position) noexcept : px_physics_component_base(handle), numX(nX), numZ(nZ)
{
	clothSystem = make_ref<px_cloth_system>(numX, numZ, physx::createPxVec3(position));
	positions = (vec3*)malloc(numX * numZ * sizeof(vec3));
	//clothSystem->translate(PxVec4(position.x, position.y, position.z, 0));
	eentity entity{ (entity_handle)entityHandle, &globalApp.getCurrentScene()->registry };
	if (!entity.hasComponent<physics::px_cloth_render_component>())
		entity.addComponent<physics::px_cloth_render_component>();
}

vec3* physics::px_cloth_component::getPositions() const noexcept
{
	PxVec4* bufferPos = clothSystem->posBuffer;
	
	for (uint32 i = 0; i < numX * numZ; i += 4)
	{
		positions[i + 0] = vec3(bufferPos[i + 0].x, bufferPos[i + 0].y, bufferPos[i + 0].z);
		positions[i + 1] = vec3(bufferPos[i + 1].x, bufferPos[i + 1].y, bufferPos[i + 1].z);
		positions[i + 2] = vec3(bufferPos[i + 2].x, bufferPos[i + 2].y, bufferPos[i + 2].z);
		positions[i + 3] = vec3(bufferPos[i + 3].x, bufferPos[i + 3].y, bufferPos[i + 3].z);
	}

	return positions;
}
