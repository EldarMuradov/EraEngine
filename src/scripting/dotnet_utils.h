#pragma once

#include <format>
#include <scripting/dotnet_host.h>
#include <scripting/dotnet_types.h>

namespace dotnet
{
    template <typename... Funcs_>
    core_type createType(type_name typeName, Funcs_... funcs)
    {
        return core_type{ null_handle, MIN_CLASS_SIZE, typeName, {funcs...} };
    }
}