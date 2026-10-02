era_module(base
    PUBLIC  imgui RTTR::Core)

# base.dll owns the Tracy client; every consumer imports it.
target_compile_definitions(base PRIVATE TRACY_EXPORTS INTERFACE TRACY_IMPORTS)
