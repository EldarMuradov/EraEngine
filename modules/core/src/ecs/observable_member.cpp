#include "ecs/observable_member.h"
#include "ecs/base_components/transform_component.h"
#include "ecs/world.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<ObservableMemberChangedFlagComponent>("ObservableMemberChangedFlagComponent")
			.constructor<ref<Entity::EcsData>>();
	}

	ObservableMemberChangedFlagComponent::ObservableMemberChangedFlagComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	std::vector<ObservableBase*> ObservableStorage::observable_ptrs;
	static std::mutex observable_sync;

	ObservableBase::ObservableBase()
	{
		std::lock_guard<std::mutex> _lock{ observable_sync };
		ObservableStorage::observable_ptrs.push_back(this);
	}

	ObservableBase::~ObservableBase()
	{
		std::lock_guard<std::mutex> _lock{ observable_sync };
		auto iter_to_remove = std::find(ObservableStorage::observable_ptrs.begin(), ObservableStorage::observable_ptrs.end(), this);
		if (iter_to_remove != ObservableStorage::observable_ptrs.end())
		{
			ObservableStorage::observable_ptrs.erase(iter_to_remove);
		}
	}

	void ObservableBase::set_component(const ComponentPtr& _component_ptr)
	{
		component_ptr = _component_ptr;
	}

	void ObservableBase::notify()
	{
		++changes_count;

		if (!component_ptr.is_empty())
		{
			Entity owner = component_ptr.get()->get_entity();
			owner.add_component<ObservableMemberChangedFlagComponent>();
		}
	}

	bool ObservableBase::is_changed() const
	{
		return changes_count != old_changes_count;
	}

	void ObservableBase::sync_changes()
	{
		old_changes_count = changes_count;
	}

	void ObservableStorage::sync_all_changes(World* world)
	{
		std::vector<Entity::Handle> entityes_to_reset_state;
		for (auto [entity_handle, _1, _2] : world->group(components_group<ObservableMemberChangedFlagComponent, TransformComponent>).each())
		{
			entityes_to_reset_state.push_back(entity_handle);
		}

		//for (ObservableBase* observable : observable_ptrs)
		//{
		//	observable->sync_changes();
		//}

		for (Entity::Handle handle : entityes_to_reset_state)
		{
			Entity owner = world->get_entity(handle);
			if (owner.has_component<ObservableMemberChangedFlagComponent>())
			{
				owner.remove_component<ObservableMemberChangedFlagComponent>();
			}
		}
	}

}