#pragma once

#include "base_api.h"

#include <rttr/type>

namespace era_engine
{
	class World;

	enum class UpdateType : uint8_t
	{
		NORMAL = 0,
		FIXED
	};

	class ERA_BASE_API UpdateGroup
	{
	public:
		UpdateGroup(const char* _name, UpdateType _update_type) noexcept;

		const char* name = nullptr;
		UpdateType update_type = UpdateType::NORMAL;
	};

	class ERA_BASE_API System
	{
	public:
		System(World* _world);
		System(void* _world);

		virtual ~System();

		virtual void init();
		virtual void update(float dt);

		RTTR_ENABLE()

	protected:
		World* world = nullptr;
	};
}