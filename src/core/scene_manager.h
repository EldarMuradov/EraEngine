#pragma once

namespace era_engine
{
	struct scene_manager
	{
		void loadScene(std::string_view name);

		static inline std::vector<fs::path> scenePathes;

	private:
		NO_COPY(scene_manager)
	};
}