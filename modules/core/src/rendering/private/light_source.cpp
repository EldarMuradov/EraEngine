// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/light_source.h"

#include <rttr/registration>

namespace era_engine
{
	void DirectionalLight::update_matrices(const RenderCamera& camera)
	{
		CameraFrustumCorners world_frustum = camera.get_world_space_frustum_corners(cascade_distances.w);

		vec3 bottom_left_ray = world_frustum.far_bottom_left - world_frustum.near_bottom_left;
		vec3 bottom_right_ray = world_frustum.far_bottom_right - world_frustum.near_bottom_right;
		vec3 top_left_ray = world_frustum.far_top_left - world_frustum.near_top_left;
		vec3 top_right_ray = world_frustum.far_top_right - world_frustum.near_top_right;

		vec3 camera_forward = camera.rotation * vec3(0.f, 0.f, -1.f);

		bottom_left_ray /= dot(bottom_left_ray, camera_forward);
		bottom_right_ray /= dot(bottom_right_ray, camera_forward);
		top_left_ray /= dot(top_left_ray, camera_forward);
		top_right_ray /= dot(top_right_ray, camera_forward);

		for (uint32 cascade = 0; cascade < num_shadow_cascades; ++cascade)
		{
			float prev_distance = (cascade == 0) ? camera.near_plane : (cascade_distances.data[cascade - 1] - blend_distances.data[cascade - 1]);
			float distance = cascade_distances.data[cascade];

			vec3 corners[8];
			corners[0] = camera.position + bottom_left_ray * prev_distance;
			corners[1] = camera.position + bottom_right_ray * prev_distance;
			corners[2] = camera.position + top_left_ray * prev_distance;
			corners[3] = camera.position + top_right_ray * prev_distance;
			corners[4] = camera.position + bottom_left_ray * distance;
			corners[5] = camera.position + bottom_right_ray * distance;
			corners[6] = camera.position + top_left_ray * distance;
			corners[7] = camera.position + top_right_ray * distance;

			vec3 center(0.0f);
			for (uint32 i = 0; i < 8; ++i)
			{
				center += corners[i];
			}
			center /= 8.0f;

			vec3 up_dir = vec3(0.f, 1.0f, 0.f);
			if (abs(dot(up_dir, direction)) > 1.0f - EPSILON)
			{
				up_dir = vec3(0.0f, 0.0f, 1.0f);
			}

			mat4 view_matrix = look_at(center, center + direction, up_dir);

			vec3 min_extents, max_extents;
			if (stabilize)
			{
				vec3 view_center = transform_position(view_matrix, center);

				float sphere_radius = 0.f;
				for (uint32 i = 0; i < 8; ++i)
				{
					float d = length(transform_position(view_matrix, corners[i]) - view_center);
					sphere_radius = max(sphere_radius, d);
				}

				sphere_radius = ceil(sphere_radius * 16.0f) / 16.0f;
				min_extents = -sphere_radius;
				max_extents = sphere_radius;
			}
			else
			{
				BoundingBox extents = BoundingBox::negative_infinity();
				for (uint32 i = 0; i < 8; ++i)
				{
					vec3 c = transform_position(view_matrix, corners[i]);
					extents.grow(c);
				}

				min_extents = extents.min_corner;
				max_extents = extents.max_corner;
			}

			vec3 cascade_extents = max_extents - min_extents;
			vec3 shadow_cam_pos = center + direction * min_extents.z;

			//viewMatrix = lookAt(shadowCamPos, shadowCamPos + direction, upDir);

			mat4 proj_matrix = create_orthographic_projection_matrix(max_extents.x, min_extents.x, max_extents.y, min_extents.y, -negative_z_offset, cascade_extents.z);

			{
				mat4 matrix = proj_matrix * view_matrix;
				vec3 shadow_origin(0.0f);
				shadow_origin = (matrix * vec4(shadow_origin, 1.0f)).xyz;
				shadow_origin *= (shadow_dimensions * 0.5f);

				vec3 rounded_origin = round(shadow_origin);
				vec3 round_offset = rounded_origin - shadow_origin;
				round_offset = round_offset * (2.f / shadow_dimensions);

				proj_matrix.m03 += round_offset.x;
				proj_matrix.m13 += round_offset.y;
			}

			view_projs[cascade] = proj_matrix * view_matrix;
		}
	}

	mat4 get_spot_light_view_projection_matrix(const SpotLightCB& sl)
	{
		mat4 view_matrix = look_at(sl.position, sl.position + sl.direction, vec3(0.f, 1.f, 0.f));
		mat4 proj_matrix = create_perspective_projection_matrix(acos(sl.get_outer_cutoff()) * 2.f, 1.f, 0.01f, sl.max_distance);
		return proj_matrix * view_matrix;
	}

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<DirectionalLight>("DirectionalLight")
			.constructor<>()
			.property("color", &DirectionalLight::color)
			.property("intensity", &DirectionalLight::intensity)
			.property("bias", &DirectionalLight::bias)
			.property("cascade_distances", &DirectionalLight::cascade_distances)
			.property("blend_distances", &DirectionalLight::blend_distances)
			.property("direction", &DirectionalLight::direction)
			.property("negative_z_offset", &DirectionalLight::negative_z_offset)
			.property("shadow_dimensions", &DirectionalLight::shadow_dimensions)
			.property("num_shadow_cascades", &DirectionalLight::num_shadow_cascades)
			.property("stabilize", &DirectionalLight::stabilize);

		registration::class_<PointLightComponent>("PointLightComponent")
			.constructor<>()
			.property("color", &PointLightComponent::color)
			.property("intensity", &PointLightComponent::intensity)
			.property("radius", &PointLightComponent::radius)
			.property("castsShadow", &PointLightComponent::casts_shadow)
			.property("shadowMapResolution", &PointLightComponent::shadow_map_resolution);

		registration::class_<SpotLightComponent>("SpotLightComponent")
			.constructor<>()
			.property("color", &SpotLightComponent::color)
			.property("intensity", &SpotLightComponent::intensity)
			.property("distance", &SpotLightComponent::distance)
			.property("innerAngle", &SpotLightComponent::inner_angle)
			.property("outerAngle", &SpotLightComponent::outer_angle)
			.property("castsShadow", &SpotLightComponent::casts_shadow)
			.property("shadowMapResolution", &SpotLightComponent::shadow_map_resolution);
	}

	PointLightComponent::PointLightComponent(ref<Entity::EcsData> _data, const vec3& _color, float _intensity, float _radius, bool _casts_shadow, uint32 _shadow_map_resolution)
		: Component(_data), color(_color), intensity(_intensity), radius(_radius), casts_shadow(_casts_shadow), shadow_map_resolution(_shadow_map_resolution)
	{
	}

	PointLightComponent::~PointLightComponent()
	{
	}

	SpotLightComponent::SpotLightComponent(ref<Entity::EcsData> _data, const vec3& _color, float _intensity, float _distance, float _inner_angle, float _outer_angle, bool _casts_shadow, uint32 _shadow_map_resolution)
		: Component(_data), color(_color), intensity(_intensity), distance(_distance), inner_angle(_inner_angle), outer_angle(_outer_angle), casts_shadow(_casts_shadow), shadow_map_resolution(_shadow_map_resolution)

	{
	}

	SpotLightComponent::~SpotLightComponent()
	{
	}

}