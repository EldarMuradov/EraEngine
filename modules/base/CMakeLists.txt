include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(base "MODULE")
    require_thirdparty_module(base rttr_core)

    target_compile_definitions(base PRIVATE ERA_BASE)
era_end(base)