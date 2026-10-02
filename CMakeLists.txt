cmake_minimum_required(VERSION 3.20)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)

project(EraEngine VERSION 1.0 LANGUAGES C CXX)

set(ERA_ENGINE_PATH ${CMAKE_SOURCE_DIR})

include(cmake/common.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

enable_testing()

# Third-party
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

add_subdirectory(modules/thirdparty-entt)
add_subdirectory(modules/thirdparty-yaml-cpp)
add_subdirectory(modules/thirdparty-directxtex)
add_subdirectory(modules/thirdparty-rttr)
add_subdirectory(modules/thirdparty-gtest)
add_subdirectory(modules/thirdparty-physx)
add_subdirectory(modules/thirdparty-assimp)

era_thirdparty_folder(gtest gtest_main)

# Engine modules (order: dependencies first)
add_subdirectory(modules/imgui)
add_subdirectory(modules/base)
add_subdirectory(modules/core)
add_subdirectory(modules/physics)
add_subdirectory(modules/dynx)
add_subdirectory(modules/simple_motion_matching)
add_subdirectory(modules/game_core)

add_subdirectory(apps/editor)
add_subdirectory(apps/example_game)
add_subdirectory(apps/unittests)
