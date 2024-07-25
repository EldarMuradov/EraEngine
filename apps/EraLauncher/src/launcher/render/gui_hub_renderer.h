#pragma once

namespace era_engine::launcher
{
	class gui_hub_renderer final
	{
		NO_COPY(gui_hub_renderer)
	public:
		static void init(ref<struct era_hub> hub);
		static void finalize();

		static void beginFrame();
		static void endFrame();
	};
}