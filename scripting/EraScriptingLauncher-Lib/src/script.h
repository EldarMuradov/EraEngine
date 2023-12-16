#pragma once
#include <string>

struct script_component
{
	script_component() = default;
	script_component(std::string name) { typeNames.push_back(name); };
	script_component(std::vector<std::string> name) : typeNames(name) {};

	std::vector<std::string> typeNames;
};