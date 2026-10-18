// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "terrain/water.h"
#include "ecs/serialization/component_registry.h"
#include "ecs/serialization/serialization_meta.h"

#include "core/string.h"
#include "core/math.h"

#include "dx/dx_buffer.h"
#include "dx/dx_pipeline.h"
#include "dx/dx_profiling.h"

#include "rendering/render_utils.h"
#include "rendering/render_pass.h"
#include "rendering/render_resources.h"

#include "geometry/mesh_builder.h"

#include <rttr/registration>

#include "water_rs.hlsli"
#include "transform.hlsli"

namespace era_engine
{

    static DXPipeline water_pipeline;

    static ref<DXTexture> normalmap1;
    static ref<DXTexture> normalmap2;
    static ref<DXTexture> foam_texture;
    static ref<DXTexture> noise_texture;

    void initialize_water_pipelines()
    {
        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                             .culling_off()
                             .render_targets(transparent_light_pass_formats, arraysize(transparent_light_pass_formats), depth_stencil_format);

            water_pipeline = create_reloadable_pipeline(desc, {"water_vs", "water_ps"});
        }

        normalmap1 = load_texture_from_file_async(get_asset_path("/resources/assets/water/waterNM1.png"), IMAGE_LOAD_FLAGS_NONCOLOR | IMAGE_LOAD_FLAGS_GEN_MIPS_ON_CPU | IMAGE_LOAD_FLAGS_CACHE_TO_DDS);
        normalmap2 = load_texture_from_file_async(get_asset_path("/resources/assets/water/waterNM2.png"), IMAGE_LOAD_FLAGS_NONCOLOR | IMAGE_LOAD_FLAGS_GEN_MIPS_ON_CPU | IMAGE_LOAD_FLAGS_CACHE_TO_DDS);
        foam_texture = load_texture_from_file_async(get_asset_path("/resources/assets/water/waterFoam.dds"), IMAGE_LOAD_FLAGS_NONCOLOR | IMAGE_LOAD_FLAGS_GEN_MIPS_ON_CPU | IMAGE_LOAD_FLAGS_CACHE_TO_DDS);
        noise_texture = load_texture_from_file_async(get_asset_path("/resources/assets/water/waterNoise.dds"), IMAGE_LOAD_FLAGS_NONCOLOR | IMAGE_LOAD_FLAGS_GEN_MIPS_ON_CPU | IMAGE_LOAD_FLAGS_CACHE_TO_DDS);
    }


    struct WaterRenderData
    {
        mat4 m;

        WaterSettings settings;
        float time;
    };

    struct WaterPipeline
    {
        PIPELINE_SETUP_DECL;
        PIPELINE_RENDER_DECL(WaterRenderData);
    };

    PIPELINE_SETUP_IMPL(WaterPipeline)
    {
        cl->set_pipeline_state(*water_pipeline.pipeline);
        cl->set_graphics_root_signature(*water_pipeline.root_signature);

        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        cl->set_graphics_dynamic_constant_buffer(WATER_RS_CAMERA, common.camera_cbv);
        cl->set_graphics_dynamic_constant_buffer(WATER_RS_LIGHTING, common.lighting_cbv);
        cl->set_descriptor_heap_srv(WATER_RS_TEXTURES, 0, common.opaque_color);
        cl->set_descriptor_heap_srv(WATER_RS_TEXTURES, 1, common.opaque_depth);
        cl->set_descriptor_heap_srv(WATER_RS_TEXTURES, 2, normalmap1 ? normalmap1 : RenderResources::default_normal_map);
        cl->set_descriptor_heap_srv(WATER_RS_TEXTURES, 3, normalmap2 ? normalmap2 : RenderResources::default_normal_map);
        cl->set_descriptor_heap_srv(WATER_RS_TEXTURES, 4, foam_texture ? foam_texture : RenderResources::black_texture);
        cl->set_descriptor_heap_srv(WATER_RS_TEXTURES, 5, noise_texture ? noise_texture : RenderResources::black_texture);

        DXCPUDescriptorHandle null_texture = RenderResources::null_texture_srv;

        cl->set_descriptor_heap_srv(WATER_RS_FRAME_CONSTANTS, 0, common.irradiance);
        cl->set_descriptor_heap_srv(WATER_RS_FRAME_CONSTANTS, 1, common.prefiltered_radiance);
        cl->set_descriptor_heap_srv(WATER_RS_FRAME_CONSTANTS, 2, RenderResources::brdf_tex);
        cl->set_descriptor_heap_srv(WATER_RS_FRAME_CONSTANTS, 3, common.shadow_map);
        cl->set_descriptor_heap_srv(WATER_RS_FRAME_CONSTANTS, 4, common.ao_texture ? common.ao_texture : RenderResources::white_texture);
        cl->set_descriptor_heap_srv(WATER_RS_FRAME_CONSTANTS, 5, common.sss_texture ? common.sss_texture : RenderResources::white_texture);
        cl->set_descriptor_heap_srv(WATER_RS_FRAME_CONSTANTS, 6, common.ssr_texture ? common.ssr_texture->default_srv : null_texture);
    }

    PIPELINE_RENDER_IMPL(WaterPipeline, WaterRenderData)
    {
        PROFILE_ALL(cl, "Water");

        WaterCB cb;
        cb.deep_color = data.settings.deep_water_color;
        cb.shallow_color = data.settings.shallow_water_color;
        cb.shallow_depth = data.settings.shallow_depth;
        cb.transition_strength = data.settings.transition_strength;
        cb.uv_offset = normalize(vec2(1.f, 1.f)) * data.time * 0.05f;
        cb.uv_scale = data.settings.uv_scale;
        cb.normalmap_strength = data.settings.normal_strength;

        cl->set_graphics_32bit_constants(WATER_RS_TRANSFORM, TransformCB{view_proj * data.m, data.m});
        cl->set_graphics_32bit_constants(WATER_RS_SETTINGS, cb);
        cl->draw(4, 1, 0, 0);
    }

    RTTR_REGISTRATION
    {
        using namespace rttr;
        rttr::registration::class_<WaterComponent>("WaterComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
            .constructor<>()
            .constructor<ref<Entity::EcsData>, const WaterSettings&>()
            .property("settings", &WaterComponent::settings);
        ComponentRegistry::register_component<WaterComponent>();
    }

    WaterComponent::WaterComponent(ref<Entity::EcsData> _data, const WaterSettings& _settings)
        : Component(_data), settings(_settings)
    {
    }

    WaterComponent::~WaterComponent()
    {
    }

    void WaterComponent::render(const RenderCamera& camera, TransparentRenderPass* render_pass, const vec3& position_offset, const vec2& scale, float dt)
    {
        time += dt;
        render_pass->render_object<WaterPipeline, WaterRenderData>({create_model_matrix(position_offset, quat::identity, vec3(scale.x, 1.f, scale.y)), settings, time});
    }

}