#include "ecs/base_components/transform_component.h"
#include "ecs/world.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::enumeration<TransformComponent::TransformType>("TransformType")
			(
				value("STATIC", TransformComponent::STATIC),
				value("DYNAMIC", TransformComponent::DYNAMIC)
			);

		rttr::registration::class_<TransformComponent>("TransformComponent")
			.constructor<ref<Entity::EcsData>, const trs&>()
			.constructor<ref<Entity::EcsData>, const trs&, TransformComponent::TransformType>()
			.constructor<ref<Entity::EcsData>, const vec3&, const quat&, const vec3&, TransformComponent::TransformType>()
			.property("transform", &TransformComponent::transform)
			.property("type", &TransformComponent::type);
	}

	TransformComponent::TransformComponent(ref<Entity::EcsData> _data, const trs& t)
		: Component(_data), transform(t)
	{
	}

	TransformComponent::TransformComponent(ref<Entity::EcsData> _data, const trs& t, TransformType _type)
		: Component(_data), transform(t), type(_type)
	{
	}

	TransformComponent::TransformComponent(ref<Entity::EcsData> _data, const vec3& position, const quat& rotation, const vec3& scale, TransformType _type)
		: Component(_data), transform(position, rotation, scale), type(_type)
	{
	}

	TransformComponent::~TransformComponent()
	{
	}

	trs TransformComponent::get_local_tranform() const
	{
		Entity parent = get_world()->get_entity(component_data->parent_handle);
		if (parent.is_valid())
		{
			const trs& parent_world_space_trs = parent.get_component<TransformComponent>().transform;
			return invert(parent_world_space_trs) * transform;
		}
		return trs::identity;
	}

	void TransformComponent::set_local_tranform(const trs& new_local_transform)
	{
		Entity parent = get_world()->get_entity(component_data->parent_handle);
		if (parent.is_valid())
		{
			const trs& parent_world_space_trs = parent.get_component<TransformComponent>().transform;
			transform = parent_world_space_trs * new_local_transform;
		}
		else
		{
			transform = new_local_transform;
		}
	}
}