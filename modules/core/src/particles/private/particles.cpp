// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "particles/particles.h"

#include "core/camera.h"

#include "geometry/mesh_builder.h"

#include "dx/dx_context.h"
#include "dx/dx_command_list.h"
#include "dx/dx_barrier_batcher.h"
#include "dx/dx_profiling.h"

#include "rendering/bitonic_sort.h"

namespace era_engine
{
	ref<DXBuffer> ParticleSystem::particle_draw_command_buffer;
	DXCommandSignature ParticleSystem::particle_command_signature;
	DXMesh ParticleSystem::billboard_mesh;

	static DXPipeline start_pipeline;

	static volatile uint32 particle_system_counter = 0;

	void ParticleSystem::initialize_pipeline()
	{
		start_pipeline = create_reloadable_pipeline("particle_start_cs");

		particle_draw_command_buffer = create_buffer(sizeof(ParticleDraw), 1024, 0, true);

		D3D12_INDIRECT_ARGUMENT_DESC argument_desc;
		argument_desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
		particle_command_signature = create_command_signature({}, &argument_desc, 1, sizeof(ParticleDraw));

		MeshBuilder builder(MESH_CREATION_FLAGS_WITH_POSITIONS);
		builder.push_quad({ });
		billboard_mesh = builder.create_dx_mesh();
	}

	void ParticleSystem::initialize_internal(uint32 particle_struct_size, uint32 max_num_particles, SubmeshInfo submesh, SortMode sort_mode)
	{
		this->max_num_particles = max_num_particles;
		this->index = atomic_increment(particle_system_counter);
		this->submesh = submesh;
		this->sort_mode = sort_mode;

		std::vector<uint32> dead(max_num_particles);

		for (uint32 i = 0; i < max_num_particles; ++i)
		{
			dead[i] = i;
		}

		ParticleCounters counters = {};
		counters.num_dead_particles = max_num_particles;

		particles_buffer = create_buffer(particle_struct_size, max_num_particles, 0, true);

		// Lists
		list_buffer = create_buffer(1,
			sizeof(ParticleCounters)
			+ max_num_particles * 3 * sizeof(uint32),
			0, true);
		update_buffer_data_range(list_buffer, &counters, 0, sizeof(ParticleCounters));
		update_buffer_data_range(list_buffer, dead.data(), get_dead_list_offset(), max_num_particles * sizeof(uint32));

		// Dispatch
		dispatch_buffer = create_buffer((uint32)sizeof(ParticleDispatch), 1, 0, true);

		if (sort_mode != SORT_MODE_NONE)
		{
			sort_buffer = create_buffer(sizeof(float), max_num_particles, 0, true);
		}

		ParticleDraw draw = {};
		draw.arguments.BaseVertexLocation = submesh.base_vertex;
		draw.arguments.IndexCountPerInstance = submesh.num_indices;
		draw.arguments.StartIndexLocation = submesh.first_index;
		update_buffer_data_range(particle_draw_command_buffer, &draw, (uint32)sizeof(ParticleDraw) * index, (uint32)sizeof(ParticleDraw));
	}

	void ParticleSystem::initialize_as_billboard(uint32 particle_struct_size, uint32 max_num_particles, SortMode sort_mode)
	{
		SubmeshInfo submesh = { 6, 0, 0, 4 };
		initialize_internal(particle_struct_size, max_num_particles, submesh, sort_mode);
	}

	void ParticleSystem::initialize_as_mesh(uint32 particle_struct_size, DXMesh mesh, SubmeshInfo submesh, uint32 max_num_particles, SortMode sort_mode)
	{
		initialize_internal(particle_struct_size, max_num_particles, submesh, sort_mode);
	}

	ParticleDrawInfo ParticleSystem::get_draw_info(const struct DXPipeline& render_pipeline)
	{
		ParticleDrawInfo result;
		result.particle_buffer = particles_buffer;
		result.alive_list = list_buffer;
		result.alive_list_offset = get_alive_list_offset(current_alive);
		result.command_buffer = particle_draw_command_buffer;
		result.command_buffer_offset = index * particle_draw_command_buffer->element_size;
		result.root_parameter_offset = render_pipeline.root_signature->total_num_parameters - PARTICLE_RENDERING_RS_COUNT;
		return result;
	}

