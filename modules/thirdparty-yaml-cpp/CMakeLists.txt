# yaml-cpp (static): yaml-cpp::yaml-cpp.

set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)

add_subdirectory(yaml-cpp)

era_thirdparty_folder(yaml-cpp)
