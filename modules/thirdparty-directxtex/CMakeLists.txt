include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(DirectXTex "THIRD-PARTY-MODULE")
    add_subdirectory(${ERA_ENGINE_PATH}/modules/thirdparty-directxtex/directxtex)
    find_library(DirectXTex NAMES DirectXTex PATHS ${ERA_ENGINE_PATH}/modules/thirdparty-directxtex/directxtex)
    set_property(TARGET DirectXTex PROPERTY INCLUDE_PATH ${ERA_ENGINE_PATH}/modules/thirdparty-directxtex/directxtex)
    set_property(TARGET DirectXTex PROPERTY TARGET_TYPE "THIRD-PARTY-MODULE")
era_end(DirectXTex)