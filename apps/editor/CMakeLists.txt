include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

era_begin(editor "APP")
    require_thirdparty_module(editor EnTT)
    require_thirdparty_module(editor yaml-cpp)
    require_thirdparty_module(editor rttr_core)
    require_thirdparty_module(editor DirectXTex)
    require_module(editor base)
    require_module(editor core)
    require_module(editor physics)

    require_physx(editor)

    target_include_directories(editor PUBLIC modules/thirdparty-imgui/imgui)
era_end(editor)