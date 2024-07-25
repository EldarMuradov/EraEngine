cmake_minimum_required(VERSION 3.12)
set(ERA_ENGINE_PATH ${CMAKE_SOURCE_DIR})
include(cmake/era_engine_common.cmake)

project(EraEngine VERSION 1.0)

era_begin(EraEngine)
    # modules
    add_subdirectory(modules/EraScriptingCPPDecls)

    # programs
    add_subdirectory(apps/EraEditor)
    add_subdirectory(apps/EraRuntime)
    add_subdirectory(apps/EraUnitTests)
    add_subdirectory(apps/EraLauncher)
era_end(EraEngine)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
