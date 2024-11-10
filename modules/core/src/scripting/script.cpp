// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "scripting/script.h"

#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<ecs::ScriptsComponent>("ScriptsComponent")
			.constructor<>()
			.constructor<ref<Entity::EcsData>, std::string_view>()
			.property("type_names", &ecs::ScriptsComponent::type_names);
	}

	ecs::ScriptsComponent::ScriptsComponent(ref<Entity::EcsData> _data, std::string_view name)
		: Component(_data)
	{
		type_names.emplace(name);
	}

	ecs::ScriptsComponent::ScriptsComponent(ref<Entity::EcsData> _data, const std::unordered_set<std::string>& name)
		: Component(_data), type_names(name)
	{
	}

	ecs::ScriptsComponent::ScriptsComponent(ref<Entity::EcsData> _data, std::unordered_set<std::string>&& name)
		: Component(_data), type_names(std::move(name))
	{
	}

	ecs::ScriptsComponent::~ScriptsComponent()
	{
	}

}
