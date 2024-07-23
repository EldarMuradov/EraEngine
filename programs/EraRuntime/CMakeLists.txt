include(${CMAKE_SOURCE_DIR}/cmake/era_engine_common.cmake)

era_begin(EraRuntime)
    declare_engine_program(EraRuntime)

    require_module(EraRuntime EraScriptingCPPDecls)

    require_physx(EraRuntime)

    target_compile_definitions(EraRuntime PRIVATE ERA_RUNTIME)
era_end(EraRuntime)