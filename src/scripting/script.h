// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include <unordered_set>

struct script_component
{
	script_component() = default;
	script_component(std::string name) { typeNames.emplace(name); };
	script_component(const std::unordered_set<std::string>& name) : typeNames(name) {};
	script_component(std::unordered_set<std::string>&& name) : typeNames(std::move(name)) {};

	std::unordered_set<std::string> typeNames;
};