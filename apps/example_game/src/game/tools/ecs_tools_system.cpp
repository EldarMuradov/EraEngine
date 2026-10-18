#include "game/tools/ecs_tools_system.h"

#include <engine/engine.h>

#include <core/editor_icons.h>
#include <core/imgui.h>
#include <core/system_info.h>

#include <rendering/ecs/renderer_holder_root_component.h>

#include <ecs/update_groups.h>
#include <ecs/base_components/base_components.h>
#include <ecs/rendering/mesh_component.h>
#include <ecs/editor/scene_editor_panels.h>

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
            ImGui::PlotLines(
                "",
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
            // ImGui::Image(aoTexture);
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
            // ImGui::Image(sssTexture);
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
            // ImGui::Image(ssrTexture);
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
            // ImGui::Image(velocityTexture);
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
            // ImGui::Image(bloomTexture);
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

        scene_panels = std::make_unique<SceneEditorPanels>(world);
    }

    void EditorToolsSystem::update(float dt)
    {
        draw_world();

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

    void EditorToolsSystem::draw_world()
    {
        draw_settings();
        draw_main_menu();

        scene_panels->draw();
    }

    void EditorToolsSystem::draw_main_menu()
    {
        bool controlsClicked = false;
        bool aboutClicked = false;
        bool systemClicked = false;
        bool instantiate_prefab_clicked = false;
        bool save_prefab_clicked = false;

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu(ICON_FA_FILE "  Project"))
            {
                if (ImGui::MenuItem(EDITOR_ICON_PREFAB "  Instantiate prefab..."))
                {
                    instantiate_prefab_clicked = true;
                }
                if (ImGui::MenuItem(EDITOR_ICON_PREFAB "  Save selection as prefab...", nullptr, false, scene_panels->get_selected_entity().is_valid()))
                {
                    save_prefab_clicked = true;
                }

                ImGui::Separator();

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

        if (instantiate_prefab_clicked)
        {
            scene_panels->instantiate_prefab_with_dialog();
        }

        if (save_prefab_clicked)
        {
            scene_panels->save_as_prefab_with_dialog(scene_panels->get_selected_entity());
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
                // if (ImGui::PropertyDropdown("Renderer mode", rendererModeNames, renderer_mode_count, (uint32&)renderer->mode))
                //{
                //	pathTracer.resetRendering();
                // }

                ImGui::PropertyDropdown("Aspect ratio", aspect_ratio_names, ASPECT_RATIO_MODE_COUNT, (uint32&)renderer->aspect_ratio_mode);

                ImGui::PropertyCheckbox("Shadow map caching", renderer->settings.cache_shadow_map);

                ImGui::EndProperties();
            }

            editCamera(renderer_holder_rc->camera);
            editTonemapping(renderer->settings.tonemap_settings);
            editSunShadowParameters(renderer_holder_rc->sun);

            if (ImGui::BeginTree("Post processing"))
            {
                if (renderer->spec.allow_ao)
                {
                    editAO(renderer->settings.enable_ao, renderer->settings.ao_settings, renderer->get_ao_result());
                    ImGui::Separator();
                }
                if (renderer->spec.allow_sss)
                {
                    editSSS(renderer->settings.enable_sss, renderer->settings.sss_settings, renderer->get_sss_result());
                    ImGui::Separator();
                }
                if (renderer->spec.allow_ssr)
                {
                    editSSR(renderer->settings.enable_ssr, renderer->settings.ssr_settings, renderer->get_ssr_result());
                    ImGui::Separator();
                }
                if (renderer->spec.allow_taa)
                {
                    editTAA(renderer->settings.enable_taa, renderer->settings.taa_settings, renderer->get_screen_velocities());
                    ImGui::Separator();
                }
                if (renderer->spec.allow_bloom)
                {
                    editBloom(renderer->settings.enable_bloom, renderer->settings.bloom_settings, renderer->get_bloom_result());
                    ImGui::Separator();
                }

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

                    EnvironmentGIMode temp_mode = environment.gi_mode;

                    ImGui::PropertyDropdown("GI mode", environment_gi_mode_names, 1 + 1, (uint32&)temp_mode);

                    if (temp_mode != environment.gi_mode)
                    {
                        DefaultJob job_object;

                        job_object.job_func = [mode = temp_mode, dest_mode = &renderer_holder_rc->environment.gi_mode]()
                        {
                            *dest_mode = mode;
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
                            // if (grid.irradiance)
                            //{
                            //	ImGui::Image(grid.irradiance, (uint32)(grid.irradiance->width * grid.irradianceUIScale));
                            // }
                            ImGui::EndTree();
                        }
                        if (ImGui::BeginTree("Depth"))
                        {
                            if (ImGui::BeginProperties())
                            {
                                ImGui::PropertySlider("Scale", grid.depth_ui_scale, 0.1f, 20.f);
                                ImGui::EndProperties();
                            }
                            // if (grid.depth)
                            //{
                            //	ImGui::Image(grid.depth, (uint32)(grid.depth->width * grid.depthUIScale));
                            // }
                            ImGui::EndTree();
                        }
                        if (ImGui::BeginTree("Raytraced radiance"))
                        {
                            if (ImGui::BeginProperties())
                            {
                                ImGui::PropertySlider("Scale", grid.raytraced_radiance_ui_scale, 0.1f, 20.f);
                                ImGui::EndProperties();
                            }
                            // if (grid.raytracedRadiance)
                            //{
                            //	ImGui::Image(grid.raytracedRadiance, (uint32)(grid.raytracedRadiance->width * grid.raytracedRadianceUIScale));
                            // }
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
}