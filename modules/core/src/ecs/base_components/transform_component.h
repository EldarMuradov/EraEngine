#pragma once

#include "core/math.h"

#include "ecs/component.h"

namespace era_engine
{

	class TransformComponent final : public Component
	{
	public:
		enum TransformType : uint8_t
		{
			STATIC,
			DYNAMIC
		};

	public:
		TransformComponent() = default;
		TransformComponent(ref<Entity::EcsData> _data, const trs& t = trs::identity);
		TransformComponent(ref<Entity::EcsData> _data, const trs& t, TransformType _type);
		TransformComponent(ref<Entity::EcsData> _data, const vec3& position, const quat& rotation, const vec3& scale = vec3(1.f, 1.f, 1.f), TransformType _type = STATIC);

		~TransformComponent() override;

		ERA_VIRTUAL_REFLECT(Component)

	public:
		trs transform = trs::identity;
		TransformType type = STATIC;
	};

}