include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(example_game "APP")
    require_module(example_game base)
    require_module(example_game core)
    require_module(example_game physics)

    require_physx(example_game)

    target_include_directories(example_game PUBLIC modules/thirdparty-imgui/imgui)
era_end(example_game)