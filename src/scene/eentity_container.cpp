#include "pch.h"
#include "eentity_container.h"

std::mutex eentity_container::sync;
std::unordered_map<eentity*, eentity_container::eentity_node> eentity_container::container;