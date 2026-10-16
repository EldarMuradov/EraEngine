// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "particles/boid_particle_system.h"

#include "core/string.h"
#include "core/log.h"

#include "dx/dx_pipeline.h"

#include "rendering/render_resources.h"
#include "rendering/render_utils.h"

#include "animation/skinning.h"

namespace era_engine
{
	DXPipeline BoidParticleSystem::emit_pipeline;
	DXPipeline BoidParticleSystem::simulate_pipeline;
	DXPipeline BoidParticleSystem::render_pipeline;

	void BoidParticleSystem::initialize_pipeline()
	{
#define name "boid_particle_system"

		emit_pipeline = create_reloadable_pipeline(EMIT_PIPELINE_NAME(name));
		simulate_pipeline = create_reloadable_pipeline(SIMULATE_PIPELINE_NAME(name));

		auto desc = CREATE_GRAPHICS_PIPELINE
			.input_layout(input_layout_position_uv_normal_tangent)
			.render_targets(transparent_light_pass_formats, arraysize(transparent_light_pass_formats), depth_stencil_format)
			.depth_settings(true, true);

		render_pipeline = create_reloadable_pipeline(desc, { VERTEX_SHADER_NAME(name), PIXEL_SHADER_NAME(name) });

#undef name
	}

	BoidParticleSystem::BoidParticleSystem(uint32 max_num_particles, float emit_rate)
	{
		initialize(max_num_particles, emit_rate);
	}

	void BoidParticleSystem::initialize(uint32 max_num_particles, float emit_rate)
	{
		this->emit_rate = emit_rate;
		cartoon_mesh = import_animated_mesh_from_file(get_asset_path("/resources/assets/cartoon/cartoon.fbx"));
		if (cartoon_mesh)
		{
			ParticleSystem::initialize_as_mesh(sizeof(BoidParticleData), cartoon_mesh->mesh, cartoon_mesh->submeshes[0].info, max_num_particles, SORT_MODE_NONE);

			settings.radius = 15.f;
		}
		else
		{
			LOG_ERROR("Cannot create particle system, since mesh was not found");
			std::cerr << "Cannot create particle system, since mesh was not found\n";
		}
	}

	void BoidParticleSystem::update(struct DXCommandList* cl, const CommonParticleSimulationData& common, float dt)
	{
		//if (cartoonMesh)
		//{
		//	const dx_mesh& mesh = cartoonMesh->mesh;
		//	animation::Skeleton& skeleton = cartoonMesh->skeleton;
		//	animation::AnimationSkeletonImportData& animation_skeleton = cartoonMesh->animation_skeleton;

		//	time += dt;
		//	time = fmod(time, animation_skeleton.clips[0].length_in_seconds);

		//	auto [skinnedVertexBuffer, skinningMatrices] = era_engine::animation::skinObject(mesh.vertexBuffer, cartoonMesh->submeshes[0].info, (uint32)skeleton.joints.size());

		//	trs localTransforms[128];
		//	animation_skeleton.sampleAnimation(0, time, localTransforms);
		//	skeleton.get_skinning_matrices_from_local_transforms(localTransforms, skinningMatrices);

		//	this->skinnedVertexBuffer = skinnedVertexBuffer;
		//	this->submesh.baseVertex = 0;

		//	struct setter : public particle_parameter_setter
		//	{
		//		boid_simulation_cb cb;

		//		virtual void setRootParameters(dx_command_list* cl) override
		//		{
		//			cl->setCompute32BitConstants(BOID_PARTICLE_SYSTEM_COMPUTE_RS_CBV, cb);
		//		}
		//	};

		//	setter s;
		//	s.cb.emitPosition = vec3(-30.f, 20.f, -10.f); // TEMPORARY
		//	s.cb.frameIndex = (uint32)dxContext.frameID;
		//	s.cb.radius = settings.radius;

		//	updateInternal(cl, emitRate * dt, dt, emitPipeline, simulatePipeline, &s);
		//}
	}

	void BoidParticleSystem::render(TransparentRenderPass* render_pass)
	{
		if (cartoon_mesh)
		{
			BoidMaterial material;

			render_pass->render_particles<BoidPipeline>(skinned_vertex_buffer, cartoon_mesh->mesh.index_buffer,
				get_draw_info(render_pipeline),
				material);
		}
	}

	PIPELINE_SETUP_IMPL(BoidParticleSystem::BoidPipeline)
	{
		cl->set_pipeline_state(*render_pipeline.pipeline);
		cl->set_graphics_root_signature(*render_pipeline.root_signature);
		cl->set_graphics_dynamic_constant_buffer(BOID_PARTICLE_SYSTEM_RENDERING_RS_CAMERA, common.camera_cbv);
		cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		cl->set_descriptor_heap_srv(BOID_PARTICLE_SYSTEM_RENDERING_RS_PBR, 0, common.irradiance);
		cl->set_descriptor_heap_srv(BOID_PARTICLE_SYSTEM_RENDERING_RS_PBR, 1, common.prefiltered_radiance);
		cl->set_descriptor_heap_srv(BOID_PARTICLE_SYSTEM_RENDERING_RS_PBR, 2, RenderResources::brdf_tex);
	}

	PARTICLE_PIPELINE_RENDER_IMPL(BoidParticleSystem::BoidPipeline, BoidParticleSystem::BoidMaterial)
	{
		ParticleRenderPipeline::render(cl, view_proj, rc);
	}
}