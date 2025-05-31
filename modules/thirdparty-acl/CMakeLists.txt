include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(acl_target "THIRD-PARTY-MODULE")
    add_subdirectory(${ERA_ENGINE_PATH}/modules/thirdparty-acl/acl)
    add_library(acl_target SHARED ${ERA_ENGINE_PATH}/modules/thirdparty-acl/dummy.cpp)
    set_property(TARGET acl_target PROPERTY INCLUDE_PATH ${ERA_ENGINE_PATH}/modules/thirdparty-acl/acl/includes ${ERA_ENGINE_PATH}/modules/thirdparty-acl/acl/external/rtm/includes ${ERA_ENGINE_PATH}/modules/thirdparty-acl/acl/external/catch2/include ${ERA_ENGINE_PATH}/modules/thirdparty-acl/acl/external/sjson-cpp/includes)
    set_property(TARGET acl_target PROPERTY TARGET_TYPE "THIRD-PARTY-MODULE")
era_end(acl_target)