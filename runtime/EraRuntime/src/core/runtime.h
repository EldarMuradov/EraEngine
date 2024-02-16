#pragma once

#include "scene/scene.h"
#include "core/camera_controller.h"
#include "core/system.h"
#include "editor/undo_stack.h"
#include "editor/transformation_gizmo.h"
#include "editor/asset_editor_panel.h"
#include "rendering/main_renderer.h"
#include "physics/physics.h"

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