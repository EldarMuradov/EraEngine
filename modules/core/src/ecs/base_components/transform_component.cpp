#include "ecs/base_components/transform_component.h"
#include "ecs/world.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::enumeration<TransformComponent::TransformType>("TransformType")
			(
				value("STATIC", TransformComponent::STATIC),
				value("DYNAMIC", TransformComponent::DYNAMIC)
			);

		registration::class_<TransformComponent>("TransformComponent")
			.constructor<ref<Entity::EcsData>, const trs&>()
			.constructor<ref<Entity::EcsData>, const trs&, TransformComponent::TransformType>()
			.constructor<ref<Entity::EcsData>, const vec3&, const quat&, const vec3&, TransformComponent::TransformType>()
			.property("transform", &TransformComponent::get_world_transform, &TransformComponent::set_world_transform)
			.property("local_transform", &TransformComponent::get_local_transform, &TransformComponent::set_local_transform)
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

	const trs& TransformComponent::get_local_transform() const
	{
		return local_transform;
	}

	void TransformComponent::set_local_transform(const trs& new_local_transform)
	{
		local_transform = new_local_transform;
	}

	const trs& TransformComponent::get_world_transform() const
	{
		return transform;
	}

	void TransformComponent::set_world_transform(const trs& new_world_transform)
	{
		transform = new_world_transform;

		sync_local_transform();
	}

	const vec3& TransformComponent::get_world_position() const
	{
		return transform.position;
	}

	void TransformComponent::set_world_position(const vec3& new_world_position)
	{
		transform.position = new_world_position;

		sync_local_transform();
	}

	const quat& TransformComponent::get_world_rotation() const
	{
		return transform.rotation;
	}

	void TransformComponent::set_world_rotation(const quat& new_world_rotation)
	{
		transform.rotation = new_world_rotation;

		sync_local_transform();
	}

	void TransformComponent::sync_local_transform()
	{
		Entity parent = get_world()->get_entity(component_data->parent_handle);
		if (parent.is_valid())
		{
			const trs& parent_world_space_trs = parent.get_component<TransformComponent>()->get_world_transform();
			local_transform = invert(parent_world_space_trs) * transform;
		}
	}

}