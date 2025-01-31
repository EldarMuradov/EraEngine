// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/camera.h"
#include "core/bounding_volumes.h"

namespace era_engine
{
	struct UserInput;

	struct ERA_CORE_API runtime_camera_controller
	{
		void initialize(render_camera* camera) { this->camera = camera; }

		void update(uint32 viewportWidth, uint32 viewportHeight);

		render_camera* camera = nullptr;
	};

	struct ERA_CORE_API camera_controller
	{
		void initialize(render_camera* camera) { this->camera = camera; }

		// Returns true, if camera is moved, and therefore input is captured
		bool centerCameraOnObject(const bounding_box& aabb);
		bool update(const UserInput& input, uint32 viewportWidth, uint32 viewportHeight, float dt);

		render_camera* camera = nullptr;

	private:
		float orbitRadius = 10.f;
		float centeringTime = -1.f;

		vec3 centeringPositionStart;
		vec3 centeringPositionTarget;
	};
}