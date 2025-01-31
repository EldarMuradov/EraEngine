include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(EnTT "THIRD-PARTY-MODULE")
    add_subdirectory(${ERA_ENGINE_PATH}/modules/thirdparty-entt/entt)
    find_library(EnTT NAMES EnTT PATHS ${ERA_ENGINE_PATH}/modules/thirdparty-entt/entt)
    set_property(TARGET EnTT PROPERTY INCLUDE_PATH ${ERA_ENGINE_PATH}/modules/thirdparty-entt/entt/src)
    set_property(TARGET EnTT PROPERTY TARGET_TYPE "THIRD-PARTY-MODULE")
era_end(EnTT)