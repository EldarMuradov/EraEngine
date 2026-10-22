// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/light_probe.h"
#include "rendering/render_utils.h"
#include "rendering/render_resources.h"
#include "rendering/pbr_raytracer.h"

#include "core/imgui.h"

#include "dx/dx_pipeline.h"
#include "dx/dx_command_list.h"
#include "dx/dx_context.h"
#include "dx/dx_barrier_batcher.h"
#include "dx/dx_profiling.h"

#include "geometry/mesh_builder.h"

#include "light_probe_rs.hlsli"
#include "transform.hlsli"

namespace era_engine
{
    static DXPipeline visualize_grid_pipeline;
    static DXPipeline visualize_rays_pipeline;

    static DXPipeline probe_update_irradiance_pipeline;
    static DXPipeline probe_update_depth_pipeline;

    static DXPipeline test_sample_pipeline;

    static DXMesh sphere_mesh;
    static SubmeshInfo sphere_submesh;

    static const DXGI_FORMAT irradiance_format = DXGI_FORMAT_R16G16B16A16_FLOAT; // DXGI_FORMAT_R11G11B10_FLOAT;
    static const DXGI_FORMAT depth_format = DXGI_FORMAT_R16G16_FLOAT;
    static const DXGI_FORMAT raytraced_radiance_format = DXGI_FORMAT_R11G11B10_FLOAT;
    static const DXGI_FORMAT raytraced_direction_and_distance_format = DXGI_FORMAT_R16G16B16A16_FLOAT;

    struct LightProbeTracer : PBRRaytracer
    {
        void initialize();

        void render(DXCommandList* cl, const RaytracingTLAS& tlas,
            const LightProbeGrid& grid,
            const CommonRenderData& common);

    private:
        const uint32 max_recursion_depth = 2;
        const uint32 max_payload_size = 4 * sizeof(float); // Radiance-payload is 1 x float3, 1 x float

        // Only descriptors in here!
        struct InputResources
        {
            DXCPUDescriptorHandle tlas;
            DXCPUDescriptorHandle sky;
            DXCPUDescriptorHandle irradiance;
            DXCPUDescriptorHandle depth;
        };

        struct OutputResources
        {
            DXCPUDescriptorHandle radiance;
            DXCPUDescriptorHandle direction_and_distance;
        };
    };

    static LightProbeTracer light_probe_tracer;

