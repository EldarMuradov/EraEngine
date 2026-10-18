#pragma once

#include "core_api.h"

#include "ecs/entity.h"

namespace era_engine
{
    class ERA_CORE_API PrefabSerializer final
    {
        PrefabSerializer() = delete;

    public:
        static std::string save_to_string(const std::vector<Entity>& top_level_entities);
        static bool save_to_file(const std::vector<Entity>& top_level_entities, const fs::path& path);
    };
}
