#pragma once

#include <ecs/system.h>

namespace era_engine
{

	class EditorInitSystem final : public System
	{
	public:
		EditorInitSystem(World* _world);
		~EditorInitSystem();

		void init() override;
		void update(float dt) override;
		ERA_VIRTUAL_REFLECT(System)
	};
}