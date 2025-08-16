include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(imgui "MODULE")
    target_compile_definitions(imgui PRIVATE ERA_IMGUI)
era_end(imgui)