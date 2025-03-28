// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "animation/skinning.h"

#include "dx/dx_command_list.h"
#include "dx/dx_barrier_batcher.h"
#include "dx/dx_profiling.h"
#include "dx/dx_buffer.h"

#include "rendering/material.h"
#include "rendering/render_pass.h"

#include "skinning_rs.hlsli"

namespace era_engine::animation
{
	static dx_pipeline skinningPipeline;
	static dx_pipeline clothSkinningPipeline;

	struct skinning_call
	{
		dx_vertex_buffer_group_view vertexBuffer;
		vertex_range range;
		uint32 jointOffset;
		uint32 numJoints;
		uint32 vertexOffset;
	};

	struct cloth_skinning_call
	{
		dx_vertex_buffer_view vertexBuffer;
		uint32 gridSizeX;
		uint32 gridSizeY;
		uint32 vertexOffset;
	};

	struct skinning_data
	{
		vertex_buffer_group skinnedVertexBuffer;
		ref<dx_buffer> skinningMatricesBuffer;

		mat4* skinningMatrices;
		uint32 numSkinningMatrices;
		uint32 matrixOffset;

		skinning_call* calls;
		uint32 numCalls;

		cloth_skinning_call* clothCalls;
		uint32 numClothCalls;
	};

	struct skinning_pipeline
	{
		PIPELINE_SETUP_DECL{}

			PIPELINE_COMPUTE_DECL(skinning_data)
		{
			PROFILE_ALL(cl, "Skinning");

			if (data.numCalls)
			{
				PROFILE_ALL(cl, "Skeletal");

				mat4* mats = (mat4*)mapBuffer(data.skinningMatricesBuffer, false);
				memcpy(mats + data.matrixOffset, data.skinningMatrices, sizeof(mat4) * data.numSkinningMatrices);
				unmapBuffer(data.skinningMatricesBuffer, true, map_range{ data.matrixOffset, data.numSkinningMatrices });

				cl->setPipelineState(*skinningPipeline.pipeline);
				cl->setComputeRootSignature(*skinningPipeline.rootSignature);

				cl->setRootComputeSRV(SKINNING_RS_MATRICES, data.skinningMatricesBuffer->gpuVirtualAddress + sizeof(mat4) * data.matrixOffset);
				cl->setRootComputeUAV(SKINNING_RS_OUTPUT0, data.skinnedVertexBuffer.positions);
				cl->setRootComputeUAV(SKINNING_RS_OUTPUT1, data.skinnedVertexBuffer.others);

				for (uint32 i = 0; i < data.numCalls; ++i)
				{
					auto& c = data.calls[i];
					cl->setRootComputeSRV(SKINNING_RS_INPUT_VERTEX_BUFFER0, c.vertexBuffer.positions.view.BufferLocation);
					cl->setRootComputeSRV(SKINNING_RS_INPUT_VERTEX_BUFFER1, c.vertexBuffer.others.view.BufferLocation);
					cl->setCompute32BitConstants(SKINNING_RS_CB, skinning_cb{ c.jointOffset, c.numJoints, c.range.firstVertex, c.range.numVertices, c.vertexOffset });
					cl->dispatch(bucketize(c.range.numVertices, 512));
				}
			}
			if (data.numClothCalls)
			{
				PROFILE_ALL(cl, "Cloth");

				cl->setPipelineState(*clothSkinningPipeline.pipeline);
				cl->setComputeRootSignature(*clothSkinningPipeline.rootSignature);

				cl->setRootComputeUAV(CLOTH_SKINNING_RS_OUTPUT0, data.skinnedVertexBuffer.positions);
				cl->setRootComputeUAV(CLOTH_SKINNING_RS_OUTPUT1, data.skinnedVertexBuffer.others);

				for (uint32 i = 0; i < data.numClothCalls; ++i)
				{
					auto& c = data.clothCalls[i];
					cl->setRootComputeSRV(CLOTH_SKINNING_RS_INPUT, c.vertexBuffer.view.BufferLocation);
					cl->setCompute32BitConstants(CLOTH_SKINNING_RS_CB, cloth_skinning_cb{ c.gridSizeX, c.gridSizeY, c.vertexOffset });
					cl->dispatch(bucketize(c.gridSizeX, 15), bucketize(c.gridSizeY, 15)); // 15 is correct here.
				}
			}
		}
	};

#define MAX_NUM_SKINNING_MATRICES_PER_FRAME 4096
#define MAX_NUM_SKINNED_VERTICES_PER_FRAME (1024 * 256)

	static ref<dx_buffer> skinningMatricesBuffer; // Buffered frames are in a single dx_buffer.

	static uint32 currentSkinnedVertexBuffer;
	static vertex_buffer_group skinnedVertexBuffer[2]; // We have two of these, so that we can compute screen space velocities.

	static mat4 skinningMatrices[MAX_NUM_SKINNING_MATRICES_PER_FRAME];
	static volatile uint32 numSkinningMatricesThisFrame;

	static skinning_call calls[1024];
	static volatile uint32 numCalls;

	static cloth_skinning_call clothCalls[128];
	static volatile uint32 numClothCalls;

	static volatile uint32 totalNumVertices;

