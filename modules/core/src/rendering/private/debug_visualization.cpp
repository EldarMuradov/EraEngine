// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/debug_visualization.h"
#include "rendering/render_utils.h"
#include "rendering/render_resources.h"

#include "core/cpu_profiling.h"

#include "transform.hlsli"

namespace era_engine
{
	static DXPipeline simple_pipeline;
	static DXPipeline unlit_position_pipeline;
	static DXPipeline unlit_position_color_pipeline;
	static DXPipeline unlit_line_position_pipeline;
	static DXPipeline unlit_line_position_color_pipeline;

	static D3D12_INPUT_ELEMENT_DESC input_layout_position_color[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	void DebugSimplePipeline::initialize()
	{
		auto& desc = CREATE_GRAPHICS_PIPELINE
			.input_layout(input_layout_position_normal)
			.render_targets(ldr_format, depth_stencil_format);

		simple_pipeline = create_reloadable_pipeline(desc, { "flat_simple_vs", "flat_simple_ps" });
	}

	PIPELINE_SETUP_IMPL(DebugSimplePipeline)
	{
		cl->set_pipeline_state(*simple_pipeline.pipeline);
		cl->set_graphics_root_signature(*simple_pipeline.root_signature);
		cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		cl->set_graphics_dynamic_constant_buffer(FLAT_SIMPLE_RS_CAMERA, common.camera_cbv);
	}

	PIPELINE_RENDER_IMPL(DebugSimplePipeline, DebugRenderData)
	{
		cl->set_graphics_32bit_constants(FLAT_SIMPLE_RS_TRANFORM, TransformCB{ view_proj * data.transform, data.transform });
		cl->set_graphics_32bit_constants(FLAT_SIMPLE_RS_CB, VisualizationCB{ data.color });
		cl->set_vertex_buffer(0, data.vertex_buffer.positions);
		cl->set_vertex_buffer(1, data.vertex_buffer.others);
		cl->set_index_buffer(data.index_buffer);
		cl->draw_indexed(data.submesh.num_indices, 1, data.submesh.first_index, data.submesh.base_vertex, 0);
	}

	void DebugUnlitPipeline::initialize()
	{
		auto& desc = CREATE_GRAPHICS_PIPELINE
			.input_layout(input_layout_position)
			.culling_off()
			.render_targets(ldr_format, depth_stencil_format);
		unlit_position_pipeline = create_reloadable_pipeline(desc, { "flat_unlit_position_vs", "flat_unlit_ps" });

		desc.input_layout(input_layout_position_color);
		unlit_position_color_pipeline = create_reloadable_pipeline(desc, { "flat_unlit_position_color_vs", "flat_unlit_ps" });
	}

	PIPELINE_SETUP_IMPL(DebugUnlitPipeline::Position)
	{
		cl->set_pipeline_state(*unlit_position_pipeline.pipeline);
		cl->set_graphics_root_signature(*unlit_position_pipeline.root_signature);
		cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	PIPELINE_SETUP_IMPL(DebugUnlitPipeline::PositionColor)
	{
		cl->set_pipeline_state(*unlit_position_color_pipeline.pipeline);
		cl->set_graphics_root_signature(*unlit_position_color_pipeline.root_signature);
		cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	PIPELINE_RENDER_IMPL(DebugUnlitPipeline, DebugRenderData)
	{
		cl->set_graphics_32bit_constants(FLAT_UNLIT_RS_TRANFORM, view_proj * data.transform);
		cl->set_graphics_32bit_constants(FLAT_UNLIT_RS_CB, VisualizationCB{ data.color });
		cl->set_vertex_buffer(0, data.vertex_buffer.positions);
		cl->set_index_buffer(data.index_buffer);
		cl->draw_indexed(data.submesh.num_indices, 1, data.submesh.first_index, data.submesh.base_vertex, 0);
	}

	void DebugUnlitLinePipeline::initialize()
	{
		auto& desc = CREATE_GRAPHICS_PIPELINE
			.input_layout(input_layout_position)
			.render_targets(ldr_format, depth_stencil_format)
			.primitive_topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
		unlit_line_position_pipeline = create_reloadable_pipeline(desc, { "flat_unlit_position_vs", "flat_unlit_ps" });

		desc.input_layout(input_layout_position_color);
		unlit_line_position_color_pipeline = create_reloadable_pipeline(desc, { "flat_unlit_position_color_vs", "flat_unlit_ps" });
	}

	PIPELINE_SETUP_IMPL(DebugUnlitLinePipeline::Position)
	{
		cl->set_pipeline_state(*unlit_line_position_pipeline.pipeline);
		cl->set_graphics_root_signature(*unlit_line_position_pipeline.root_signature);
		cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	}

	PIPELINE_SETUP_IMPL(DebugUnlitLinePipeline::PositionColor)
	{
		cl->set_pipeline_state(*unlit_line_position_color_pipeline.pipeline);
		cl->set_graphics_root_signature(*unlit_line_position_color_pipeline.root_signature);
		cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	}

	PIPELINE_RENDER_IMPL(DebugUnlitLinePipeline, DebugRenderData)
	{
		cl->set_graphics_32bit_constants(FLAT_UNLIT_RS_TRANFORM, view_proj * data.transform);
		cl->set_graphics_32bit_constants(FLAT_UNLIT_RS_CB, data.color);
		cl->set_vertex_buffer(0, data.vertex_buffer.positions);
		cl->set_index_buffer(data.index_buffer);
		cl->draw_indexed(data.submesh.num_indices, 1, data.submesh.first_index, data.submesh.base_vertex, 0);
	}

	void render_point(vec3 pos, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		render_wire_sphere(pos, 0.05f, color, render_pass, overlay);
	}

	void render_triangle(vec3 a, vec3 b, vec3 c, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), 3);
		auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), 3);

