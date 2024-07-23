#pragma once
#include "scripting/dotnet_host.h"

#define MIN_OBJECT_SIZE 24
#define MIN_CLASS_SIZE 1

namespace era_engine::dotnet
{
    enum class visibility
    {
        v_private,
        v_private_protected,
        v_internal,
        v_protected,
        v_protected_internal,
        v_public
    };

    enum class types : uint32
    {
        t_end = 0x00,
        t_void = 0x01,
        t_boolean = 0x02,
        t_char = 0x03,
        t_i1 = 0x04,
        t_u1 = 0x05,
        t_i2 = 0x06,
        t_u2 = 0x07,
        t_i4 = 0x08,
        t_u4 = 0x09,
        t_i8 = 0x0a,
        t_u8 = 0x0b,
        t_r4 = 0x0c,
        t_r8 = 0x0d,
        t_string = 0x0e,
        t_ptr = 0x0f,
        t_by_ref = 0x10,
        t_value_type = 0x11,
        t_class = 0x12,
        t_var = 0x13,
        t_array = 0x14,
        t_generic_inst = 0x15,
        t_type_by_ref = 0x16,
        t_i = 0x18,
        t_u = 0x19,
        t_fnptr = 0x1b,
        t_object = 0x1c,
        t_sz_array = 0x1d,
        t_mvar = 0x1e,
        t_cmod_reqd = 0x1f,
        t_cmod_ppt = 0x20,
        t_internal = 0x21,
        t_modifier = 0x40,
        t_sentinel = 0x41,
        t_pinned = 0x45,
        t_enum = 0x55
    };

    enum class gc_collection_mode
    {
        collection_default = 0,
        collection_forced = 1,
        collection_optimized = 2,
        collection_aggressive = 3
    };

#if USE_NETCORE

    typedef unsigned long long gc_handle;
    typedef unsigned long long vtable_ptr;

    static inline gc_handle null_handle{};
    static inline vtable_ptr null_vtable_ptr{};

    struct type_name
    {
        const char_t* name;
        const char_t* libName;
    };

    struct delegate
    {
        type_name typeName;
        const char_t* methodName;
        const char_t* nativeName;
        void* function = nullptr;
    };

    struct core_type
    {
        gc_handle handle = null_handle; //Unused now
        uint32 size = MIN_CLASS_SIZE; // Unused now
        type_name typeName;
        std::unordered_map<const char_t*, delegate> methods;
    };

    struct reflect_storage
    {
        std::unordered_map<const char_t*, core_type> types;
    };

    extern reflect_storage storage;

    typedef std::pair<const char_t*, dotnet::delegate> delegate_pair;

#endif
}