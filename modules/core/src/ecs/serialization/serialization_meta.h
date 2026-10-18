#pragma once

#include "core_api.h"

#include <rttr/type>

namespace era_engine
{
    enum class SerializationMeta : uint8
    {
        // Property: visible in reflection, never saved or loaded.
        NOT_SERIALIZABLE_PROPERTY,
        // Class: runtime-only component, skipped on save and not offered in editor.
        NOT_SERIALIZABLE_CLASS,
    };

    inline bool is_serializable(const rttr::property& property)
    {
        const rttr::variant meta = property.get_metadata(SerializationMeta::NOT_SERIALIZABLE_PROPERTY);
        return !(meta.is_valid() && meta.to_bool());
    }

    inline bool is_serializable(const rttr::type& type)
    {
        const rttr::variant meta = type.get_metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS);
        return !(meta.is_valid() && meta.to_bool());
    }
}
