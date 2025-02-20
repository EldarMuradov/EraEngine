#include "core/ecs/tags_component.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<TagsComponent>("TagsComponent")
			.constructor<ref<Entity::EcsData>>();
	}


	TagsComponent::TagsComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	TagsComponent::~TagsComponent()
	{
	}

	void TagsComponent::add_tag(const std::string& tag)
	{
		container.add_tag(tag);
	}

	bool TagsComponent::remove_tag(const std::string& tag)
	{
		return container.remove_tag(tag);
	}

	bool TagsComponent::has_tag(const std::string& tag) const
	{
		return container.has_tag(tag);
	}

	void TagsComponent::clear()
	{
		container.clear();
	}

	const TagsContainer& TagsComponent::get_container() const
	{
		return container;
	}



}