    void initialize_light_probe_pipelines()
    {
        if (!dx_context.feature_support.raytracing())
            return;

        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                             .input_layout(input_layout_position)
                             .render_targets(hdr_format, depth_stencil_format);

            visualize_grid_pipeline = create_reloadable_pipeline(desc, {"light_probe_grid_visualization_vs", "light_probe_grid_visualization_ps"});
        }

        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                             .primitive_topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE)
                             .render_targets(hdr_format, depth_stencil_format);

            visualize_rays_pipeline = create_reloadable_pipeline(desc, {"light_probe_ray_visualization_vs", "light_probe_ray_visualization_ps"});
        }

        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                             .input_layout(input_layout_position)
                             .render_targets(hdr_format, depth_stencil_format);

            test_sample_pipeline = create_reloadable_pipeline(desc, {"light_probe_test_sample_vs", "light_probe_test_sample_ps"});
        }

        MeshBuilder builder(MESH_CREATION_FLAGS_WITH_POSITIONS);
        builder.push_sphere({});
        sphere_submesh = builder.end_submesh();
        sphere_mesh = builder.create_dx_mesh();

        probe_update_irradiance_pipeline = create_reloadable_pipeline("light_probe_update_irradiance_cs");
        probe_update_depth_pipeline = create_reloadable_pipeline("light_probe_update_depth_cs");

        light_probe_tracer.initialize();
    }

    void LightProbeGrid::initialize(vec3 min_corner, vec3 dimensions, float cell_size)
    {
        if (!dx_context.feature_support.raytracing())
            return;

        uint32 grid_size_x = (uint32)(ceilf(dimensions.x / cell_size));
        uint32 grid_size_y = (uint32)(ceilf(dimensions.y / cell_size));
        uint32 grid_size_z = (uint32)(ceilf(dimensions.z / cell_size));

        vec3 old_dimensions = dimensions;

        dimensions = vec3((float)grid_size_x, (float)grid_size_y, (float)grid_size_z) * cell_size;
        min_corner -= (dimensions - old_dimensions) * 0.5f;

        this->min_corner = min_corner;
        this->cell_size = cell_size;

        this->num_nodes_x = grid_size_x + 1;
        this->num_nodes_y = grid_size_y + 1;
        this->num_nodes_z = grid_size_z + 1;
        this->total_num_nodes = num_nodes_x * num_nodes_y * num_nodes_z;

        uint32 irradiance_y_slice_width = num_nodes_x * LIGHT_PROBE_TOTAL_RESOLUTION;
        uint32 irradiance_tex_width = num_nodes_y * irradiance_y_slice_width;
        uint32 irradiance_tex_height = num_nodes_z * LIGHT_PROBE_TOTAL_RESOLUTION;

        uint32 depth_y_slice_width = num_nodes_x * LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION;
        uint32 depth_tex_width = num_nodes_y * depth_y_slice_width;
        uint32 depth_tex_height = num_nodes_z * LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION;

#if 0
		struct ColorRGBA
		{
			uint8 r, g, b, a;
		};

		ColorRGBA* testBuffer = new ColorRGBA[irradiance_tex_width * irradiance_tex_height];


		ColorRGBA testProbe[LIGHT_PROBE_TOTAL_RESOLUTION][LIGHT_PROBE_TOTAL_RESOLUTION] = {};

		for (uint32 ly = 1; ly < LIGHT_PROBE_TOTAL_RESOLUTION - 1; ++ly)
		{
			for (uint32 lx = 1; lx < LIGHT_PROBE_TOTAL_RESOLUTION - 1; ++lx)
			{
				// Subtract the border.
				float x = (float)(lx - 1);
				float y = (float)(ly - 1);

				vec2 uv = (vec2(x, y) + 0.5f) / LIGHT_PROBE_RESOLUTION;
				vec2 oct = uv * 2.f - 1.f;

				vec3 dir = decode_octahedral(oct);

				vec3 c = lerp(vec3(0.f, 0.f, 1.f), vec3(1.f, 0.f, 0.f), dir.x * 0.5f + 0.5f);
				//vec3 c = vec3(dir.x, 0.f, 0.f);
				testProbe[ly][lx] = { (uint8)(clamp01(c.x) * 255.f), (uint8)(clamp01(c.y) * 255.f), (uint8)(clamp01(c.z) * 255.f), 255 };
				int a = 0;
			}
		}

		// Copy border.
		testProbe[0][0] = testProbe[6][6];
		testProbe[0][7] = testProbe[6][1];
		testProbe[7][0] = testProbe[1][6];
		testProbe[7][7] = testProbe[1][1];

		testProbe[0][1] = testProbe[1][6];
		testProbe[0][2] = testProbe[1][5];
		testProbe[0][3] = testProbe[1][4];
		testProbe[0][4] = testProbe[1][3];
		testProbe[0][5] = testProbe[1][2];
		testProbe[0][6] = testProbe[1][1];

		testProbe[7][1] = testProbe[6][6];
		testProbe[7][2] = testProbe[6][5];
		testProbe[7][3] = testProbe[6][4];
		testProbe[7][4] = testProbe[6][3];
		testProbe[7][5] = testProbe[6][2];
		testProbe[7][6] = testProbe[6][1];

		testProbe[1][0] = testProbe[6][1];
		testProbe[2][0] = testProbe[5][1];
		testProbe[3][0] = testProbe[4][1];
		testProbe[4][0] = testProbe[3][1];
		testProbe[5][0] = testProbe[2][1];
		testProbe[6][0] = testProbe[1][1];

		testProbe[1][7] = testProbe[6][6];
		testProbe[2][7] = testProbe[5][6];
		testProbe[3][7] = testProbe[4][6];
		testProbe[4][7] = testProbe[3][6];
		testProbe[5][7] = testProbe[2][6];
		testProbe[6][7] = testProbe[1][6];

		for (uint32 z = 0; z < num_nodes_z; ++z)
		{
			for (uint32 y = 0; y < num_nodes_y; ++y)
			{
				for (uint32 x = 0; x < num_nodes_x; ++x)
				{
					uint32 texStartX = irradiance_y_slice_width * y + x * LIGHT_PROBE_TOTAL_RESOLUTION;
					uint32 texStartY = z * LIGHT_PROBE_TOTAL_RESOLUTION;

					for (uint32 ly = 0; ly < LIGHT_PROBE_TOTAL_RESOLUTION; ++ly)
					{
						for (uint32 lx = 0; lx < LIGHT_PROBE_TOTAL_RESOLUTION; ++lx)
						{
							uint32 gx = texStartX + lx;
							uint32 gy = texStartY + ly;

							uint32 c = irradiance_tex_width * gy + gx;

							testBuffer[c] = testProbe[ly][lx];
						}
					}
				}
			}
		}

		this->irradiance = create_texture(testBuffer, irradiance_tex_width, irradiance_tex_height, DXGI_FORMAT_R8G8B8A8_UNORM, false, false, true, D3D12_RESOURCE_STATE_GENERIC_READ);

		delete[] testBuffer;
#else
        this->irradiance = create_texture(0, irradiance_tex_width, irradiance_tex_height, irradiance_format, false, false, true);
#endif

        this->depth = create_texture(0, depth_tex_width, depth_tex_height, depth_format, false, false, true);

        this->raytraced_radiance = create_texture(0, NUM_RAYS_PER_PROBE, total_num_nodes, raytraced_radiance_format, 0, 0, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        this->raytraced_direction_and_distance = create_texture(0, NUM_RAYS_PER_PROBE, total_num_nodes, raytraced_direction_and_distance_format, false, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    struct VisualizeGridRenderData
    {
        mat4 transform;
        DXVertexBufferGroupView vertex_buffer;
        DXIndexBufferView index_buffer;
        SubmeshInfo submesh;

        float cell_size;
        uint32 count_x;
        uint32 count_y;
        uint32 count_z;
        uint32 total;

        ref<DXTexture> texture0;
        ref<DXTexture> texture1;
    };

    struct VisualizeGridPipeline
    {
        PIPELINE_SETUP_DECL;
        PIPELINE_RENDER_DECL(VisualizeGridRenderData);
    };

    PIPELINE_SETUP_IMPL(VisualizeGridPipeline)
    {
        cl->set_pipeline_state(*visualize_grid_pipeline.pipeline);
        cl->set_graphics_root_signature(*visualize_grid_pipeline.root_signature);
        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    PIPELINE_RENDER_IMPL(VisualizeGridPipeline, VisualizeGridRenderData)
    {
        vec2 uv_scale = 1.f / vec2((float)(data.count_x * data.count_y), (float)data.count_z);
        cl->set_graphics_32bit_constants(LIGHT_PROBE_GRID_VISUALIZATION_RS_CB, LightProbeGridVisualizationCB{view_proj * data.transform, uv_scale, data.cell_size, data.count_x, data.count_y});
        cl->set_descriptor_heap_srv(LIGHT_PROBE_GRID_VISUALIZATION_RS_IRRADIANCE, 0, data.texture0);

        cl->set_vertex_buffer(0, data.vertex_buffer.positions);
        cl->set_vertex_buffer(1, data.vertex_buffer.others);
        cl->set_index_buffer(data.index_buffer);
        cl->draw_indexed(data.submesh.num_indices, data.total, data.submesh.first_index, data.submesh.base_vertex, 0);
    }

    struct VisualizeRaysRenderData
    {
        mat4 transform;

        float cell_size;
        uint32 count_x;
        uint32 count_y;
        uint32 count_z;
        uint32 total;

        ref<DXTexture> texture0;
        ref<DXTexture> texture1;
    };

    struct VisualizeRaysPipeline
    {
        PIPELINE_SETUP_DECL;
        PIPELINE_RENDER_DECL(VisualizeRaysRenderData);
    };

    PIPELINE_SETUP_IMPL(VisualizeRaysPipeline)
    {
        cl->set_pipeline_state(*visualize_rays_pipeline.pipeline);
        cl->set_graphics_root_signature(*visualize_rays_pipeline.root_signature);
        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    }

    PIPELINE_RENDER_IMPL(VisualizeRaysPipeline, VisualizeRaysRenderData)
    {
        cl->set_graphics_32bit_constants(LIGHT_PROBE_RAY_VISUALIZATION_RS_CB, LightProbeRayVisualizationCB{view_proj * data.transform, data.cell_size, data.count_x, data.count_y});
        cl->set_descriptor_heap_srv(LIGHT_PROBE_RAY_VISUALIZATION_RS_RAYS, 0, data.texture0);
        cl->set_descriptor_heap_srv(LIGHT_PROBE_RAY_VISUALIZATION_RS_RAYS, 1, data.texture1);

        cl->transition_barrier(data.texture0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
        cl->transition_barrier(data.texture1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
        cl->draw(2, NUM_RAYS_PER_PROBE * data.total, 0, 0);
        cl->transition_barrier(data.texture0, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cl->transition_barrier(data.texture1, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    void LightProbeGrid::visualize(OpaqueRenderPass* render_pass)
    {
        if (!dx_context.feature_support.raytracing() || total_num_nodes == 0)
            return;

        if (visualize_probes)
        {
            mat4 transform = create_translation_matrix(min_corner);
            VisualizeGridRenderData data =
                {
                    transform, sphere_mesh.vertex_buffer, sphere_mesh.index_buffer, sphere_submesh, cell_size, num_nodes_x, num_nodes_y, num_nodes_z, total_num_nodes, irradiance};

            render_pass->render_object<VisualizeGridPipeline>(data);
        }
        if (visualize_rays)
        {
            mat4 transform = create_translation_matrix(min_corner);
            VisualizeRaysRenderData data = {transform, cell_size, num_nodes_x, num_nodes_y, num_nodes_z, total_num_nodes, raytraced_radiance, raytraced_direction_and_distance};

            render_pass->render_object<VisualizeRaysPipeline>(data);
        }
    }

    void LightProbeGrid::update_probes(DXCommandList* cl, const RaytracingTLAS& light_probe_tlas, const CommonRenderData& common) const
    {
        if (!dx_context.feature_support.raytracing() || total_num_nodes == 0)
            return;

        if (auto_rotate_rays || rotate_rays)
        {
            ray_rotation = rng.random_rotation();
            rotate_rays = false;
        }

        light_probe_tracer.finalize_for_render();
        light_probe_tracer.render(cl, light_probe_tlas, *this, common);
    }

#define LIGHT_PROBE_TRACING_RS_RESOURCES 0
#define LIGHT_PROBE_TRACING_RS_CB 1
#define LIGHT_PROBE_TRACING_RS_SUN 2

    void LightProbeTracer::initialize()
    {
        const wchar* shader_path = L"light_probe/light_probe_trace_rts.hlsl";

        const uint32 num_input_resources = sizeof(InputResources) / sizeof(DXCPUDescriptorHandle);
        const uint32 num_output_resources = sizeof(OutputResources) / sizeof(DXCPUDescriptorHandle);

        CD3DX12_DESCRIPTOR_RANGE resource_ranges[] =
            {
                // Must be input first, then output
                CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, num_input_resources, 0),
                CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, num_output_resources, 0),
            };

        CD3DX12_ROOT_PARAMETER global_root_parameters[] =
            {
                root_descriptor_table(arraysize(resource_ranges), resource_ranges),
                root_constants<LightProbeTraceCB>(0),
                root_cbv(1),
            };

        CD3DX12_STATIC_SAMPLER_DESC global_static_sampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

        D3D12_ROOT_SIGNATURE_DESC global_desc =
            {
                arraysize(global_root_parameters), global_root_parameters,
                1, &global_static_sampler};

        PBRRaytracer::initialize(shader_path, max_payload_size, max_recursion_depth, global_desc);

        allocate_descriptor_heap_space_for_global_resources<InputResources, OutputResources>(descriptor_heap);
    }

    void LightProbeTracer::render(DXCommandList* cl, const RaytracingTLAS& tlas, const LightProbeGrid& grid, const CommonRenderData& common)
    {
        if (!tlas.tlas || grid.total_num_nodes == 0)
            return;

        {
            PROFILE_ALL(cl, "Raytrace probes");

            InputResources in;
            in.tlas = tlas.tlas->raytracing_srv;
            in.sky = common.sky->default_srv;
            in.irradiance = grid.irradiance->default_srv;
            in.depth = grid.depth->default_srv;

            OutputResources out;
            out.radiance = grid.raytraced_radiance->default_uav;
            out.direction_and_distance = grid.raytraced_direction_and_distance->default_uav;

            DXGPUDescriptorHandle gpu_handle = copy_global_resources_to_descriptor_heap(in, out);

            // Fill out description
            D3D12_DISPATCH_RAYS_DESC raytrace_desc;
            fill_out_ray_tracing_render_desc(binding_table.get_buffer(), raytrace_desc,
                NUM_RAYS_PER_PROBE, grid.total_num_nodes, 1,
                num_ray_types, binding_table.get_number_of_hit_groups());

            // Set up pipeline
            cl->set_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptor_heap.descriptor_heap);

            cl->set_pipeline_state(pipeline.pipeline);
            cl->set_compute_root_signature(pipeline.root_signature);

            LightProbeTraceCB cb;
            cb.ray_rotation = create_model_matrix(0.f, grid.ray_rotation);
            cb.grid = grid.get_cb();
            cb.sample_sky_from_texture = !common.procedural_sky;

            cl->set_compute_descriptor_table(LIGHT_PROBE_TRACING_RS_RESOURCES, gpu_handle);
            cl->set_compute_32bit_constants(LIGHT_PROBE_TRACING_RS_CB, cb);
            cl->set_compute_dynamic_constant_buffer(LIGHT_PROBE_TRACING_RS_SUN, common.lighting_cbv);

            cl->raytrace(raytrace_desc);

            cl->reset_to_dynamic_descriptor_heap();
        }

        BarrierBatcher(cl)
            .transition(grid.raytraced_radiance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            .transition(grid.raytraced_direction_and_distance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            .transition(grid.irradiance, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            .transition(grid.depth, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        {
            PROFILE_ALL(cl, "Update irradiance");

            cl->set_pipeline_state(*probe_update_irradiance_pipeline.pipeline);
            cl->set_compute_root_signature(*probe_update_irradiance_pipeline.root_signature);

            LightProbeUpdateCB cb;
            cb.count_x = grid.num_nodes_x;
            cb.count_y = grid.num_nodes_y;

            cl->set_compute_32bit_constants(LIGHT_PROBE_UPDATE_RS_CB, cb);
            cl->set_descriptor_heap_srv(LIGHT_PROBE_UPDATE_RS_RAYTRACE_RESULTS, 0, grid.raytraced_radiance);
            cl->set_descriptor_heap_srv(LIGHT_PROBE_UPDATE_RS_RAYTRACE_RESULTS, 1, grid.raytraced_direction_and_distance);
            cl->set_descriptor_heap_uav(LIGHT_PROBE_UPDATE_RS_RAYTRACE_RESULTS, 2, grid.irradiance);

            cl->dispatch(bucketize(grid.irradiance->width, LIGHT_PROBE_BLOCK_SIZE), bucketize(grid.irradiance->height, LIGHT_PROBE_BLOCK_SIZE));
        }

#if 0
		{
			PROFILE_ALL(cl, "Update depth");

			cl->set_pipeline_state(*probe_update_depth_pipeline.pipeline);
			cl->set_compute_root_signature(*probe_update_depth_pipeline.root_signature);

			LightProbeUpdateCB cb;
			cb.count_x = grid.num_nodes_x;
			cb.count_y = grid.num_nodes_y;

			cl->set_compute_32bit_constants(LIGHT_PROBE_UPDATE_RS_CB, cb);
			cl->set_descriptor_heap_srv(LIGHT_PROBE_UPDATE_RS_RAYTRACE_RESULTS, 0, grid.raytraced_radiance);
			cl->set_descriptor_heap_srv(LIGHT_PROBE_UPDATE_RS_RAYTRACE_RESULTS, 1, grid.raytraced_direction_and_distance);
			cl->set_descriptor_heap_uav(LIGHT_PROBE_UPDATE_RS_RAYTRACE_RESULTS, 2, grid.depth);

			cl->dispatch(bucketize(grid.depth->width, LIGHT_PROBE_BLOCK_SIZE), bucketize(grid.depth->height, LIGHT_PROBE_BLOCK_SIZE));
		}
#endif

        BarrierBatcher(cl)
            .transition(grid.raytraced_radiance, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            .transition(grid.raytraced_direction_and_distance, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            .transition(grid.irradiance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON)
            .transition(grid.depth, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
    }
}