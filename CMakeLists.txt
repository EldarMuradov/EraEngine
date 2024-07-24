cmake_minimum_required(VERSION 3.12)
set(ERA_ENGINE_PATH ${CMAKE_SOURCE_DIR})
include(cmake/era_engine_common.cmake)

project(EraEngine VERSION 1.0)

era_begin(EraEngine)
    # modules
    add_subdirectory(modules/EraScriptingCPPDecls)

    # programs
    add_subdirectory(programs/EraEditor)
    add_subdirectory(programs/EraRuntime)
    add_subdirectory(programs/EraUnitTests)
    add_subdirectory(programs/EraLauncher)
era_end(EraEngine)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
