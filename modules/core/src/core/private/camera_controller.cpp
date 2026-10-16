// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "core/camera_controller.h"
#include "core/input.h"

namespace era_engine
{
	bool CameraController::center_camera_on_object(const BoundingBox& aabb)
	{
		vec3 center = aabb.get_center();
		float radius = length(aabb.get_radius());

		vec3 offset_direction = camera->rotation * vec3(0.f, 0.f, 1.f); // Camera backward axis

		float min_extent = camera->get_min_projection_extent();

		float scaling = radius / min_extent;

		centering_time = 0.f;

		centering_position_start = camera->position;
		centering_position_target = center + scaling * offset_direction;

		orbit_radius = scaling;

		return true;
	}

	bool CameraController::update(const UserInput& input, uint32 viewport_width, uint32 viewport_height, float dt)
	{
		const float CAMERA_MOVEMENT_SPEED = 8.f;
		const float CAMERA_SENSITIVITY = 4.f;
		const float CAMERA_CENTERING_TIME = 0.1f;

		camera->set_viewport(viewport_width, viewport_height);

		bool result = false;

		if (centering_time >= 0.f)
		{
			centering_time += dt;

			float relative_time = min(centering_time / CAMERA_CENTERING_TIME, 1.f);
			camera->position = lerp(centering_position_start, centering_position_target, relative_time);

			if (relative_time == 1.f)
			{
				centering_time = -1.f;
			}
		}
		else if (input.mouse.right.down)
		{
			// Fly camera
			vec3 camera_input_dir = vec3(
				(input.keyboard['D'].down ? 1.f : 0.f) + (input.keyboard['A'].down ? -1.f : 0.f),
				(input.keyboard['E'].down ? 1.f : 0.f) + (input.keyboard['Q'].down ? -1.f : 0.f),
				(input.keyboard['W'].down ? -1.f : 0.f) + (input.keyboard['S'].down ? 1.f : 0.f)
			) * (input.keyboard[KEY_SHIFT].down ? 3.f : 1.f) * (input.keyboard[KEY_CTRL].down ? 0.1f : 1.f) * CAMERA_MOVEMENT_SPEED;

			vec2 turn_angle(0.f, 0.f);
			turn_angle = vec2(-input.mouse.reldx, -input.mouse.reldy) * CAMERA_SENSITIVITY;

			quat& camera_rotation = camera->rotation;
			camera_rotation = quat(vec3(0.f, 1.f, 0.f), turn_angle.x) * camera_rotation;
			camera_rotation = camera_rotation * quat(vec3(1.f, 0.f, 0.f), turn_angle.y);

			camera->position += camera_rotation * camera_input_dir * dt;

			result = true;
		}
		else if (input.keyboard[KEY_ALT].down)
		{
			if (input.mouse.left.down)
			{
				// Orbit camera
				vec2 turn_angle(0.f, 0.f);
				turn_angle = vec2(-input.mouse.reldx, -input.mouse.reldy) * CAMERA_SENSITIVITY;

				quat& camera_rotation = camera->rotation;

				vec3 center = camera->position + camera_rotation * vec3(0.f, 0.f, -orbit_radius);

				camera_rotation = quat(vec3(0.f, 1.f, 0.f), turn_angle.x) * camera_rotation;
				camera_rotation = camera_rotation * quat(vec3(1.f, 0.f, 0.f), turn_angle.y);

				camera->position = center - camera_rotation * vec3(0.f, 0.f, -orbit_radius);
			}
			else if (input.mouse.middle.down)
			{
				// Pan camera
				vec3 camera_input_dir = vec3(
					-input.mouse.reldx * camera->aspect,
					input.mouse.reldy,
					0.f
				) * (input.keyboard[KEY_SHIFT].down ? 3.f : 1.f) * (input.keyboard[KEY_CTRL].down ? 0.1f : 1.f) * 1000.f * CAMERA_MOVEMENT_SPEED;

				camera->position += camera->rotation * camera_input_dir * dt;
			}

			result = true;
		}

		camera->update_matrices();

		return result;
	}

	void RuntimeCameraController::update(uint32 viewport_width, uint32 viewport_height)
	{
		camera->set_viewport(viewport_width, viewport_height);
		camera->update_matrices();
	}

}