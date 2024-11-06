#include "ecs/entity.h"
#include "ecs/world.h"
#include "ecs/component.h"

namespace era_engine
{

	Entity Entity::Null = Entity(make_ref<Entity::EcsData>(Entity::NullHandle, (entt::registry*)nullptr));

	Entity::Entity(const Entity& _entity) noexcept
		: internal_data(_entity.internal_data)
	{
	}

	Entity::Entity(Entity&& _entity) noexcept
	{
		internal_data = std::move(_entity.internal_data);
	}


	Entity::Entity(ref<EcsData> _data)
		: internal_data(_data)
	{
	}

	Entity::~Entity() noexcept
	{
	}

	Entity& Entity::operator=(const Entity& _entity)
	{
		internal_data = _entity.internal_data;
		return *this;
	}

	Entity& Entity::operator=(Entity&& _entity)
	{
		internal_data = std::move(_entity.internal_data);
		return *this;
	}

	bool Entity::operator==(const Entity& _other) const
	{
		return internal_data == _other.internal_data;
	}

	bool Entity::operator!=(const Entity& _other) const
	{
		return !(*this == _other);
	}

	bool Entity::operator==(Entity::Handle _handle) const
	{
		return internal_data->entity_handle == _handle;
	}

	bool Entity::is_valid() const noexcept
	{
		return internal_data != nullptr && 
			   internal_data->entity_handle != Entity::NullHandle && 
			   internal_data->registry != nullptr;
	}

	World* Entity::get_world() const
	{
		return worlds[internal_data->registry];
	}

	Entity::Handle Entity::get_handle() const
	{
		return internal_data->entity_handle;
	}

}