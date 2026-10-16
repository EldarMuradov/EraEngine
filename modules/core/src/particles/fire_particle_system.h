// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "particles/particles.h"

#include "dx/dx_texture.h"

#include "rendering/material.h"

#include "fire_particle_system.hlsli"

namespace era_engine
{
	struct ERA_CORE_API FireParticleSystem : ParticleSystem
	{
		static void initialize_pipeline();

		FireParticleSystem() {}
		FireParticleSystem(uint32 max_num_particles, float emit_rate, const std::string& texture_filename, uint32 cols, uint32 rows, vec3 pos);
		void initialize(uint32 max_num_particles, float emit_rate, const std::string& texture_filename, uint32 cols, uint32 rows, vec3 pos);

		virtual void update(struct DXCommandList* cl, const CommonParticleSimulationData& common, float dt) override;
		void render(TransparentRenderPass* render_pass);

		float emit_rate;
		FireParticleSettings settings;

	private:
		DXTextureAtlas atlas;
		TextureAtlasCB atlas_cb;

		vec3 position;

		static DXPipeline emit_pipeline;
		static DXPipeline simulate_pipeline;
		static DXPipeline render_pipeline;

		struct FireMaterial
		{
			DXTextureAtlas atlas;
			DXDynamicConstantBuffer cbv;
		};

		struct FirePipeline : ParticleRenderPipeline<FireMaterial>
		{
			PIPELINE_SETUP_DECL;
			PARTICLE_PIPELINE_RENDER_DECL(FireMaterial);
		};
	};
}