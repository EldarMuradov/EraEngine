// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "particles/particles.h"

#include "dx/dx_texture.h"

#include "rendering/material.h"

#include "fire_particle_system.hlsli"

namespace era_engine
{
	struct fire_particle_system : particle_system
	{
		static void initializePipeline();

		fire_particle_system() {}
		fire_particle_system(uint32 maxNumParticles, float emitRate, const std::string& textureFilename, uint32 cols, uint32 rows, vec3 pos);
		void initialize(uint32 maxNumParticles, float emitRate, const std::string& textureFilename, uint32 cols, uint32 rows, vec3 pos);

		virtual void update(struct dx_command_list* cl, const common_particle_simulation_data& common, float dt) override;
		void render(transparent_render_pass* renderPass);

		float emitRate;
		fire_particle_settings settings;

	private:
		dx_texture_atlas atlas;
		texture_atlas_cb atlasCB;

		vec3 position;

		static dx_pipeline emitPipeline;
		static dx_pipeline simulatePipeline;
		static dx_pipeline renderPipeline;

		struct fire_material
		{
			dx_texture_atlas atlas;
			dx_dynamic_constant_buffer cbv;
		};

		struct fire_pipeline : particle_render_pipeline<fire_material>
		{
			PIPELINE_SETUP_DECL;
			PARTICLE_PIPELINE_RENDER_DECL(fire_material);
		};
	};
}