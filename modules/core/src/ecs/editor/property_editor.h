#pragma once

#include "core_api.h"

#include <rttr/type>

namespace era_engine
{
    class World;

    class ERA_CORE_API PropertyEditor final
    {
        PropertyEditor() = delete;

    public:
        static bool edit_object(const rttr::instance& object, World* world = nullptr);
        static bool edit_value(const char* label, rttr::variant& value, World* world = nullptr, bool read_only = false);
    };
}
