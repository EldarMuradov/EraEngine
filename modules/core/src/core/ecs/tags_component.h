#pragma once

#include "core_api.h"

#include "core/tags_container.h"

#include "ecs/component.h"

namespace era_engine
{

	class ERA_CORE_API TagsComponent final : public Component
	{
	public:
		TagsComponent() = default;
		TagsComponent(ref<Entity::EcsData> _data);

		~TagsComponent() override;

		void add_tag(const std::string& tag);
		bool remove_tag(const std::string& tag);

		bool has_tag(const std::string& tag) const;

		void clear();
	
		const TagsContainer& get_container() const;

		ERA_VIRTUAL_REFLECT(Component)

	private:
		TagsContainer container;
	};

}