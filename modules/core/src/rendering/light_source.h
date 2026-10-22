// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"
#include "core/camera.h"
#include "core/reflect.h"

#include "ecs/component.h"

#include "light_source.hlsli"

namespace era_engine
{
    class ERA_CORE_API DirectionalLight
    {
    public:
        DirectionalLight() = default;
        DirectionalLight(const DirectionalLight& other) noexcept = default;
        DirectionalLight(DirectionalLight&& other) noexcept = default;
        DirectionalLight& operator=(const DirectionalLight& other) noexcept = default;
        DirectionalLight& operator=(DirectionalLight&& other) noexcept = default;

        void update_matrices(const RenderCamera& camera);

        vec4 cascade_distances;
        vec4 bias;
        vec4 depth_bias;
        vec4 blend_distances;
        vec4 shadow_map_viewports[MAX_NUM_SUN_SHADOW_CASCADES];

        mat4 view_projs[MAX_NUM_SUN_SHADOW_CASCADES];

        vec3 color;
        vec3 direction;

        float intensity;
        float negative_z_offset = 750.0f;

        uint32 shadow_dimensions = 2048;
        uint32 num_shadow_cascades;

        bool stabilize;

        ERA_REFLECT
    };

    class ERA_CORE_API PointLightComponent final : public Component
    {
    public:
        PointLightComponent() = default;
        PointLightComponent(ref<Entity::EcsData> _data);
        PointLightComponent(ref<Entity::EcsData> _data, const vec3& _color, float _intensity, float _radius, bool _casts_shadow = false, uint32 _shadow_map_resolution = 2048);
        PointLightComponent(const PointLightComponent&) = default;
        virtual ~PointLightComponent();

        ERA_VIRTUAL_REFLECT(Component)
    public:
        vec3 color = vec3(1.0f);

        float intensity = 1.0f;
        float radius = 10.0f;

        uint32 shadow_map_resolution = 2048;

        bool casts_shadow = false;
    };

    class ERA_CORE_API SpotLightComponent final : public Component
    {
    public:
        SpotLightComponent() = default;
        SpotLightComponent(ref<Entity::EcsData> _data);
        SpotLightComponent(ref<Entity::EcsData> _data, const vec3& _color, float _intensity, float _distance, float _inner_angle, float _outer_angle, bool _casts_shadow = false, uint32 _shadow_map_resolution = 2048);
        SpotLightComponent(const SpotLightComponent&) = default;
        virtual ~SpotLightComponent();

        ERA_VIRTUAL_REFLECT(Component)

    public:
        vec3 color = vec3(1.0f);

        float intensity = 1.0f;
        float distance = 10.0f;
        float inner_angle = deg2rad(20.0f);
        float outer_angle = deg2rad(30.0f);

        uint32 shadow_map_resolution = 2048;

        bool casts_shadow = false;
    };

    ERA_CORE_API mat4 get_spot_light_view_projection_matrix(const SpotLightCB& sl);
}