#include "game/tools/ecs_tools_system.h"

#include <engine/engine.h>

#include <core/editor_icons.h>
#include <core/imgui.h>
#include <core/system_info.h>

#include <rendering/ecs/renderer_holder_root_component.h>

#include <ecs/update_groups.h>
#include <ecs/base_components/base_components.h>
#include <ecs/rendering/mesh_component.h>
#include <ecs/editor/entity_editor_utils.h>

#include "geometry/mesh.h"

#include <audio/sound_management.h>

#include "editor/system_calls.h"

#include "rendering/debug_visualization.h"

#include <rttr/policy.h>
#include <rttr/registration>


namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<EditorToolsSystem>("EditorToolsSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("debug")))
			.method("update", &EditorToolsSystem::update)(metadata("update_group", update_types::GAMEPLAY_NORMAL));
	}

	static bool editCamera(RenderCamera& camera)
	{
		bool result = false;
		if (ImGui::BeginTree("Camera"))
		{
			if (ImGui::BeginProperties())
			{
				result |= ImGui::PropertySliderAngle("Field of view", camera.vertical_fov, 1.f, 150.f);

				result |= ImGui::PropertyDrag("Near plane", camera.near_plane, 0.01f, 0.f);

				bool infiniteFarplane = camera.far_plane < 0.f;
				if (ImGui::PropertyCheckbox("Infinite far plane", infiniteFarplane))
				{
					if (!infiniteFarplane)
					{
						camera.far_plane = (camera.far_plane == -1.f) ? 500.f : -camera.far_plane;
					}
					else
					{
						camera.far_plane = -camera.far_plane;
					}
					result = true;
				}
				if (!infiniteFarplane)
				{
					result |= ImGui::PropertyDrag("Far plane", camera.far_plane, 0.1f, 0.f);
				}

				ImGui::EndProperties();
			}

			ImGui::EndTree();
		}
		return result;
	}

	static bool editTonemapping(TonemapSettings& tonemap)
	{
		bool result = false;
		if (ImGui::BeginTree("Tonemapping"))
		{
			ImGui::PlotLines("",
				[](void* data, int idx)
				{
					float t = idx * 0.01f;
					TonemapSettings& aces = *(TonemapSettings*)data;
					return aces.tonemap(t);
				},
				&tonemap, 100, 0, 0, 0.f, 1.f, ImVec2(250.f, 250.f));

			if (ImGui::BeginProperties())
			{
				result |= ImGui::PropertySlider("Shoulder strength", tonemap.A, 0.f, 1.f);
				result |= ImGui::PropertySlider("Linear strength", tonemap.B, 0.f, 1.f);
				result |= ImGui::PropertySlider("Linear angle", tonemap.C, 0.f, 1.f);
				result |= ImGui::PropertySlider("Toe strength", tonemap.D, 0.f, 1.f);
				result |= ImGui::PropertySlider("Tone numerator", tonemap.E, 0.f, 1.f);
				result |= ImGui::PropertySlider("Toe denominator", tonemap.F, 0.f, 1.f);
				result |= ImGui::PropertySlider("Linear white", tonemap.linear_white, 0.f, 100.f);
				result |= ImGui::PropertySlider("Exposure", tonemap.exposure, -3.f, 3.f);

				ImGui::EndProperties();
			}

			ImGui::EndTree();
		}
		return result;
	}

	static bool editSunShadowParameters(DirectionalLight& sun)
	{
		bool result = false;
		if (ImGui::BeginTree("Sun"))
		{
			if (ImGui::BeginProperties())
			{
				result |= ImGui::PropertySlider("Intensity", sun.intensity, 0.f, 1000.f);
				result |= ImGui::PropertyColorWheel("Color", sun.color);

				result |= ImGui::PropertyDropdownPowerOfTwo("Shadow resolution", 128, 2048, sun.shadow_dimensions);

				result |= ImGui::PropertyCheckbox("Stabilize", sun.stabilize);

				result |= ImGui::PropertyDrag("Negative z-offset", sun.negative_z_offset, 0.5f, 0.f);

				result |= ImGui::PropertySlider("Cascade count", sun.num_shadow_cascades, 1, 4);

				const float minCascadeDistance = 0.f, maxCascadeDistance = 300.f;
				const float minBias = 0.f, maxBias = 0.0015f;
				const float minBlend = 0.f, maxBlend = 10.f;
				if (sun.num_shadow_cascades == 1)
				{
					result |= ImGui::PropertySlider("Distance", sun.cascade_distances.x, minCascadeDistance, maxCascadeDistance);

					result |= ImGui::PropertySlider("Bias", sun.bias.x, minBias, maxBias, "%.6f");
					result |= ImGui::PropertySlider("Blend distances", sun.blend_distances.x, minBlend, maxBlend, "%.6f");
				}
				else if (sun.num_shadow_cascades == 2)
				{
					result |= ImGui::PropertySlider("Distance", sun.cascade_distances.xy, minCascadeDistance, maxCascadeDistance);
					result |= ImGui::PropertySlider("Bias", sun.bias.xy, minBias, maxBias, "%.6f");
					result |= ImGui::PropertySlider("Blend distances", sun.blend_distances.xy, minBlend, maxBlend, "%.6f");
				}
				else if (sun.num_shadow_cascades == 3)
				{
					result |= ImGui::PropertySlider("Distance", sun.cascade_distances.xyz, minCascadeDistance, maxCascadeDistance);
					result |= ImGui::PropertySlider("Bias", sun.bias.xyz, minBias, maxBias, "%.6f");
					result |= ImGui::PropertySlider("Blend distances", sun.blend_distances.xyz, minBlend, maxBlend, "%.6f");
				}
				else if (sun.num_shadow_cascades == 4)
				{
					result |= ImGui::PropertySlider("Distance", sun.cascade_distances, minCascadeDistance, maxCascadeDistance);
					result |= ImGui::PropertySlider("Bias", sun.bias, minBias, maxBias, "%.6f");
					result |= ImGui::PropertySlider("Blend distances", sun.blend_distances, minBlend, maxBlend, "%.6f");
				}

				ImGui::EndProperties();
			}

			ImGui::EndTree();
		}
		return result;
	}

	static bool editAO(bool& enable, HBAOSettings& settings, const ref<DXTexture>& aoTexture)
	{
		bool result = false;
		if (ImGui::BeginProperties())
		{
			result |= ImGui::PropertyCheckbox("Enable HBAO", enable);
			if (enable)
			{
				result |= ImGui::PropertySlider("Num rays", settings.num_rays, 1, 16);
				result |= ImGui::PropertySlider("Max num steps per ray", settings.max_num_steps_per_ray, 1, 16);
				result |= ImGui::PropertySlider("Radius", settings.radius, 0.f, 1.f, "%.3fm");
				result |= ImGui::PropertySlider("Strength", settings.strength, 0.f, 2.f);
			}
			ImGui::EndProperties();
		}
		if (enable && aoTexture && ImGui::BeginTree("Show##ShowAO"))
		{
			//ImGui::Image(aoTexture);
			ImGui::EndTree();
		}
		return result;
	}

	static bool editSSS(bool& enable, SSSSettings& settings, const ref<DXTexture>& sssTexture)
	{
		bool result = false;
		if (ImGui::BeginProperties())
		{
			result |= ImGui::PropertyCheckbox("Enable SSS", enable);
			if (enable)
			{
				result |= ImGui::PropertySlider("Num iterations", settings.num_steps, 1, 64);
				result |= ImGui::PropertySlider("Ray distance", settings.ray_distance, 0.05f, 3.f, "%.3fm");
				result |= ImGui::PropertySlider("Thickness", settings.thickness, 0.05f, 1.f, "%.3fm");
				result |= ImGui::PropertySlider("Max distance from camera", settings.max_distance_from_camera, 5.f, 1000.f, "%.3fm");
				result |= ImGui::PropertySlider("Distance fadeout range", settings.distance_fadeout_range, 1.f, 5.f, "%.3fm");
				result |= ImGui::PropertySlider("Border fadeout", settings.border_fadeout, 0.f, 0.5f);
			}
			ImGui::EndProperties();
		}
		if (enable && sssTexture && ImGui::BeginTree("Show##ShowSSS"))
		{
			//ImGui::Image(sssTexture);
			ImGui::EndTree();
		}
		return result;
	}

	static bool editSSR(bool& enable, SSRSettings& settings, const ref<DXTexture>& ssrTexture)
	{
		bool result = false;
		if (ImGui::BeginProperties())
		{
			result |= ImGui::PropertyCheckbox("Enable SSR", enable);
			if (enable)
			{
				result |= ImGui::PropertySlider("Num iterations", settings.num_steps, 1, 1024);
				result |= ImGui::PropertySlider("Max distance", settings.max_distance, 5.f, 1000.f, "%.3fm");
				result |= ImGui::PropertySlider("Min stride", settings.min_stride, 1.f, 50.f, "%.3fm");
				result |= ImGui::PropertySlider("Max stride", settings.max_stride, settings.min_stride, 50.f, "%.3fm");
			}
			ImGui::EndProperties();
		}
		if (enable && ssrTexture && ImGui::BeginTree("Show##ShowSSR"))
		{
			//ImGui::Image(ssrTexture);
			ImGui::EndTree();
		}
		return result;
	}

	static bool editTAA(bool& enable, TAASettings& settings, const ref<DXTexture>& velocityTexture)
	{
		bool result = false;
		if (ImGui::BeginProperties())
		{
			result |= ImGui::PropertyCheckbox("Enable TAA", enable);
			if (enable)
			{
				result |= ImGui::PropertySlider("Jitter strength", settings.camera_jitter_strength);
			}
			ImGui::EndProperties();
		}
		if (enable && velocityTexture && ImGui::BeginTree("Show##ShowTAA"))
		{
			//ImGui::Image(velocityTexture);
			ImGui::EndTree();
		}
		return result;
	}

	static bool editBloom(bool& enable, BloomSettings& settings, const ref<DXTexture>& bloomTexture)
	{
		bool result = false;
		if (ImGui::BeginProperties())
		{
			result |= ImGui::PropertyCheckbox("Enable bloom", enable);
			if (enable)
			{
				result |= ImGui::PropertySlider("Bloom threshold", settings.threshold, 0.5f, 100.f);
				result |= ImGui::PropertySlider("Bloom strength", settings.strength);
			}
			ImGui::EndProperties();
		}
		if (enable && bloomTexture && ImGui::BeginTree("Show##ShowBloom"))
		{
			//ImGui::Image(bloomTexture);
			ImGui::EndTree();
		}
		return result;
	}

	static bool editSharpen(bool& enable, SharpenSettings& settings)
	{
		bool result = false;
		if (ImGui::BeginProperties())
		{
			result |= ImGui::PropertyCheckbox("Enable sharpen", enable);
			if (enable)
			{
				result |= ImGui::PropertySlider("Sharpen strength", settings.strength);
			}
			ImGui::EndProperties();
		}
		return result;
	}

	EditorToolsSystem::EditorToolsSystem(World* _world)
		: System(_world)
	{

	}

	EditorToolsSystem::~EditorToolsSystem()
	{
	}

	void EditorToolsSystem::init()
	{
		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);
	}

	void EditorToolsSystem::update(float dt)
	{
		draw_world();
		draw_creation_popup();

		if (ImGui::Begin("Settings"))
		{
			ImGui::Text("%.3f ms, %u FPS", dt * 1000.f, (uint32)(1.f / dt));

			if (ImGui::BeginProperties())
			{
				ImGui::PropertySlider("Time scale", renderer_holder_rc->timestep_scale);

				DXMemoryUsage memoryUsage = get_dx_mem_usage();

				ImGui::PropertyValue("Video memory usage", "%u / %uMB", memoryUsage.currently_used, memoryUsage.available);

				ImGui::EndProperties();
			}
		}
		ImGui::End();
	}

	void EditorToolsSystem::draw_creation_popup()
	{
		bool clicked = false;

		RenderCamera& camera = renderer_holder_rc->camera;

		if (ImGui::BeginPopup("CreateEntityPopup"))
		{
			if (ImGui::MenuItem("Point light", "P") || ImGui::IsKeyPressed('P'))
			{
				Entity pl = world->create_entity("Point light");
				pl.add_component<PointLightComponent>(
					vec3(1.f, 1.f, 1.f),
					1.f,
					10.f,
					true,
					2048u
				);

				TransformComponent* transform = pl.get_component<TransformComponent>();
				transform->set_world_position(camera.position + camera.rotation * vec3(0.f, 0.f, -3.f));

				set_selected_entity(pl);
				clicked = true;
			}

			if (ImGui::MenuItem("Spot light", "S") || ImGui::IsKeyPressed('S'))
			{
				Entity sl = world->create_entity("Spot light");
				sl.add_component<SpotLightComponent>(
					vec3(1.f, 1.f, 1.f),
					1.f,
					25.f,
					deg2rad(20.f),
					deg2rad(30.f),
					true,
					2048u
				);

				TransformComponent* transform = sl.get_component<TransformComponent>();
				transform->set_world_position(camera.position + camera.rotation * vec3(0.f, 0.f, -3.f));

				set_selected_entity(sl);
				clicked = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Empty", "E") || ImGui::IsKeyPressed('E'))
			{
				auto empty = world->create_entity("Empty");
				TransformComponent* transform = empty.get_component<TransformComponent>();
				transform->set_world_position(camera.position + camera.rotation * vec3(0.f, 0.f, -3.f));

				set_selected_entity(empty);
				clicked = true;
			}

			if (clicked)
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void EditorToolsSystem::draw_world()
	{
		draw_settings();
		draw_main_menu();

		bool objectMovedByWidget = false;

		if (ImGui::Begin("Scene Hierarchy"))
		{
			if (ImGui::BeginChild("Outliner"))
			{
				world->view<NameComponent>()
					.each([this](Entity::Handle entityHandle, NameComponent& tag)
						{
							ImGui::PushID((uint32)entityHandle);
							const char* name = tag.name;
							Entity entity = world->get_entity(entityHandle);

							Entity parent = world->get_entity(entity.get_parent_handle());

							if (parent.is_valid())
							{
								ImGuiTreeNodeFlags flags = ((selected_entity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
								if (ImGui::TreeNodeEx((void*)(uint32)entityHandle, flags, name))
								{
									if (ImGui::IsItemClicked())
									{
										set_selected_entity(entity);
									}

									draw_childs(entity);

									ImGui::TreePop();
								}
							}

							bool entityDeleted = false;
							if (ImGui::BeginPopupContextItem(name))
							{
								if (ImGui::MenuItem("Delete"))
								{
									entityDeleted = true;
								}

								ImGui::EndPopup();
							}

							if (entityDeleted)
							{
								if (entity == selected_entity)
								{
									set_selected_entity({});
								}
								world->destroy_entity(entity);
							}
							ImGui::PopID();
						});
			}
			ImGui::EndChild();

			if (selected_entity.is_valid())
			{
				if (ImGui::Begin("Components"))
				{
					ImGui::AlignTextToFramePadding();

					ImGui::PushID((uint32)selected_entity.get_handle());

					{
						NameComponent* name_component = selected_entity.get_component_if_exists<NameComponent>();

						NameComponent tagBefore = *name_component;
						ImGui::InputText("Name", name_component->name, sizeof(NameComponent::name));
					}

					ImGui::PopID();

					ImGui::SameLine();
					if (ImGui::Button(ICON_FA_TRASH_ALT))
					{
						world->destroy_entity(selected_entity);
						set_selected_entity({});
						objectMovedByWidget = true;
					}

					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Delete entity");
					}

					if (selected_entity.is_valid())
					{
						EntityEditorUtils::edit_entity(world, selected_entity.get_handle());
					}

					if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered())
					{
						ImGui::OpenPopup("CreateComponentPopup");
					}

					if (ImGui::BeginPopup("CreateComponentPopup"))
					{
						ImGui::Text("Create component");
						ImGui::Separator();

						ImGui::EndPopup();
					}
				}
				ImGui::End();
			}
		}
		ImGui::End();
	}

	void EditorToolsSystem::draw_main_menu()
	{
		bool controlsClicked = false;
		bool aboutClicked = false;
		bool systemClicked = false;

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu(ICON_FA_FILE "  Project"))
			{
				if (ImGui::MenuItem(ICON_FA_TIMES "  Exit", "Esc"))
				{
					PostQuitMessage(0);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(ICON_FA_TOOLS "  Tools"))
			{
				if (ImGui::MenuItem(ICON_FA_DESKTOP "  System"))
				{
					systemClicked = true;
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(ICON_FA_QUESTION "  Help"))
			{
				// TODO: Docs
				if (ImGui::MenuItem(ICON_FA_MEMORY "  Repository"))
					os::SystemCalls::open_url("https://github.com/EldarMuradau/EraEngine");

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(ICON_FA_QUESTION "  About"))
			{
				if (ImGui::MenuItem(ICON_FA_QUESTION "  Author"))
				{
					aboutClicked = true;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();

		if (systemClicked)
		{
			ImGui::OpenPopup(ICON_FA_DESKTOP "  System");
		}

		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal(ICON_FA_DESKTOP "  System", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			SystemInfo info = get_system_info();
			ImGui::Value("CPU", info.cpu_name.c_str());
			ImGui::Value("GPU", info.gpu_name.c_str());

			ImGui::Separator();

			ImGui::PopupOkButton();
			ImGui::EndPopup();
		}

		if (aboutClicked)
		{
			ImGui::OpenPopup(ICON_FA_QUESTION "  About");
		}

		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal(ICON_FA_QUESTION "  About", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Dummy(ImVec2(300.f, 1.f));
			ImGui::CenteredText("Era Engine");
			ImGui::CenteredText("by Eldar Muradov");

			ImGui::Separator();

			ImGui::PopupOkButton();
			ImGui::EndPopup();
		}
	}

	void EditorToolsSystem::draw_settings()
	{
		if (ImGui::Begin("Render"))
		{
			MainRenderer* renderer = renderer_holder_rc->get_renderer();
			PathTracer& pathTracer = renderer->path_tracer;

			if (ImGui::BeginProperties())
			{
				//if (ImGui::PropertyDropdown("Renderer mode", rendererModeNames, renderer_mode_count, (uint32&)renderer->mode))
				//{
				//	pathTracer.resetRendering();
				//}

				ImGui::PropertyDropdown("Aspect ratio", aspect_ratio_names, ASPECT_RATIO_MODE_COUNT, (uint32&)renderer->aspect_ratio_mode);

				ImGui::PropertyCheckbox("Shadow map caching", renderer->settings.cache_shadow_map);

				ImGui::EndProperties();
			}

			editCamera(renderer_holder_rc->camera);
			editTonemapping(renderer->settings.tonemap_settings);
			editSunShadowParameters(renderer_holder_rc->sun);

			if (ImGui::BeginTree("Post processing"))
			{
				if (renderer->spec.allow_ao) { editAO(renderer->settings.enable_ao, renderer->settings.ao_settings, renderer->get_ao_result()); ImGui::Separator(); }
				if (renderer->spec.allow_sss) { editSSS(renderer->settings.enable_sss, renderer->settings.sss_settings, renderer->get_sss_result()); ImGui::Separator(); }
				if (renderer->spec.allow_ssr) { editSSR(renderer->settings.enable_ssr, renderer->settings.ssr_settings, renderer->get_ssr_result()); ImGui::Separator(); }
				if (renderer->spec.allow_taa) { editTAA(renderer->settings.enable_taa, renderer->settings.taa_settings, renderer->get_screen_velocities()); ImGui::Separator(); }
				if (renderer->spec.allow_bloom) { editBloom(renderer->settings.enable_bloom, renderer->settings.bloom_settings, renderer->get_bloom_result()); ImGui::Separator(); }

				editSharpen(renderer->settings.enable_sharpen, renderer->settings.sharpen_settings);

				ImGui::EndTree();
			}

			if (ImGui::BeginTree("Environment"))
			{
				auto& environment = renderer_holder_rc->environment;

				if (ImGui::BeginProperties())
				{
					AssetHandle handle = (environment.sky) ? environment.sky->handle : AssetHandle{};
					if (ImGui::PropertyAssetHandle("Texture source", EDITOR_ICON_IMAGE_HDR, handle, "Make proc"))
					{
						if (handle)
						{
							environment.set_from_texture(get_path_from_asset_handle(handle));
						}
						else
						{
							environment.set_to_procedural(renderer_holder_rc->sun.direction);
						}
					}

					ImGui::PropertySlider("Sky intensity", environment.sky_intensity, 0.f, 2.f);
					ImGui::PropertySlider("GI intensity", environment.global_illumination_intensity, 0.f, 2.f);

					EnvironmentGIMode tempMode = environment.gi_mode;

					ImGui::PropertyDropdown("GI mode", environment_gi_mode_names, 1 + 1, (uint32&)tempMode);

					if (tempMode != environment.gi_mode)
					{
						struct ChangeGIModeData { EnvironmentGIMode mode; EnvironmentGIMode& destMode; } data{ tempMode, renderer_holder_rc->environment.gi_mode };

						DefaultJob job_object;

						job_object.data.user_data = &data;
						job_object.data.data_size = sizeof(ChangeGIModeData);
						job_object.data.destroy_after_execution = false;

						job_object.job_func = [](DefaultJob::Data& default_data) {
							ChangeGIModeData* data = reinterpret_cast<ChangeGIModeData*>(default_data.user_data);

							data->destMode = data->mode;
							};

						schedule_low_priority_job(job_object).submit_now();
					}

					ImGui::EndProperties();
				}

				if (environment.gi_mode == ENVIRONMENT_GI_RAYTRACED)
				{
					if (ImGui::BeginTree("Light probe"))
					{
						auto& grid = environment.light_probe_grid;

						if (ImGui::BeginProperties())
						{
							ImGui::PropertyCheckbox("Visualize probes", grid.visualize_probes);
							ImGui::PropertyCheckbox("Visualize rays", grid.visualize_rays);

							ImGui::PropertyCheckbox("Auto rotate rays", grid.auto_rotate_rays);
							if (!grid.auto_rotate_rays)
							{
								grid.rotate_rays = ImGui::PropertyButton("Rotate", "Go");
							}

							ImGui::EndProperties();
						}

						if (ImGui::BeginTree("Irradiance"))
						{
							if (ImGui::BeginProperties())
							{
								ImGui::PropertySlider("Scale", grid.irradiance_ui_scale, 0.1f, 20.f);
								ImGui::EndProperties();
							}
							//if (grid.irradiance)
							//{
							//	ImGui::Image(grid.irradiance, (uint32)(grid.irradiance->width * grid.irradianceUIScale));
							//}
							ImGui::EndTree();
						}
						if (ImGui::BeginTree("Depth"))
						{
							if (ImGui::BeginProperties())
							{
								ImGui::PropertySlider("Scale", grid.depth_ui_scale, 0.1f, 20.f);
								ImGui::EndProperties();
							}
							//if (grid.depth)
							//{
							//	ImGui::Image(grid.depth, (uint32)(grid.depth->width * grid.depthUIScale));
							//}
							ImGui::EndTree();
						}
						if (ImGui::BeginTree("Raytraced radiance"))
						{
							if (ImGui::BeginProperties())
							{
								ImGui::PropertySlider("Scale", grid.raytraced_radiance_ui_scale, 0.1f, 20.f);
								ImGui::EndProperties();
							}
							//if (grid.raytracedRadiance)
							//{
							//	ImGui::Image(grid.raytracedRadiance, (uint32)(grid.raytracedRadiance->width * grid.raytracedRadianceUIScale));
							//}
							ImGui::EndTree();
						}

						ImGui::EndTree();
					}
				}
				ImGui::EndTree();
			}

			if (renderer->mode == RENDERER_MODE_PATHTRACED)
			{
				if (ImGui::BeginProperties())
				{
					auto& settings = pathTracer.settings;

					ImGui::PropertySlider("Max recursion depth", settings.recursion_depth, 0, settings.max_recursion_depth - 1);
					ImGui::PropertySlider("Start russian roulette after", settings.start_russian_roulette_after, 0, settings.recursion_depth);

					ImGui::PropertyCheckbox("Use thin lens camera", settings.use_thin_lens_camera);
					if (settings.use_thin_lens_camera)
					{
						ImGui::PropertySlider("Focal length", settings.focal_length, 0.5f, 50.f);
						ImGui::PropertySlider("F-Number", settings.f_number, 1.f, 128.f);
					}
					ImGui::PropertyCheckbox("Use real materials", settings.use_real_materials);
					ImGui::PropertyCheckbox("Enable direct lighting", settings.enable_direct_lighting);
					if (settings.enable_direct_lighting)
					{
						ImGui::PropertySlider("Light intensity scale", settings.light_intensity_scale, 0.f, 50.f);
						ImGui::PropertySlider("Point light radius", settings.point_light_radius, 0.01f, 1.f);

						ImGui::PropertyCheckbox("Multiple importance sampling", settings.multiple_importance_sampling);
					}

					ImGui::EndProperties();
				}
			}
		}

		ImGui::End();
	}

	void EditorToolsSystem::draw_childs(const Entity& entity)
	{
		static int i = 0;

		auto childs = EntityContainer::get_childs(world, entity.get_handle());

		if (!childs.empty())
		{
			auto iter = childs.begin();
			const auto& end = childs.end();
			for (; iter != end; ++iter)
			{
				Entity child = world->get_entity(*iter);
				NameComponent* name_component = child.get_component_if_exists<NameComponent>();
				ImGuiTreeNodeFlags flags = ((selected_entity == *iter) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
				if (name_component)
				{
					if (ImGui::TreeNodeEx((void*)(uint32)(*iter), flags, name_component->name))
					{
						if (ImGui::IsItemClicked())
						{
							set_selected_entity(child);
						}

						draw_childs(child);

						ImGui::TreePop();
					}
				}
				else
				{
					if (ImGui::TreeNodeEx((void*)(uint32)(*iter), flags, std::to_string(i++).c_str()))
					{
						if (ImGui::IsItemClicked())
						{
							set_selected_entity(child);
						}

						draw_childs(child);

						ImGui::TreePop();
					}
				}
			}
		}
	}

	void EditorToolsSystem::update_selected_entity_ui_rotation()
	{
		if (selected_entity.is_valid())
		{
			quat rotation = quat::identity;

			if (const TransformComponent* transform = selected_entity.get_component_if_exists<TransformComponent>())
			{
				rotation = transform->get_world_rotation();
			}

			selected_entity_euler_rotation = quat_to_euler(rotation);
		}
	}

	void EditorToolsSystem::set_selected_entity(const Entity& entity)
	{
		selected_entity = entity;
		update_selected_entity_ui_rotation();
	}

}