// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"
#include "core/camera.h"

#include "ecs/component.h"

namespace era_engine
{
    struct ERA_CORE_API WaterSettings
    {
        vec4 deep_water_color = vec4(0.f, 0.241f, 0.799f, 0.76f);
        vec4 shallow_water_color = vec4(0.120f, 0.546f, 0.941f, 0.176f);
        float uv_scale = 0.5f;
        float shallow_depth = 3.f;
        float transition_strength = 0.07f;
        float normal_strength = 1.f;
    };

    class ERA_CORE_API WaterComponent : public Component
    {
    public:
        WaterComponent() = default;
        WaterComponent(ref<Entity::EcsData> _data, const WaterSettings& _settings = WaterSettings{});
        ~WaterComponent() override;

        void render(const RenderCamera& camera, struct TransparentRenderPass* render_pass, const vec3& position_offset, const vec2& scale, float dt);

        ERA_VIRTUAL_REFLECT(Component)

    public:
        WaterSettings settings;

    protected:
        float time = 0.f;
    };

    void initialize_water_pipelines();
}