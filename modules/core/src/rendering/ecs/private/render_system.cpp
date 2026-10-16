#include "rendering/ecs/private/render_system.h"
#include "rendering/ecs/renderer_holder_root_component.h"

#include "core/imgui.h"
#include "core/editor_icons.h"
#include "core/log.h"
#include "core/string.h"
#include "core/cpu_profiling.h"
#include "core/ecs/private/input_system.h"
#include "core/ecs/input_receiver_component.h"

#include "rendering/mesh_shader.h"

#include "ecs/update_groups.h"
#include "ecs/base_components/transform_component.h"
#include "ecs/rendering/world_renderer.h"
#include "ecs/rendering/scene_rendering.h"
#include "core/ecs/camera_holder_component.h"
#include "ecs/rendering/mesh_component.h"
#include "ecs/prefab/prefab_utils.h"

#include "audio/audio.h"

#include "terrain/terrain.h"

#include "animation/skinning.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<RenderSystem>("RenderSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("render")))
			.method("begin_frame", &RenderSystem::begin_frame)(metadata("update_group", update_types::INPUT),
				metadata("Before", std::vector<std::string>{"InputSystem::update"}))
			.method("before_render", &RenderSystem::before_render)(metadata("update_group", update_types::BEFORE_RENDER))
			.method("update", &RenderSystem::update)(metadata("update_group", update_types::RENDER))
			.method("after_render", &RenderSystem::after_render)(metadata("update_group", update_types::AFTER_RENDER));
	}

	RenderSystem::RenderSystem(World* _world)
		: System(_world)
	{
		load_custom_shaders();

		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);

		if (dx_context.feature_support.raytracing())
		{
			raytracing_tlas.initialize();
		}

		renderer_holder_rc->camera.initialize_ingame(vec3::zero, quat::identity, deg2rad(70.f), 0.2f);
		renderer_holder_rc->environment.set_from_texture(get_asset_path("/resources/assets/sky/sunset_in_the_chalk_quarry_4k.hdr"));
		renderer_holder_rc->environment.light_probe_grid.initialize(vec3(-20.f, -1.f, -20.f), vec3(40.f, 20.f, 40.f), 1.5f);

		renderer_holder_rc->sun.direction = normalize(vec3(-0.6f, -1.f, -0.3f));
		renderer_holder_rc->sun.color = vec3(1.f, 0.93f, 0.76f);
		renderer_holder_rc->sun.intensity = 11.1f;
		renderer_holder_rc->sun.num_shadow_cascades = 3;
		renderer_holder_rc->sun.shadow_dimensions = 2048;
		renderer_holder_rc->sun.cascade_distances = vec4(9.f, 25.f, 50.f, 10000.f);
		renderer_holder_rc->sun.bias = vec4(0.0001f, 0.000784f, 0.000524f, 0.0035f);
		renderer_holder_rc->sun.blend_distances = vec4(5.f, 10.f, 10.f, 10.f);
		renderer_holder_rc->sun.stabilize = true;

		for (uint32 i = 0; i < NUM_BUFFERED_FRAMES; ++i)
		{
			point_light_buffer[i] = create_upload_buffer(sizeof(PointLightCB), 512, 0);
			spot_light_buffer[i] = create_upload_buffer(sizeof(SpotLightCB), 512, 0);
			decal_buffer[i] = create_upload_buffer(sizeof(PBRDecalCB), 512, 0);

			spot_light_shadow_info_buffer[i] = create_upload_buffer(sizeof(SpotShadowInfo), 512, 0);
			point_light_shadow_info_buffer[i] = create_upload_buffer(sizeof(PointShadowInfo), 512, 0);

			SET_NAME(point_light_buffer[i]->resource, "Point lights");
			SET_NAME(spot_light_buffer[i]->resource, "Spot lights");
			SET_NAME(decal_buffer[i]->resource, "Decals");

			SET_NAME(spot_light_shadow_info_buffer[i]->resource, "Spot light shadow infos");
			SET_NAME(point_light_shadow_info_buffer[i]->resource, "Point light shadow infos");
		}

		stack_arena.initialize();
	}

	RenderSystem::~RenderSystem()
	{
	}

	void RenderSystem::init()
	{
	}

	void RenderSystem::begin_frame(float dt)
	{
		ZoneScopedN("RenderSystem::begin_frame");

		ImGui::BeginWindowHiddenTabBar("Scene Viewport");
		ImVec2 resolution = ImGui::GetContentRegionAvail();

		if (resolution.x > 0 && resolution.y > 0)
		{
			renderer_holder_rc->width = (uint32)resolution.x;
			renderer_holder_rc->height = (uint32)resolution.y;
		}

		uint32 render_width = renderer_holder_rc->width;
		uint32 render_height = renderer_holder_rc->height;

		MainRenderer* renderer = renderer_holder_rc->renderer;

		ImGui::Image(renderer->frame_result, render_width, render_height);

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EDITOR_ICON_PREFAB))
			{
				const RenderCamera& camera = renderer_holder_rc->camera;

				PrefabInstantiateParams params;
				params.transform = trs(camera.position + camera.rotation * vec3(0.0f, 0.0f, -3.0f));
				PrefabUtils::instantiate_async(world, fs::path(static_cast<const char*>(payload->Data)), params);
			}
			ImGui::EndDragDropTarget();
		}
	}

	void RenderSystem::before_render(float dt)
	{
		ZoneScopedN("RenderSystem::before_render");
		MainRenderer* renderer = renderer_holder_rc->renderer;

		renderer->begin_frame(renderer_holder_rc->width, renderer_holder_rc->height);

		reset_render_passes();

		stack_arena.reset();
	}

	void RenderSystem::update(float dt)
	{
		ZoneScopedN("RenderSystem::update");
		MainRenderer* renderer = renderer_holder_rc->renderer;

		DirectionalLight& sun = renderer_holder_rc->sun;
		PBREnvironment& environment = renderer_holder_rc->environment;

		RenderCamera& camera = renderer_holder_rc->camera;

		environment.update(sun.direction);
		sun.update_matrices(camera);
		set_audio_listener(camera.position, camera.rotation, vec3(0.f));
		environment.light_probe_grid.visualize(&opaque_render_pass);

		{
			for (auto [entity_handle, transform, terrain] : world->group(components_group<TransformComponent, TerrainComponent>).each())
			{
				terrain.update();
			}

			SceneLighting lighting;
			lighting.spot_light_buffer = spot_light_buffer[dx_context.buffered_frame_id];
			lighting.point_light_buffer = point_light_buffer[dx_context.buffered_frame_id];
			lighting.spot_light_shadow_info_buffer = spot_light_shadow_info_buffer[dx_context.buffered_frame_id];
			lighting.point_light_shadow_info_buffer = point_light_shadow_info_buffer[dx_context.buffered_frame_id];
			lighting.spot_shadow_render_passes = spot_shadow_render_passes;
			lighting.point_shadow_render_passes = point_shadow_render_passes;
			lighting.max_num_spot_shadow_render_passes = arraysize(spot_shadow_render_passes);
			lighting.max_num_point_shadow_render_passes = arraysize(point_shadow_render_passes);

			render_world(camera, world, stack_arena, Entity::null_handle, sun, lighting, !renderer->settings.cache_shadow_map,
				&opaque_render_pass, &transparent_render_pass, renderer_holder_rc->ldr_render_pass, &sun_shadow_render_pass, &compute_pass, dt);

			renderer->set_spot_lights(spot_light_buffer[dx_context.buffered_frame_id], world->number_of_components_of_type<SpotLightComponent>(), spot_light_shadow_info_buffer[dx_context.buffered_frame_id]);
			renderer->set_point_lights(point_light_buffer[dx_context.buffered_frame_id], world->number_of_components_of_type<PointLightComponent>(), point_light_shadow_info_buffer[dx_context.buffered_frame_id]);

			if (decals.size())
			{
				update_upload_buffer_data(decal_buffer[dx_context.buffered_frame_id], decals.data(), (uint32)(sizeof(PBRDecalCB) * decals.size()));
				renderer->set_decals(decal_buffer[dx_context.buffered_frame_id], (uint32)decals.size(), decal_texture);
			}

			submit_renderer_params(lighting.num_spot_shadow_render_passes, lighting.num_point_shadow_render_passes);
		}
	}

	void RenderSystem::after_render(float dt)
	{
		ZoneScopedN("RenderSystem::after_render");

		MainRenderer* renderer = renderer_holder_rc->renderer;

		DirectionalLight& sun = renderer_holder_rc->sun;
		PBREnvironment& environment = renderer_holder_rc->environment;

		RenderCamera& camera = renderer_holder_rc->camera;

		animation::perform_skinning(&compute_pass);

		if (dx_context.feature_support.raytracing())
		{
			raytracing_tlas.reset();

			for (auto [entity_handle, transform, raytrace] : world->group(components_group<TransformComponent, RaytraceComponent>).each())
			{
				auto handle = raytracing_tlas.instantiate(raytrace.type, transform.get_world_transform());
			}

			renderer->set_raytracing_scene(&raytracing_tlas);
		}

		renderer->set_environment(environment);
		renderer->set_sun(sun);
		renderer->set_camera(camera);

		end_frame_common();

		const UserInput& input = get_current_frame_input();
		renderer->end_frame(&input);
	}

	void RenderSystem::load_custom_shaders()
	{
		if (dx_context.feature_support.mesh_shaders())
		{
			initialize_mesh_shader();
		}
	}

	void RenderSystem::reset_render_passes()
	{
		opaque_render_pass.reset();
		transparent_render_pass.reset();
		renderer_holder_rc->ldr_render_pass->reset();
		sun_shadow_render_pass.reset();
		compute_pass.reset();

		for (uint32 i = 0; i < arraysize(spot_shadow_render_passes); ++i)
		{
			spot_shadow_render_passes[i].reset();
		}

		for (uint32 i = 0; i < arraysize(point_shadow_render_passes); ++i)
		{
			point_shadow_render_passes[i].reset();
		}
	}

	void RenderSystem::submit_renderer_params(uint32 num_spot_light_shadow_passes, uint32 num_point_light_shadow_passes)
	{
		MainRenderer* renderer = renderer_holder_rc->renderer;

		{
			CPU_PROFILE_BLOCK("Sort render passes");
			ZoneScopedN("RenderSystem::submit_renderer_params");

			opaque_render_pass.sort();
			transparent_render_pass.sort();
			renderer_holder_rc->ldr_render_pass->sort();

			for (uint32 i = 0; i < sun_shadow_render_pass.num_cascades; ++i)
			{
				sun_shadow_render_pass.cascades[i].sort();
			}

			for (uint32 i = 0; i < num_spot_light_shadow_passes; ++i)
			{
				spot_shadow_render_passes[i].sort();
			}

			for (uint32 i = 0; i < num_point_light_shadow_passes; ++i)
			{
				point_shadow_render_passes[i].sort();
			}
		}

		renderer->submit_render_pass(&opaque_render_pass);
		renderer->submit_render_pass(&transparent_render_pass);
		renderer->submit_render_pass(renderer_holder_rc->ldr_render_pass);
		renderer->submit_compute_pass(&compute_pass);

		renderer->submit_shadow_render_pass(&sun_shadow_render_pass);

		for (uint32 i = 0; i < num_spot_light_shadow_passes; ++i)
		{
			renderer->submit_shadow_render_pass(&spot_shadow_render_passes[i]);
		}

		for (uint32 i = 0; i < num_point_light_shadow_passes; ++i)
		{
			renderer->submit_shadow_render_pass(&point_shadow_render_passes[i]);
		}
	}

}