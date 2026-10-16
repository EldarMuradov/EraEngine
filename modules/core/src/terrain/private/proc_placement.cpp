// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "terrain/proc_placement.h"

#include "dx/dx_command_list.h"
#include "dx/dx_pipeline.h"
#include "dx/dx_barrier_batcher.h"
#include "dx/dx_profiling.h"

#include "geometry/mesh_builder.h"

#include "rendering/render_utils.h"
#include "rendering/material.h"
#include "rendering/render_pass.h"
#include "rendering/pbr.h"

#include <rttr/registration>

#include "proc_placement_rs.hlsli"

namespace era_engine
{
	static DXPipeline generate_points_pipeline;
	static DXPipeline prefix_sum_pipeline;
	static DXPipeline create_draw_calls_pipeline;
	static DXPipeline create_transforms_pipeline;

	static DXPipeline visualize_points_pipeline;

	static DXMesh visualize_points_mesh;
	static SubmeshInfo visualize_points_submesh;
	static DXCommandSignature visualize_points_command_signature;

	void initialize_procedural_placement_pipelines()
	{
		generate_points_pipeline = create_reloadable_pipeline("proc_placement_generate_points_cs");
		prefix_sum_pipeline = create_reloadable_pipeline("proc_placement_prefix_sum_cs");
		create_draw_calls_pipeline = create_reloadable_pipeline("proc_placement_create_draw_calls_cs");
		create_transforms_pipeline = create_reloadable_pipeline("proc_placement_create_transforms_cs");

		{
			auto& desc = CREATE_GRAPHICS_PIPELINE
				.input_layout(input_layout_position_uv_normal_tangent)
				.render_targets(ldr_format, depth_stencil_format);

			visualize_points_pipeline = create_reloadable_pipeline(desc, { "proc_placement_points_vs", "proc_placement_points_ps" }, RS_IN_PIXEL_SHADER, true);
		}


		MeshBuilder builder(MESH_CREATION_FLAGS_WITH_POSITIONS);
		//builder.pushSphere({});
		builder.push_arrow({});
		visualize_points_submesh = builder.end_submesh();
		visualize_points_mesh = builder.create_dx_mesh();


		D3D12_INDIRECT_ARGUMENT_DESC argument_descs[2] = {};
		argument_descs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
		argument_descs[0].ShaderResourceView.RootParameterIndex = 1;
		argument_descs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
		visualize_points_command_signature = create_command_signature(*visualize_points_pipeline.root_signature, argument_descs, arraysize(argument_descs), sizeof(PlacementDraw));
	}

	struct RenderProcPlacementLayerData
	{
		ref<DXBuffer> command_buffer;
		uint32 command_buffer_offset;

		DXVertexBufferGroupView vertex_buffer;
		DXIndexBufferView index_buffer;

		ref<DXTexture> albedo;
	};

	struct RenderProcPlacementLayerPipeline
	{
		PIPELINE_SETUP_DECL;
		PIPELINE_RENDER_DECL(RenderProcPlacementLayerData);
	};

	PIPELINE_SETUP_IMPL(RenderProcPlacementLayerPipeline)
	{
		cl->set_pipeline_state(*visualize_points_pipeline.pipeline);
		cl->set_graphics_root_signature(*visualize_points_pipeline.root_signature);
		cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cl->set_graphics_dynamic_constant_buffer(0, common.camera_cbv);
	}

