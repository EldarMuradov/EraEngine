// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/camera_controller.h"

namespace era_engine
{
	struct editor_scene;
	struct main_renderer;

	namespace runtime
	{
		struct runtime
		{
			void initialize(editor_scene* scene, main_renderer* renderer);

			void update();

		private:
			editor_scene* scene;
			main_renderer* renderer;

			runtime_camera_controller cameraController;

			friend struct application;
		};
	}
}