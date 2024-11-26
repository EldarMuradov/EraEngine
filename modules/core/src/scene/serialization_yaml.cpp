// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "scene/serialization_yaml.h"

#include "core/yaml.h"

#include "editor/file_dialog.h"

#include "asset/file_registry.h"

#include "terrain/terrain.h"
#include "terrain/grass.h"
#include "terrain/water.h"
#include "terrain/proc_placement.h"

#include "application.h"

namespace YAML
{
	using namespace era_engine;

}

void era_engine::serializeSceneToYAMLFile(editor_scene& scene, const renderer_settings& rendererSettings)
{
}

bool era_engine::deserializeSceneFromYAMLFile(editor_scene& scene, renderer_settings& rendererSettings, std::string& environmentName)
{
	return false;
}

bool era_engine::deserializeSceneFromCurrentYAMLFile(const fs::path& path, editor_scene& scene, renderer_settings& rendererSettings, std::string& environmentName)
{
	return false;
}
