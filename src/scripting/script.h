// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include <unordered_set>

namespace era_engine::ecs
{
	struct scripts_component
	{
		scripts_component() = default;
		scripts_component(std::string name) { typeNames.emplace(name); };
		scripts_component(const std::unordered_set<std::string>& name) : typeNames(name) {};
		scripts_component(std::unordered_set<std::string>&& name) : typeNames(std::move(name)) {};

		std::unordered_set<std::string> typeNames;
	};
}