include(${CMAKE_SOURCE_DIR}/cmake/era_engine_common.cmake)

era_begin(EraEditor)
    declare_engine_program(EraEditor)

    require_module(EraEditor EraScriptingCPPDecls)

    require_physx(EraEditor)

    target_compile_definitions(EraEditor PRIVATE ERA_EDITOR ENABLE_MESSAGE_LOG=1)
era_end(EraEditor)