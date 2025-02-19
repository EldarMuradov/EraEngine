// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"
#include "core/camera.h"

#include "ecs/component.h"

#include "dx/dx_texture.h"

#include "rendering/material.h"
#include "rendering/render_command.h"

namespace era_engine
{
	struct ERA_CORE_API terrain_chunk
	{
		ref<dx_texture> heightmap;
		ref<dx_texture> normalmap;

		std::vector<uint16> heights;
	};

	struct ERA_CORE_API terrain_generation_settings
	{
		float scale = 0.01f;

		float domainWarpStrength = 1.2f;
		vec2 domainWarpNoiseOffset = vec2(0.f, 0.f);
		uint32 domainWarpOctaves = 3;

		vec2 noiseOffset = vec2(0.f, 0.f);
		uint32 noiseOctaves = 15;
	};

	class ERA_CORE_API TerrainComponent : public Component
	{
	public:
		TerrainComponent() = default;
		TerrainComponent(ref<Entity::EcsData> _data, uint32 _chunks_per_dim, float _chunk_size, float _amplitude_scale,
			ref<pbr_material> ground_material, ref<pbr_material> rock_material, ref<pbr_material> _mud_material,
			const terrain_generation_settings& _gen_settings = {});
		virtual ~TerrainComponent();

		void update();
		void render(const render_camera& camera, struct opaque_render_pass* renderPass, struct sun_shadow_render_pass* shadowPass, struct ldr_render_pass* ldrPass,
			vec3 positionOffset, bool selected = false,
			struct TransformComponent* waterPlaneTransforms = 0, uint32 numWaters = 0);

		terrain_chunk& chunk(uint32 x, uint32 z) { return chunks[z * chunksPerDim + x]; }
		const terrain_chunk& chunk(uint32 x, uint32 z) const { return chunks[z * chunksPerDim + x]; }

		vec3 get_min_corner(vec3 positionOffset) const
		{
			float xzOffset = -(chunkSize * chunksPerDim) * 0.5f; // Offsets entire terrain by half.
			return positionOffset + vec3(xzOffset, 0.f, xzOffset);
		}

		ERA_VIRTUAL_REFLECT(Component)

	public:
		uint32 chunksPerDim;
		float chunkSize;
		float amplitudeScale;

		terrain_generation_settings genSettings;

		ref<pbr_material> groundMaterial;
		ref<pbr_material> rockMaterial;
		ref<pbr_material> mudMaterial;

	private:
		void generate_chunks_CPU();
		void generate_chunks_GPU();

		terrain_generation_settings oldGenSettings;

		std::vector<terrain_chunk> chunks;

		friend struct GrassComponent;
	};

	void initializeTerrainPipelines();
}