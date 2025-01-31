include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(yaml-cpp "THIRD-PARTY-MODULE")
    add_subdirectory(${ERA_ENGINE_PATH}/modules/thirdparty-yaml-cpp/yaml-cpp)
    find_library(yaml-cpp NAMES yaml-cpp PATHS ${ERA_ENGINE_PATH}/modules/thirdparty-yaml-cpp/yaml-cpp)
    set_property(TARGET yaml-cpp PROPERTY INCLUDE_PATH ${ERA_ENGINE_PATH}/modules/thirdparty-yaml-cpp/yaml-cpp/include)
    set_property(TARGET yaml-cpp PROPERTY NEED_COPY "false")
    set_property(TARGET yaml-cpp PROPERTY TARGET_TYPE "THIRD-PARTY-MODULE")
era_end(yaml-cpp)