	PIPELINE_RENDER_IMPL(RenderProcPlacementLayerPipeline, RenderProcPlacementLayerData)
	{
		cl->set_vertex_buffer(0, data.vertex_buffer.positions);
		cl->set_vertex_buffer(1, data.vertex_buffer.others);
		cl->set_index_buffer(data.index_buffer);
		cl->set_descriptor_heap_srv(2, 0, data.albedo);
		cl->draw_indirect(visualize_points_command_signature, 1, data.command_buffer, data.command_buffer_offset * sizeof(PlacementDraw));
	}

#define READBACK 0

#if READBACK
	static ref<DXBuffer> readbackIndirect;
	static ref<DXBuffer> readbackMeshCounts;
	static ref<DXBuffer> readbackMeshOffsets;
#endif

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<ProcPlacementComponent>("ProcPlacementComponent")
			.constructor<>()
			.constructor<ref<Entity::EcsData>, const std::vector<ProcPlacementLayerDesc>&>()
			.property("layers", &ProcPlacementComponent::layers);
	}

	ProcPlacementComponent::ProcPlacementComponent(ref<Entity::EcsData> _data, const std::vector<ProcPlacementLayerDesc>& _layers)
		: Component(_data)
	{
		std::vector<PlacementDraw> draw_args;

		{
			PlacementDraw draw;
			draw.draw.BaseVertexLocation = visualize_points_submesh.base_vertex;
			draw.draw.IndexCountPerInstance = visualize_points_submesh.num_indices;
			draw.draw.StartIndexLocation = visualize_points_submesh.first_index;
			draw.draw.InstanceCount = 0;
			draw.draw.StartInstanceLocation = 0;
			draw_args.push_back(draw);

			submesh_to_mesh.push_back(0);
		}

		uint32 global_mesh_offset = 1;

		for (const auto& layer_desc : layers)
		{
			PlacementLayer layer;
			layer.name = layer_desc.name;
			layer.footprint = layer_desc.footprint;
			layer.global_mesh_offset = global_mesh_offset;
			layer.num_meshes = 0;

			for (uint32 i = 0; i < 4; ++i)
			{
				const ref<MultiMesh>& mesh = layer_desc.meshes[i];
				layer.meshes[i] = mesh;

				if (mesh)
				{
					++layer.num_meshes;

					for (const auto& sub : mesh->submeshes)
					{
						PlacementDraw draw;
						draw.draw.BaseVertexLocation = sub.info.base_vertex;
						draw.draw.IndexCountPerInstance = sub.info.num_indices;
						draw.draw.StartIndexLocation = sub.info.first_index;
						draw.draw.InstanceCount = 0;
						draw.draw.StartInstanceLocation = 0;
						draw_args.push_back(draw);

						submesh_to_mesh.push_back(global_mesh_offset + i);

						has_valid_meshes = true;
					}
				}
			}

			global_mesh_offset += layer.num_meshes;

			this->layers.push_back(layer);
		}

		ASSERT(global_mesh_offset <= 512); // Prefix sum currently only supports up to 512 points.

		placement_point_buffer = create_buffer(sizeof(PlacementPoint), 100000, 0, true);
		transform_buffer = create_buffer(sizeof(PlacementTransform), 100000, 0, true);
		mesh_count_buffer = create_buffer(sizeof(uint32), global_mesh_offset, 0, true, true);
		mesh_offset_buffer = create_buffer(sizeof(uint32), global_mesh_offset, 0, true, true);
		submesh_to_mesh_buffer = create_buffer(sizeof(uint32), (uint32)submesh_to_mesh.size(), submesh_to_mesh.data());
		draw_indirect_buffer = create_buffer(sizeof(PlacementDraw), (uint32)draw_args.size(), draw_args.data(), true);


#if READBACK
		readbackIndirect = create_readback_buffer(sizeof(PlacementDraw), (uint32)draw_args.size());
		readbackMeshCounts = create_readback_buffer(sizeof(uint32), global_mesh_offset);
		readbackMeshOffsets = create_readback_buffer(sizeof(uint32), global_mesh_offset);
#endif
	}

	ProcPlacementComponent::~ProcPlacementComponent()
	{
	}

	void ProcPlacementComponent::generate(const RenderCamera& camera, const TerrainComponent& terrain, const vec3& position_offset)
	{
		if (!has_valid_meshes)
		{
			return;
		}

		DXCommandList* cl = dx_context.get_free_compute_command_list(false);

		{
			PROFILE_ALL(cl, "Procedural placement");

			{
				PROFILE_ALL(cl, "Generate points");

				CameraFrustumPlanes frustum = camera.get_world_space_frustum_planes();

				const float radius_in_uv_space = sqrt(1.f / (2.f * sqrt(3.f) * arraysize(POISSON_SAMPLES)));
				float diameter_in_uv_space = radius_in_uv_space * 2.f;
				float diameter_in_world_space = diameter_in_uv_space * terrain.chunk_size;

				cl->set_pipeline_state(*generate_points_pipeline.pipeline);
				cl->set_compute_root_signature(*generate_points_pipeline.root_signature);

				vec3 min_corner = terrain.get_min_corner(position_offset);
				vec3 chunk_size(terrain.chunk_size, terrain.amplitude_scale, terrain.chunk_size);

				cl->clear_uav(mesh_count_buffer, 0u);

				for (uint32 z = 0; z < terrain.chunks_per_dim; ++z)
				{
					for (uint32 x = 0; x < terrain.chunks_per_dim; ++x)
					{
						auto& chunk = terrain.chunk(x, z);
						vec3 chunk_min_corner = min_corner + vec3(x * terrain.chunk_size, 0.f, z * terrain.chunk_size);
						vec3 chunk_max_corner = chunk_min_corner + chunk_size;

						BoundingBox aabb = { chunk_min_corner, chunk_max_corner };
						if (!frustum.cull_world_space_aabb(aabb))
						{
							cl->set_descriptor_heap_srv(PROC_PLACEMENT_GENERATE_POINTS_RS_RESOURCES, 0, chunk.heightmap);
							cl->set_descriptor_heap_srv(PROC_PLACEMENT_GENERATE_POINTS_RS_RESOURCES, 1, chunk.normalmap);
							cl->set_descriptor_heap_uav(PROC_PLACEMENT_GENERATE_POINTS_RS_RESOURCES, 2, placement_point_buffer);
							cl->set_descriptor_heap_uav(PROC_PLACEMENT_GENERATE_POINTS_RS_RESOURCES, 3, mesh_count_buffer);


							ProcPlacementGeneratePointsCB cb;
							cb.amplitude_scale = terrain.amplitude_scale;
							cb.chunk_corner = chunk_min_corner;
							cb.chunk_size = terrain.chunk_size;

							for (const auto& layer : layers)
							{
								float footprint = layer.footprint;

								float scaling = diameter_in_world_space / footprint;
								uint32 num_groups_per_dim = (uint32)ceil(scaling);

								cb.densities = vec4(0.f);
								for (uint32 i = 0; i < layer.num_meshes; ++i)
								{
									cb.densities.data[i] = layer.densities[i];
								}

								cb.uv_scale = 1.f / scaling;
								cb.num_meshes = layer.num_meshes;
								cb.global_mesh_offset = layer.global_mesh_offset;

								cl->set_compute_32bit_constants(PROC_PLACEMENT_GENERATE_POINTS_RS_CB, cb);

								cl->dispatch(num_groups_per_dim, num_groups_per_dim, 1);

								BarrierBatcher(cl)
									.uav(placement_point_buffer)
									.uav(mesh_count_buffer);
							}
						}
					}
				}
			}

			BarrierBatcher(cl)
				.transition(mesh_count_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ)
				.transition(placement_point_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);

			{
				PROFILE_ALL(cl, "Prefix sum");

				cl->set_pipeline_state(*prefix_sum_pipeline.pipeline);
				cl->set_compute_root_signature(*prefix_sum_pipeline.root_signature);

				cl->set_compute_32bit_constants(PROC_PLACEMENT_PREFIX_SUM_RS_CB, PrefixSumCB{ mesh_count_buffer->element_count - 1 });

				// Offset by 1 element, since that is the point count.
				cl->set_root_compute_uav(PROC_PLACEMENT_PREFIX_SUM_RS_OUTPUT, mesh_offset_buffer->gpu_virtual_address + mesh_offset_buffer->element_size);
				cl->set_root_compute_srv(PROC_PLACEMENT_PREFIX_SUM_RS_INPUT, mesh_count_buffer->gpu_virtual_address + mesh_count_buffer->element_size);

				cl->dispatch(1);
			}

			BarrierBatcher(cl)
				.transition(mesh_offset_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);

			{
				PROFILE_ALL(cl, "Create draw calls");

				cl->set_pipeline_state(*create_draw_calls_pipeline.pipeline);
				cl->set_compute_root_signature(*create_draw_calls_pipeline.root_signature);

				ProcPlacementCreateDrawCallsCB cb;
				cb.transform_address_high = (uint32)(transform_buffer->gpu_virtual_address >> 32);
				cb.transform_address_low = (uint32)(transform_buffer->gpu_virtual_address & 0xFFFFFFFF);
				cb.stride = transform_buffer->element_size;
				cl->set_compute_32bit_constants(PROC_PLACEMENT_CREATE_DRAW_CALLS_RS_CB, cb);
				cl->set_root_compute_uav(PROC_PLACEMENT_CREATE_DRAW_CALLS_RS_OUTPUT, draw_indirect_buffer);
				cl->set_root_compute_srv(PROC_PLACEMENT_CREATE_DRAW_CALLS_RS_MESH_COUNTS, mesh_count_buffer);
				cl->set_root_compute_srv(PROC_PLACEMENT_CREATE_DRAW_CALLS_RS_MESH_OFFSETS, mesh_offset_buffer);
				cl->set_root_compute_srv(PROC_PLACEMENT_CREATE_DRAW_CALLS_RS_SUBMESH_TO_MESH, submesh_to_mesh_buffer);

				cl->dispatch(bucketize(draw_indirect_buffer->element_count, PROC_PLACEMENT_CREATE_DRAW_CALLS_BLOCK_SIZE));
			}

#if READBACK
			cl->copy_buffer_region_to_buffer(meshCountBuffer, readbackMeshCounts, 0, meshCountBuffer->element_count, 0);
#endif

			BarrierBatcher(cl)
				.transition(mesh_count_buffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
				.uav(draw_indirect_buffer);

			{
				PROFILE_ALL(cl, "Create transforms");

				cl->set_pipeline_state(*create_transforms_pipeline.pipeline);
				cl->set_compute_root_signature(*create_transforms_pipeline.root_signature);

				cl->set_root_compute_uav(PROC_PLACEMENT_CREATE_TRANSFORMS_RS_TRANSFORMS, transform_buffer);
				cl->set_root_compute_uav(PROC_PLACEMENT_CREATE_TRANSFORMS_RS_MESH_COUNTS, mesh_count_buffer);
				cl->set_root_compute_srv(PROC_PLACEMENT_CREATE_TRANSFORMS_RS_POINTS, placement_point_buffer);
				cl->set_root_compute_srv(PROC_PLACEMENT_CREATE_TRANSFORMS_RS_MESH_OFFSETS, mesh_offset_buffer);

				cl->dispatch(bucketize(placement_point_buffer->element_count, PROC_PLACEMENT_CREATE_TRANSFORMS_BLOCK_SIZE)); // TODO
				cl->uav_barrier(transform_buffer);
				cl->uav_barrier(mesh_count_buffer);
			}
	}

#if READBACK
		cl->transition_barrier(draw_indirect_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
		cl->copy_buffer_region_to_buffer(draw_indirect_buffer, readbackIndirect, 0, draw_indirect_buffer->element_count, 0);
		cl->copy_buffer_region_to_buffer(mesh_offset_buffer, readbackMeshOffsets, 0, mesh_offset_buffer->element_count, 0);
#endif

		dx_context.execute_command_list(cl);

#if READBACK
		dx_context.flush_application();

		{
			PlacementDraw* draw = (PlacementDraw*)map_buffer(readbackIndirect, true);
			uint32* offsets = (uint32*)map_buffer(readbackMeshOffsets, true);
			uint32* counts = (uint32*)map_buffer(readbackMeshCounts, true);

			for (uint32 i = 0; i < readbackIndirect->element_count; ++i)
			{
				uint32 mesh_id = submeshToMesh[i];

				D3D12_GPU_VIRTUAL_ADDRESS expectedAddress = transformBuffer->gpu_virtual_address + offsets[mesh_id] * transformBuffer->element_size;
				uint32 expectedHigh = (uint32)(expectedAddress >> 32);
				uint32 expectedLow = (uint32)(expectedAddress & 0xFFFFFFFF);

				ASSERT(draw[i].transform_srv_high == expectedHigh);
				ASSERT(draw[i].transform_srv_low == expectedLow);
			}

			LOG_MESSAGE("%u vs %u (%u), %u vs %u (%u), %u vs %u (%u)",
				draw[0].draw.InstanceCount, counts[0], offsets[0],
				draw[1].draw.InstanceCount, counts[1], offsets[1],
				draw[2].draw.InstanceCount, counts[2], offsets[2]);

			unmap_buffer(readbackMeshCounts, false);
			unmap_buffer(readbackMeshOffsets, false);
			unmap_buffer(readbackIndirect, false);
		}
#endif
	}

	void ProcPlacementComponent::render(LDRRenderPass* render_pass)
	{
#if 0

		RenderProcPlacementLayerData data = { transform_buffer, draw_indirect_buffer, 0, visualize_points_mesh.vertex_buffer, visualize_points_mesh.index_buffer };
		render_pass->render_object<RenderProcPlacementLayerPipeline>(data);

#else

		uint32 draw_call_offset = 1;

		for (const auto& layer : layers)
		{
			for (uint32 i = 0; i < layer.num_meshes; ++i)
			{
				for (uint32 j = 0; j < (uint32)layer.meshes[i]->submeshes.size(); ++j)
				{
					// TODO: We could pack the vertex and index buffer into the indirect call, but I'm not sure if that fits well with stuff like procedural objects.
					RenderProcPlacementLayerData data = { draw_indirect_buffer, draw_call_offset,
						layer.meshes[i]->mesh.vertex_buffer, layer.meshes[i]->mesh.index_buffer, layer.meshes[i]->submeshes[j].material->albedo };

					render_pass->render_object<RenderProcPlacementLayerPipeline>(data);

					++draw_call_offset;
				}
			}
		}

#endif
	}

}