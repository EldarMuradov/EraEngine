#include "ecs/base_components/transform_component.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<TransformComponent>("TransformComponent")
			.constructor<ref<Entity::EcsData>, const trs&>()
			.constructor<ref<Entity::EcsData>, const vec3&, const quat&, const vec3&>()
			.property("transform", &TransformComponent::transform);
	}

	TransformComponent::TransformComponent(ref<Entity::EcsData> _data, const trs& t)
		: Component(_data), transform(t)
	{
	}

	TransformComponent::TransformComponent(ref<Entity::EcsData> _data, const vec3& position, const quat& rotation, const vec3& scale)
		: Component(_data), transform(position, rotation, scale)
	{
	}

	TransformComponent::~TransformComponent()
	{
	}

}