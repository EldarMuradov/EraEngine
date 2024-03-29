#pragma once

struct scene_manager
{
	static inline std::vector<fs::path> scenePathes;

	void loadScene(std::string_view name);

private:
	NO_COPY(scene_manager)
};