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
	struct ERA_CORE_API TerrainChunk
	{
		ref<DXTexture> heightmap;
		ref<DXTexture> normalmap;

		std::vector<uint16> heights;
	};

	struct ERA_CORE_API TerrainGenerationSettings
	{
		float scale = 0.01f;

		float domain_warp_strength = 1.2f;
		vec2 domain_warp_noise_offset = vec2(0.f, 0.f);
		uint32 domain_warp_octaves = 3;

		vec2 noise_offset = vec2(0.f, 0.f);
		uint32 noise_octaves = 15;
	};

	class ERA_CORE_API TerrainComponent : public Component
	{
	public:
		TerrainComponent() = default;
		TerrainComponent(ref<Entity::EcsData> _data, uint32 _chunks_per_dim, float _chunk_size, float _amplitude_scale,
			ref<PBRMaterial> _ground_material, ref<PBRMaterial> _rock_material, ref<PBRMaterial> _mud_material,
			const TerrainGenerationSettings& _gen_settings = {});
		~TerrainComponent() override;

		void update();
		void render(const RenderCamera& camera, 
			struct OpaqueRenderPass* render_pass, 
			struct SunShadowRenderPass* shadow_pass, 
			struct LDRRenderPass* ldr_pass,
			vec3 position_offset, 
			bool selected = false,
			struct TransformComponent* water_plane_transforms = 0, 
			uint32 num_waters = 0);

		TerrainChunk& chunk(uint32 x, uint32 z) { return chunks[z * chunks_per_dim + x]; }
		const TerrainChunk& chunk(uint32 x, uint32 z) const { return chunks[z * chunks_per_dim + x]; }

		TerrainChunk& chunk(uint32 id) { return chunks[id]; }
		const TerrainChunk& chunk(uint32 id) const { return chunks[id]; }

		vec3 get_min_corner(const vec3& position_offset) const
		{
			float xz_offset = -(chunk_size * chunks_per_dim) * 0.5f; // Offsets entire terrain by half.
			return position_offset + vec3(xz_offset, 0.f, xz_offset);
		}

		ERA_VIRTUAL_REFLECT(Component)

	public:
		uint32 chunks_per_dim;
		float chunk_size;
		float amplitude_scale;

		TerrainGenerationSettings gen_settings;

		ref<PBRMaterial> ground_material;
		ref<PBRMaterial> rock_material;
		ref<PBRMaterial> mud_material;

	private:
		void generate_chunks_cpu();
		void generate_chunks_gpu();

		TerrainGenerationSettings old_gen_settings;

		std::vector<TerrainChunk> chunks;

		friend struct GrassComponent;
	};

	void initialize_terrain_pipelines();
}