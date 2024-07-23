#pragma once

#include <vector>
#include <string>


#ifdef _WIN32
  #define SHADERS_EXPORT __declspec(dllexport)
#else
  #define SHADERS_EXPORT
#endif

SHADERS_EXPORT void shaders();
SHADERS_EXPORT void shaders_print_vector(const std::vector<std::string> &strings);
