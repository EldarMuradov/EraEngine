include(${CMAKE_SOURCE_DIR}/cmake/era_engine_common.cmake)

era_begin(EraLauncher)
    declare_support_program(EraLauncher)

    require_module(EraLauncher EraScriptingCPPDecls)

    target_compile_definitions(EraLauncher PRIVATE ERA_LAUNCHER)
era_end(EraLauncher)