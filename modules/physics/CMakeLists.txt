include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(physics "MODULE")
    require_module(physics core)
    require_physx(physics)
    target_compile_definitions(physics PRIVATE ERA_PHYSICS)
era_end(physics)