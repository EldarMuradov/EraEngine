#include <pch.h>
#include "runtime.h"
#include <core/imgui.h>

void runtime::initialize(editor_scene* scene, main_renderer* renderer)
{
	this->scene = scene;
	this->renderer = renderer;
	cameraController.initialize(&scene->camera);
}

void runtime::update()
{
	CPU_PROFILE_BLOCK("Update runtime");

	cameraController.update(renderer->renderWidth, renderer->renderHeight);
}