	void ParticleSystem::update_internal(struct DXCommandList* cl, float new_particles, float dt, const DXPipeline& emit_pipeline, const DXPipeline& simulate_pipeline, ParticleParameterSetter* parameter_setter)
	{
		{
			DX_PROFILE_BLOCK(cl, "Particle system update");

			// Buffers get promoted to D3D12_RESOURCE_STATE_UNORDERED_ACCESS implicitly, so we can omit this.
			//cl->transitionBarrier(dispatchBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			ParticleStartCB start_cb = { new_particles, submesh.num_indices, submesh.first_index, submesh.base_vertex };

			// ---------------------------------------
			// START
			// ---------------------------------------
			{
				DX_PROFILE_BLOCK(cl, "Start emit");

				cl->set_pipeline_state(*start_pipeline.pipeline);
				cl->set_compute_root_signature(*start_pipeline.root_signature);

				cl->set_root_compute_uav(PARTICLE_COMPUTE_RS_DISPATCH_INFO, dispatch_buffer);
				set_resources(cl, start_cb, 0, 0);

				cl->dispatch(1);
				BarrierBatcher(cl)
					//.uav(dispatchBuffer)
					.uav(list_buffer)
					.transition(dispatch_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			}

			// ----------------------------------------
			// EMIT
			// ----------------------------------------
			{
				DX_PROFILE_BLOCK(cl, "Emit");

				cl->set_pipeline_state(*emit_pipeline.pipeline);
				cl->set_compute_root_signature(*emit_pipeline.root_signature);

				uint32 num_user_root_parameters = get_num_user_root_parameters(emit_pipeline);
				set_resources(cl, start_cb, num_user_root_parameters, parameter_setter);

				cl->dispatch_indirect(1, dispatch_buffer, 0);
				BarrierBatcher(cl)
					.uav(particle_draw_command_buffer)
					.uav(particles_buffer)
					.uav(list_buffer);
			}

			ParticleSimCB sim_cb = { dt };

			// ----------------------------------------
			// SIMULATE
			// ----------------------------------------
			{
				DX_PROFILE_BLOCK(cl, "Simulate");

				cl->set_pipeline_state(*simulate_pipeline.pipeline);
				cl->set_compute_root_signature(*simulate_pipeline.root_signature);

				uint32 num_user_root_parameters = get_num_user_root_parameters(simulate_pipeline);
				set_resources(cl, sim_cb, num_user_root_parameters, parameter_setter);

				cl->dispatch_indirect(1, dispatch_buffer, sizeof(D3D12_DISPATCH_ARGUMENTS));
				BarrierBatcher(cl)
					.uav(particle_draw_command_buffer)
					.uav(particles_buffer)
					.uav(list_buffer)
					.uav(sort_buffer);
			}

			current_alive = 1 - current_alive;

			// ----------------------------------------
			// SORT
			// ----------------------------------------
			auto draw_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

			if (sort_mode != SORT_MODE_NONE)
			{
				BarrierBatcher(cl)
					.transition(particle_draw_command_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

				draw_state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

				bitonic_sort_float(cl,
					sort_buffer, 0,
					list_buffer, get_alive_list_offset(current_alive),
					max_num_particles,
					particle_draw_command_buffer, sizeof(ParticleDraw) * index + offsetof(ParticleDraw, arguments) + offsetof(D3D12_DRAW_INDEXED_ARGUMENTS, InstanceCount),
					sort_mode == SORT_MODE_FRONT_TO_BACK);
			}

			BarrierBatcher(cl)
				.transition(particle_draw_command_buffer, draw_state, D3D12_RESOURCE_STATE_COMMON);

			// Buffers decay to D3D12_RESOURCE_STATE_COMMON implicitly, so we can omit this.
			//cl->transitionBarrier(dispatchBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COMMON);
		}
	}

	void ParticleSystem::set_resources(DXCommandList* cl, uint32 offset, ParticleParameterSetter* parameter_setter)
	{
		uint32 next_alive = 1 - current_alive;

		cl->set_root_compute_uav(offset + PARTICLE_COMPUTE_RS_DRAW_INFO, particle_draw_command_buffer->gpu_virtual_address + sizeof(ParticleDraw) * index);
		cl->set_root_compute_uav(offset + PARTICLE_COMPUTE_RS_COUNTERS, list_buffer->gpu_virtual_address);
		cl->set_root_compute_uav(offset + PARTICLE_COMPUTE_RS_PARTICLES, particles_buffer);
		cl->set_root_compute_uav(offset + PARTICLE_COMPUTE_RS_DEAD_LIST, list_buffer->gpu_virtual_address + get_dead_list_offset());
		cl->set_root_compute_uav(offset + PARTICLE_COMPUTE_RS_CURRENT_ALIVE, list_buffer->gpu_virtual_address + get_alive_list_offset(current_alive));
		cl->set_root_compute_uav(offset + PARTICLE_COMPUTE_RS_NEW_ALIVE, list_buffer->gpu_virtual_address + get_alive_list_offset(next_alive));

		if (sort_buffer)
		{
			cl->set_root_compute_uav(offset + PARTICLE_COMPUTE_RS_SORT_KEY, sort_buffer);
		}

		if (parameter_setter)
		{
			parameter_setter->set_root_parameters(cl);
		}
	}

	uint32 ParticleSystem::get_alive_list_offset(uint32 alive)
	{
		ASSERT(alive == 0 || alive == 1);

		return get_dead_list_offset() +
			(1 + alive) * max_num_particles * (uint32)sizeof(uint32);
	}

	uint32 ParticleSystem::get_dead_list_offset()
	{
		return (uint32)sizeof(ParticleCounters);
	}

	uint32 ParticleSystem::get_num_user_root_parameters(const DXPipeline& pipeline)
	{
		uint32 num_simulation_root_parameters = PARTICLE_COMPUTE_RS_COUNT;
		uint32 num_user_root_parameters = pipeline.root_signature->total_num_parameters - num_simulation_root_parameters;
		return num_user_root_parameters;
	}
}