// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include <pch.h>
#include "runtime.h"
#include <core/imgui.h>
#include <scene/serialization_yaml.h>
#include <scene/scene.h>
#include <rendering/main_renderer.h>

void runtime::initialize(editor_scene* scene, main_renderer* renderer)
{
	this->scene = scene;
	this->renderer = renderer;
	cameraController.initialize(&scene->camera);

	std::string environmentName;
	if (deserializeSceneFromCurrentYAMLFile("assets/sample_scene.sc", *scene, renderer->settings, environmentName))
	{
		scene->stop();

		scene->environment.setFromTexture(environmentName);
		scene->environment.forceUpdate(this->scene->sun.direction);
		renderer->pathTracer.resetRendering();

		::std::cout << "Scene loaded\n";
	}
	else
		::std::cout << "Error\n";
}

void runtime::update()
{
	CPU_PROFILE_BLOCK("Runtime update");

	cameraController.update(renderer->renderWidth, renderer->renderHeight);

	::std::cout << "Runtime update\n";
}