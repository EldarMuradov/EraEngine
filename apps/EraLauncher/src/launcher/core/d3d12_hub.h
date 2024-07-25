#pragma once

#include "launcher/render/hub_renderer.h"

namespace era_engine::launcher
{
	struct era_hub
	{
		era_hub();
		~era_hub();

		void render();
	private:
		ref<d3d12_hub_renderer> renderer = nullptr;
	};
}