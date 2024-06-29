#pragma once

#include <scripting/dotnet_types.h>
#include <format>

namespace era_engine::dotnet
{
    template <typename... Funcs_>
    core_type createType(type_name typeName, Funcs_... funcs)
    {
        return core_type{ null_handle, MIN_CLASS_SIZE, typeName, {funcs...} };
    }
}