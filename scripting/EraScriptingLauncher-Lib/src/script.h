#pragma once
#include <string>

struct script_component
{
	script_component() = default;
	script_component(std::string name) { typeNames.emplace(name); };
	script_component(std::set<std::string>& name) : typeNames(name) {};
	script_component(std::set<std::string>&& name) : typeNames(std::move(name)) {};

	std::set<std::string> typeNames;
};