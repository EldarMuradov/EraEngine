include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(assets_compiler "APP")
    require_thirdparty_module(assets_compiler EnTT)
    require_thirdparty_module(assets_compiler yaml-cpp)
    require_thirdparty_module(assets_compiler rttr_core)
    require_thirdparty_module(assets_compiler DirectXTex)
    require_module(assets_compiler base)
    require_module(assets_compiler core)

    target_include_directories(assets_compiler PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty-imgui/imgui)
era_end(assets_compiler)