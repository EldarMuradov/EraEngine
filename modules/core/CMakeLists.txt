include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(core "MODULE")
    require_module(core base)
    require_thirdparty_module(core EnTT)
    require_thirdparty_module(core yaml-cpp)
    require_thirdparty_module(core rttr_core)
    require_thirdparty_module(core DirectXTex)

    target_include_directories(core PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty-imgui/imgui)
    target_compile_definitions(core PRIVATE ERA_CORE)
era_end(core)