	void initializeSkinning()
	{
		skinningMatricesBuffer = createUploadBuffer(sizeof(mat4), MAX_NUM_SKINNING_MATRICES_PER_FRAME * NUM_BUFFERED_FRAMES, 0);

		for (uint32 i = 0; i < 2; ++i)
		{
			skinnedVertexBuffer[i].positions = createVertexBuffer(sizeof(vec3), MAX_NUM_SKINNED_VERTICES_PER_FRAME, 0, true);
			skinnedVertexBuffer[i].others = createVertexBuffer(
				getVertexOthersSize(mesh_creation_flags_with_positions | mesh_creation_flags_with_uvs | mesh_creation_flags_with_normals | mesh_creation_flags_with_tangents),
				MAX_NUM_SKINNED_VERTICES_PER_FRAME, 0, true);
		}

		skinningPipeline = createReloadablePipeline("skinning_cs");
		clothSkinningPipeline = createReloadablePipeline("cloth_skinning_cs");
	}

	NODISCARD std::tuple<dx_vertex_buffer_group_view, mat4*> skinObject(const dx_vertex_buffer_group_view& vertexBuffer, vertex_range range, uint32 numJoints)
	{
		uint32 jointOffset = atomic_add(numSkinningMatricesThisFrame, numJoints);
		ASSERT(jointOffset + numJoints <= MAX_NUM_SKINNING_MATRICES_PER_FRAME);

		uint32 vertexOffset = atomic_add(totalNumVertices, range.numVertices);
		ASSERT(vertexOffset + range.numVertices <= MAX_NUM_SKINNED_VERTICES_PER_FRAME);

		uint32 callIndex = atomic_increment(numCalls);
		ASSERT(callIndex < arraysize(calls));

		calls[callIndex] =
		{
			vertexBuffer,
			range,
			jointOffset,
			numJoints,
			vertexOffset
		};

		uint32 numVertices = range.numVertices;

		auto& positions = skinnedVertexBuffer[currentSkinnedVertexBuffer].positions;
		auto& others = skinnedVertexBuffer[currentSkinnedVertexBuffer].others;

		dx_vertex_buffer_group_view result;
		result.positions.view.BufferLocation = positions->gpuVirtualAddress + positions->elementSize * vertexOffset;
		result.positions.view.SizeInBytes = positions->elementSize * numVertices;
		result.positions.view.StrideInBytes = positions->elementSize;
		result.others.view.BufferLocation = others->gpuVirtualAddress + others->elementSize * vertexOffset;
		result.others.view.SizeInBytes = others->elementSize * numVertices;
		result.others.view.StrideInBytes = others->elementSize;

		return { result, skinningMatrices + jointOffset };
	}

	NODISCARD std::tuple<dx_vertex_buffer_group_view, mat4*> skinObject(const dx_vertex_buffer_group_view& vertexBuffer, uint32 numVertices, uint32 numJoints)
	{
		auto [vb, mats] = skinObject(vertexBuffer, vertex_range{ 0, numVertices }, numJoints);
		return { vb, mats };
	}

	NODISCARD std::tuple<dx_vertex_buffer_group_view, mat4*> skinObject(const dx_vertex_buffer_group_view& vertexBuffer, submesh_info submesh, uint32 numJoints)
	{
		auto [vb, mats] = skinObject(vertexBuffer, vertex_range{ submesh.baseVertex, submesh.numVertices }, numJoints);

		return { vb, mats };
	}

	NODISCARD dx_vertex_buffer_group_view skinCloth(const dx_vertex_buffer_view& inpositions, uint32 gridSizeX, uint32 gridSizeY)
	{
		uint32 numVertices = gridSizeX * gridSizeY;
		uint32 vertexOffset = atomic_add(totalNumVertices, numVertices);
		ASSERT(vertexOffset + numVertices <= MAX_NUM_SKINNED_VERTICES_PER_FRAME);

		uint32 callIndex = atomic_increment(numClothCalls);
		ASSERT(callIndex < arraysize(clothCalls));

		clothCalls[callIndex] =
		{
			inpositions,
			gridSizeX,
			gridSizeY,
			vertexOffset
		};

		auto& positions = skinnedVertexBuffer[currentSkinnedVertexBuffer].positions;
		auto& others = skinnedVertexBuffer[currentSkinnedVertexBuffer].others;

		dx_vertex_buffer_group_view result;
		result.positions.view.BufferLocation = positions->gpuVirtualAddress + positions->elementSize * vertexOffset;
		result.positions.view.SizeInBytes = positions->elementSize * numVertices;
		result.positions.view.StrideInBytes = positions->elementSize;
		result.others.view.BufferLocation = others->gpuVirtualAddress + others->elementSize * vertexOffset;
		result.others.view.SizeInBytes = others->elementSize * numVertices;
		result.others.view.StrideInBytes = others->elementSize;
		return result;
	}

	void performSkinning(compute_pass* computePass)
	{
		// TODO: We currently make no attempt to ensure that all draw calls have actually been written completely, if executed on another thread. 
		if (numCalls > 0 || numClothCalls > 0)
		{
			skinning_data data;
			data.skinnedVertexBuffer = skinnedVertexBuffer[currentSkinnedVertexBuffer];
			data.skinningMatricesBuffer = skinningMatricesBuffer;

			data.skinningMatrices = skinningMatrices;
			data.numSkinningMatrices = numSkinningMatricesThisFrame;
			data.matrixOffset = dxContext.bufferedFrameID * MAX_NUM_SKINNING_MATRICES_PER_FRAME;

			data.calls = calls;
			data.numCalls = numCalls;

			data.clothCalls = clothCalls;
			data.numClothCalls = numClothCalls;

			computePass->addTask<skinning_pipeline>(compute_pass_frame_start, data);

			currentSkinnedVertexBuffer = 1 - currentSkinnedVertexBuffer;
			numCalls = 0;
			numClothCalls = 0;
			numSkinningMatricesThisFrame = 0;
			totalNumVertices = 0;
		}
	}
}