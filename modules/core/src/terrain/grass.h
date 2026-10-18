// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "ecs/component.h"

#include "rendering/render_pass.h"

#include "terrain/terrain.h"

namespace era_engine
{
    struct ERA_CORE_API GrassSettings
    {
        static inline bool depth_prepass = true;

        float blade_height = 1.f;
        float blade_width = 0.05f;
        uint32 num_grass_blades_per_chunk_dim = 350;

        float lod_change_start_distance = 50.f;
        float lod_change_transition_distance = 75.f;

        float cull_start_distance = 350.f;
        float cull_transition_distance = 50.f;
    };

    class ERA_CORE_API GrassComponent final : public Component
    {
    public:
        GrassComponent() = default;
        GrassComponent(ref<Entity::EcsData> _data, const GrassSettings& _settings = {});
        ~GrassComponent() override;

        void generate(struct ComputePass* compute_pass, const RenderCamera& camera, const TerrainComponent& terrain, const vec3& position_offset, float dt);
        void render(struct OpaqueRenderPass* render_pass);

        ERA_VIRTUAL_REFLECT(Component)

    public:
        GrassSettings settings;

    private:
        ref<DXBuffer> draw_buffer;
        ref<DXBuffer> count_buffer;
        ref<DXBuffer> blade_buffer_lod0;
        ref<DXBuffer> blade_buffer_lod1;

        float time = 0.f;
        float prev_time = 0.f;
        vec2 wind_direction = normalize(vec2(1.f, 1.f));
    };

    void initialize_grass_pipelines();

}