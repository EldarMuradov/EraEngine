#pragma once
#include <string>

struct script 
{
	script() = default;
	script(std::string name) : typeName(name) {};

	std::string typeName;
};