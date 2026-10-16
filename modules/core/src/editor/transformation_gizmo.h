// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"
#include "core/input.h"
#include "core/camera.h"

#include "rendering/render_pass.h"

#include "core/bounding_volumes.h"

namespace era_engine
{
	struct ERA_CORE_API TransformationGizmo
	{
		bool manipulate_transformation(trs& transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass);
		bool manipulate_position(vec3& position, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass);
		bool manipulate_position_rotation(vec3& position, quat& rotation, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass);
		bool manipulate_position_scale(vec3& position, vec3& scale, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass);
		bool manipulate_nothing(const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass);

		bool manipulate_bounding_sphere(BoundingSphere& sphere, const trs& parent_transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass);
		bool manipulate_bounding_capsule(BoundingCapsule& capsule, const trs& parent_transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass);
		bool manipulate_bounding_cylinder(BoundingCylinder& cylinder, const trs& parent_transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass);
		bool manipulate_bounding_box(BoundingBox& aabb, const trs& parent_transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass);
		bool manipulate_oriented_bounding_box(BoundingOrientedBox& obb, const trs& parent_transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass);

		// Transform before start of dragging. Only valid if object was dragged.
		trs original_transform;

		bool dragging = false;

	private:
		bool handle_user_input(bool allow_keyboard_input, bool allow_translation, bool allow_rotation, bool allow_scaling, bool allow_space_change);
		void manipulate_internal(trs& transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass, bool allow_non_uniform_scale = true);
		void manipulate_handles(vec3* handles, uint32 num_handles); //TODO

		enum TransformationType
		{
			TRANSFORMATION_TYPE_NONE = -1,
			TRANSFORMATION_TYPE_TRANSLATION,
			TRANSFORMATION_TYPE_ROTATION,
			TRANSFORMATION_TYPE_SCALE,
		};

		enum TransformationSpace
		{
			TRANSFORMATION_GLOBAL,
			TRANSFORMATION_LOCAL,
		};

		uint32 handle_translation(trs& transform, ray r, const UserInput& input, float snapping, float scaling);
		uint32 handle_rotation(trs& transform, ray r, const UserInput& input, float snapping, float scaling);
		uint32 handle_scaling(trs& transform, ray r, const UserInput& input, float scaling, const RenderCamera& camera, bool allow_non_uniform);

		TransformationType type = TRANSFORMATION_TYPE_TRANSLATION;
		TransformationSpace space = TRANSFORMATION_GLOBAL;

		uint32 axis_index;
		float anchor;
		vec3 anchor3;
		vec4 plane;

		vec3 original_position;
		quat original_rotation;
		vec3 original_scale;
	};

	ERA_CORE_API void initialize_transformation_gizmos();
}