		vec3* vertices = (vec3*)vertexPtr;
		IndexedTriangle16* triangles = (IndexedTriangle16*)index_ptr;

		*vertices++ = a;
		*vertices++ = b;
		*vertices++ = c;

		*triangles = { 0, 1, 2 };

		render_debug<DebugUnlitPipeline::Position>(mat4::identity, vb, ib, color, render_pass, overlay);
	}

	void render_disk(vec3 position, vec3 up_axis, float radius, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		const uint32 num_segments = 32;

		uint32 num_triangles = num_segments;
		uint32 num_vertices = num_segments + 1;

		auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), num_vertices);
		auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), num_triangles * 3);

		vec3* vertices = (vec3*)vertexPtr;
		IndexedTriangle16* triangles = (IndexedTriangle16*)index_ptr;

		float delta_rot = M_TAU / num_segments;
		float rot = 0.f;
		for (uint32 i = 0; i < num_segments; ++i)
		{
			*vertices++ = vec3(cos(rot), sin(rot), 0.f) * radius;
			rot += delta_rot;
		}

		*vertices++ = vec3(0.f, 0.f, 0.f);

		uint16 center = (uint16)num_vertices - 1;

		for (uint16 i = 0; i < num_segments; ++i)
		{
			uint16 next = i + 1;
			if (i == num_segments - 1)
			{
				next = 0;
			}
			*triangles++ = { center, i, next };
		}

		render_debug<DebugUnlitPipeline::Position>(create_model_matrix(position, rotate_from_to(vec3(0.f, 0.f, 1.f), up_axis)), vb, ib, color, render_pass, overlay);
	}

	void render_ring(vec3 position, vec3 up_axis, float outer_radius, float inner_radius, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		const uint32 num_segments = 32;

		uint32 num_triangles = num_segments * 2;
		uint32 num_vertices = num_segments * 2;

		auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), num_vertices);
		auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), num_triangles * 3);

		vec3* outer = (vec3*)vertexPtr;
		vec3* inner = outer + num_segments;
		IndexedTriangle16* triangles = (IndexedTriangle16*)index_ptr;

		float delta_rot = M_TAU / num_segments;
		float rot = 0.f;
		for (uint32 i = 0; i < num_segments; ++i)
		{
			vec3 p = vec3(cos(rot), sin(rot), 0.f);
			*outer++ = p * outer_radius;
			*inner++ = p * inner_radius;
			rot += delta_rot;
		}

		uint16 offset = (uint16)num_segments;
		for (uint16 i = 0; i < num_segments; ++i)
		{
			uint16 next = i + 1;
			if (i == num_segments - 1)
			{
				next = 0;
			}
			*triangles++ = { i, next, (uint16)(i + offset) };
			*triangles++ = { next, (uint16)(i + offset), (uint16)(next + offset) };
		}

		render_debug<DebugUnlitPipeline::Position>(create_model_matrix(position, rotate_from_to(vec3(0.f, 0.f, 1.f), up_axis)), vb, ib, color, render_pass, overlay);
	}

	void render_angle_ring(vec3 position, vec3 up_axis, float outer_radius, float inner_radius,
		vec3 zero_deg_axis, float min_angle, float max_angle, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		const uint32 num_segments360 = 32;

		const float delta_rot = M_TAU / num_segments360;
		float angle_range = max_angle - min_angle;
		const uint32 num_segments = (uint32)ceil(angle_range / delta_rot);

		uint32 num_triangles = num_segments * 2;
		uint32 num_vertices = (num_segments + 1) * 2;

		auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), num_vertices);
		auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), num_triangles * 3);

		vec3* outer = (vec3*)vertexPtr;
		vec3* inner = outer + (num_segments + 1);
		IndexedTriangle16* triangles = (IndexedTriangle16*)index_ptr;

		vec3 x = zero_deg_axis;
		vec3 z = cross(up_axis, x);

		float rot = min_angle;
		for (uint32 i = 0; i < num_segments + 1; ++i)
		{
			vec3 p = cos(rot) * x + sin(rot) * z;
			*outer++ = p * outer_radius + position;
			*inner++ = p * inner_radius + position;
			rot += delta_rot;
			rot = min(rot, max_angle);
		}

		uint16 offset = (uint16)(num_segments + 1);
		for (uint16 i = 0; i < num_segments; ++i)
		{
			uint16 next = i + 1;
			*triangles++ = { i, next, (uint16)(i + offset) };
			*triangles++ = { next, (uint16)(i + offset), (uint16)(next + offset) };
		}

		render_debug<DebugUnlitPipeline::Position>(mat4::identity, vb, ib, color, render_pass, overlay);
	}

	static std::tuple<DXDynamicVertexBuffer, DXDynamicIndexBuffer> get_wire_ring()
	{
		const uint32 num_segments = 32;

		uint32 num_lines = num_segments;
		uint32 num_vertices = num_segments;

		auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), num_vertices);
		auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), num_lines * 2);

		vec3* vertices = (vec3*)vertexPtr;
		IndexedLine16* lines = (IndexedLine16*)index_ptr;

		float delta_rot = M_TAU / num_segments;
		float rot = 0.f;
		for (uint32 i = 0; i < num_segments; ++i)
		{
			*vertices++ = vec3(cos(rot), sin(rot), 0.f);
			rot += delta_rot;
		}

		for (uint16 i = 0; i < num_segments; ++i)
		{
			uint16 next = i + 1;
			if (i == num_segments - 1)
			{
				next = 0;
			}
			*lines++ = { i, next };
		}

		return { vb, ib };
	}

	static std::tuple<DXDynamicVertexBuffer, DXDynamicIndexBuffer> get_wire_capsule_cross_section(float length, float radius)
	{
		const uint32 num_segments_per_half = 16;

		uint32 num_vertices = num_segments_per_half * 2 + 2;
		uint32 num_lines = num_vertices;

		auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), num_vertices);
		auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), num_lines * 2);

		vec3* vertices_a = (vec3*)vertexPtr;
		vec3* vertices_b = vertices_a + (num_segments_per_half + 1);
		IndexedLine16* lines = (IndexedLine16*)index_ptr;

		float delta_rot = M_PI / num_segments_per_half;
		float rot = 0.f;
		float half_length = length * 0.5f;
		for (uint32 i = 0; i < num_segments_per_half + 1; ++i)
		{
			float x = cos(rot), y = sin(rot);
			*vertices_a++ = vec3(radius * x, radius * y + half_length, 0.f);
			*vertices_b++ = -vec3(radius * x, radius * y + half_length, 0.f);
			rot += delta_rot;
		}

		for (uint16 i = 0; i < num_lines; ++i)
		{
			uint16 next = i + 1;
			if (i == num_lines - 1)
			{
				next = 0;
			}
			*lines++ = { i, next };
		}

		return { vb, ib };
	}

	void render_line(vec3 position_a, vec3 position_b, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		//CPU_PROFILE_BLOCK("Render line");

		auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), 2);
		auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), 2);

		vec3* vertices = (vec3*)vertexPtr;
		IndexedLine16* lines = (IndexedLine16*)index_ptr;

		*vertices++ = position_a;
		*vertices++ = position_b;

		*lines++ = { 0, 1 };

		render_debug<DebugUnlitLinePipeline::Position>(mat4::identity, vb, ib, color, render_pass, overlay);
	}

	void render_wire_triangle(vec3 a, vec3 b, vec3 c, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), 3);
		auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), 3);

		vec3* vertices = (vec3*)vertexPtr;
		IndexedTriangle16* triangles = (IndexedTriangle16*)index_ptr;

		*vertices++ = a;
		*vertices++ = b;
		*vertices++ = c;

		*triangles = { 0, 1, 2 };

		render_debug<DebugUnlitLinePipeline::Position>(mat4::identity, vb, ib, color, render_pass, overlay);
	}

	void render_wire_sphere(vec3 position, float radius, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		CPU_PROFILE_BLOCK("Render wire sphere");

		auto [vb, ib] = get_wire_ring();

		render_debug<DebugUnlitLinePipeline::Position>(create_model_matrix(position, quat::identity, radius), vb, ib, color, render_pass, overlay);
		render_debug<DebugUnlitLinePipeline::Position>(create_model_matrix(position, quat(vec3(0.f, 1.f, 0.f), deg2rad(90.f)), radius), vb, ib, color, render_pass, overlay);
		render_debug<DebugUnlitLinePipeline::Position>(create_model_matrix(position, quat(vec3(1.f, 0.f, 0.f), deg2rad(90.f)), radius), vb, ib, color, render_pass, overlay);
	}

	void render_wire_capsule(vec3 position_a, vec3 position_b, float radius, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		CPU_PROFILE_BLOCK("Render wire capsule");

		vec3 center = 0.5f * (position_a + position_b);
		quat rotation = rotate_from_to(vec3(0.f, 1.f, 0.f), position_a - position_b);
		auto [vb, ib] = get_wire_capsule_cross_section(length(position_a - position_b), radius);

		render_debug<DebugUnlitLinePipeline::Position>(create_model_matrix(center, rotation * quat::identity, 1.f), vb, ib, color, render_pass, overlay);
		render_debug<DebugUnlitLinePipeline::Position>(create_model_matrix(center, rotation * quat(vec3(0.f, 1.f, 0.f), deg2rad(90.f)), 1.f), vb, ib, color, render_pass, overlay);
	}

	void render_wire_cylinder(vec3 position_a, vec3 position_b, float radius, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		CPU_PROFILE_BLOCK("Render wire cylinder");

		quat rotation = rotate_from_to(vec3(0.f, 1.f, 0.f), position_a - position_b);

		{
			auto [vb, ib] = get_wire_ring();
			quat rotate90deg(vec3(1.f, 0.f, 0.f), deg2rad(90.f));
			render_debug<DebugUnlitLinePipeline::Position>(create_model_matrix(position_a, rotation * rotate90deg, radius), vb, ib, color, render_pass, overlay);
			render_debug<DebugUnlitLinePipeline::Position>(create_model_matrix(position_b, rotation * rotate90deg, radius), vb, ib, color, render_pass, overlay);
		}

		{
			auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), 8);
			auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), 8);

			vec3* vertices = (vec3*)vertexPtr;
			IndexedLine16* lines = (IndexedLine16*)index_ptr;

			vec3 x_axis = rotation * vec3(radius, 0.f, 0.f);
			vec3 z_axis = rotation * vec3(0.f, 0.f, radius);

			*vertices++ = position_a + x_axis;
			*vertices++ = position_b + x_axis;

			*vertices++ = position_a + z_axis;
			*vertices++ = position_b + z_axis;

			*vertices++ = position_a - x_axis;
			*vertices++ = position_b - x_axis;

			*vertices++ = position_a - z_axis;
			*vertices++ = position_b - z_axis;

			*lines++ = { 0, 1 };
			*lines++ = { 2, 3 };
			*lines++ = { 4, 5 };
			*lines++ = { 6, 7 };

			render_debug<DebugUnlitLinePipeline::Position>(mat4::identity, vb, ib, color, render_pass, overlay);
		}
	}

	void render_wire_cone(vec3 position, vec3 direction, float distance, float angle, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		CPU_PROFILE_BLOCK("Render wire cone");

		const uint32 num_segments = 32;
		const uint32 num_cone_lines = 8;
		static_assert(num_segments % num_cone_lines == 0, "");

		uint32 num_lines = num_cone_lines + num_segments;
		uint32 num_vertices = 1 + num_segments;

		auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), num_vertices);
		auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), num_lines * 2);

		vec3* vertices = (vec3*)vertexPtr;
		IndexedLine16* lines = (IndexedLine16*)index_ptr;

		float half_angle = angle * 0.5f;
		float axis_length = tan(half_angle);

		vec3 x_axis, y_axis;
		get_tangents(direction, x_axis, y_axis);
		vec3 z_axis = direction * distance;
		x_axis *= distance * axis_length;
		y_axis *= distance * axis_length;

		*vertices++ = position;

		float delta_rot = M_TAU / num_segments;
		float rot = 0.f;
		for (uint32 i = 0; i < num_segments; ++i)
		{
			*vertices++ = position + z_axis + x_axis * cos(rot) + y_axis * sin(rot);
			rot += delta_rot;
		}

		const uint16 step = num_segments / num_cone_lines;

		for (uint16 i = 0; i < num_cone_lines; ++i)
		{
			uint16 next = 1u + i * step;
			*lines++ = { 0, next };
		}

		for (uint16 i = 0; i < num_segments; ++i)
		{
			uint16 cur = i + 1;
			uint16 next = i + 2;
			if (i == num_segments - 1)
			{
				next = 1;
			}
			*lines++ = { cur, next };
		}

		render_debug<DebugUnlitLinePipeline::Position>(mat4::identity, vb, ib, color, render_pass, overlay);
	}

	void render_wire_box(vec3 position, vec3 radius, quat rotation, vec4 color, LDRRenderPass* render_pass, bool overlay)
	{
		CPU_PROFILE_BLOCK("Render wire box");

		auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), 8);
		auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), 12 * 2);

		vec3* vertices = (vec3*)vertexPtr;
		*vertices++ = vec3(-1.f, 1.f, -1.f);
		*vertices++ = vec3(1.f, 1.f, -1.f);
		*vertices++ = vec3(-1.f, -1.f, -1.f);
		*vertices++ = vec3(1.f, -1.f, -1.f);
		*vertices++ = vec3(-1.f, 1.f, 1.f);
		*vertices++ = vec3(1.f, 1.f, 1.f);
		*vertices++ = vec3(-1.f, -1.f, 1.f);
		*vertices++ = vec3(1.f, -1.f, 1.f);

		IndexedLine16* lines = (IndexedLine16*)index_ptr;
		*lines++ = { 0, 1 };
		*lines++ = { 1, 3 };
		*lines++ = { 3, 2 };
		*lines++ = { 2, 0 };

		*lines++ = { 4, 5 };
		*lines++ = { 5, 7 };
		*lines++ = { 7, 6 };
		*lines++ = { 6, 4 };

		*lines++ = { 0, 4 };
		*lines++ = { 1, 5 };
		*lines++ = { 2, 6 };
		*lines++ = { 3, 7 };

		render_debug<DebugUnlitLinePipeline::Position>(create_model_matrix(position, rotation, radius), vb, ib, color, render_pass, overlay);
	}

	void render_camera_frustum(const RenderCamera& frustum, vec4 color, LDRRenderPass* render_pass, float alternative_far_plane, bool overlay)
	{
		CPU_PROFILE_BLOCK("Render camera frustum");

		auto [vb, vertexPtr] = dx_context.create_dynamic_vertex_buffer(sizeof(vec3), 8);
		auto [ib, index_ptr] = dx_context.create_dynamic_index_buffer(sizeof(uint16), 12 * 2);

		vec3* vertices = (vec3*)vertexPtr;
		CameraFrustumCorners corners = frustum.get_world_space_frustum_corners(alternative_far_plane);
		for (uint32 i = 0; i < 8; ++i)
		{
			*vertices++ = corners.corners[i];
		}

		IndexedLine16* lines = (IndexedLine16*)index_ptr;
		*lines++ = { 0, 1 };
		*lines++ = { 1, 3 };
		*lines++ = { 3, 2 };
		*lines++ = { 2, 0 };

		*lines++ = { 4, 5 };
		*lines++ = { 5, 7 };
		*lines++ = { 7, 6 };
		*lines++ = { 6, 4 };

		*lines++ = { 0, 4 };
		*lines++ = { 1, 5 };
		*lines++ = { 2, 6 };
		*lines++ = { 3, 7 };

		render_debug<DebugUnlitLinePipeline::Position>(mat4::identity, vb, ib, color, render_pass, overlay);
	}
}