include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(base "MODULE")
    require_thirdparty_module(base rttr_core)
    require_module(base imgui)
    target_compile_definitions(base PRIVATE ERA_BASE TRACY_EXPORTS)
era_end(base)