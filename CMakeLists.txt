cmake_minimum_required(VERSION 3.12)

set(ERA_ENGINE_PATH ${CMAKE_SOURCE_DIR})

include(cmake/common.cmake)

project(EraEngine VERSION 1.0)

add_subdirectory(modules/thirdparty-entt)
add_subdirectory(modules/thirdparty-yaml-cpp)
add_subdirectory(modules/thirdparty-directxtex)
add_subdirectory(modules/thirdparty-rttr)

add_subdirectory(modules/base)
add_subdirectory(modules/core)
add_subdirectory(modules/physics)

add_subdirectory(apps/editor)
add_subdirectory(apps/example_game)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)