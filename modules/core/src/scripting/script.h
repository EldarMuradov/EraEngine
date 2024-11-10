// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "ecs/component.h"

namespace era_engine::ecs
{
	class ScriptsComponent : public Component
	{
	public:
		ScriptsComponent() = default;
		ScriptsComponent(ref<Entity::EcsData> _data, std::string_view name);
		ScriptsComponent(ref<Entity::EcsData> _data, const std::unordered_set<std::string>& name);
		ScriptsComponent(ref<Entity::EcsData> _data, std::unordered_set<std::string>&& name);
		virtual ~ScriptsComponent();

		ERA_VIRTUAL_REFLECT(Component)
	public:
		std::unordered_set<std::string> type_names;
	};
}