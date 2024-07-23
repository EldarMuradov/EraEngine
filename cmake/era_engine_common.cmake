cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /arch:AVX2 /Zi /Gy /GF /EHsc")

set(ENGINE_RELEASE_LIBS
d3d12.lib
uxtheme.lib
D3Dcompiler.lib
DXGI.lib
dxguid.lib
dxcompiler.lib
XAudio2.lib
${ERA_ENGINE_PATH}/resources/lib/Release/libfbxsdk.lib
${ERA_ENGINE_PATH}/resources/lib/Release/alembic.lib
${ERA_ENGINE_PATH}/resources/lib/Release/capnp.lib
${ERA_ENGINE_PATH}/resources/lib/Release/capnpc.lib
${ERA_ENGINE_PATH}/resources/lib/Release/kj.lib
${ERA_ENGINE_PATH}/resources/lib/Release/kj-async.lib
${ERA_ENGINE_PATH}/resources/lib/Release/zlib.lib
${ERA_ENGINE_PATH}/resources/lib/Release/libxml2.lib
${ERA_ENGINE_PATH}/resources/lib/Release/yaml-cpp.lib
${ERA_ENGINE_PATH}/resources/lib/Release/DirectXTex.lib
${ERA_ENGINE_PATH}/resources/lib/Release/nvsdk_ngx_d.lib
${ERA_ENGINE_PATH}/resources/lib/Release/nvsdk_ngx_d_dbg.lib
${ERA_ENGINE_PATH}/resources/lib/Release/nvsdk_ngx_s.lib
${ERA_ENGINE_PATH}/resources/lib/Release/nvsdk_ngx_s_dbg.lib
${ERA_ENGINE_PATH}/resources/lib/Release/nethost.lib
${ERA_ENGINE_PATH}/resources/lib/Release/libnethost.lib
${ERA_ENGINE_PATH}/modules/thirdparty_ext/CUDA/lib/x64/cuda.lib)

function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)

function(require_module project module_name)
    # Set include directories
    target_include_directories(${project} PUBLIC ${ERA_ENGINE_PATH}/modules/${module_name}/src)

    message("Adding target ${module_name} to ${project} program...")

    # Link dependencies (if any)
    target_link_directories(${project} PUBLIC modules/${module_name}/Release)
endfunction()

function(require_module_internal module module_name)
    # Set include directories
    target_include_directories(${module} PUBLIC ${ERA_ENGINE_PATH}/modules/${module_name}/src)

    message("Adding target ${module_name} to ${module} module...")

    add_subdirectory(${ERA_ENGINE_PATH}/modules/${module_name} build)

    # Link dependencies (if any)
    target_link_directories(${module} PUBLIC modules/${module_name}/Release)
endfunction()

function(era_begin name)
    message("-- Begin processing target ${name}...")
endfunction()

function(era_end name)
    message("-- End processing target ${name}.")
endfunction()

function(require_physx module_name)
    set(PHYSX_AND_BLAST_RELEASE_LIBS
    ${ERA_ENGINE_PATH}/resources/lib/Release/LowLevel_static_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/LowLevelAABB_static_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/LowLevelDynamics_static_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/SceneQuery_static_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/SimulationController_static_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/PhysXCommon_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/PhysXExtensions_static_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/PhysX_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/PhysXCooking_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/PhysXFoundation_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/PhysXPvdSDK_static_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/PhysXTask_static_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/PhysXCharacterKinematic_static_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/PhysXVehicle2_static_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/PhysXVehicle_static_64.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/NvBlast.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/NvBlastExtAssetUtils.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/NvBlastTk.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/NvBlastGlobals.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/NvBlastExtStress.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/NvBlastExtShaders.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/NvBlastExtSerialization.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/NvBlastExtAuthoring.lib
    ${ERA_ENGINE_PATH}/resources/lib/Release/NvBlastExtTkSerialization.lib)

    target_link_libraries(${module_name} PRIVATE ${PHYSX_AND_BLAST_RELEASE_LIBS})

    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/physx)
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/blast/include/lowlevel)
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/blast/include/shared/NvFoundation)
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/blast/include/extensions/assetutils)
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/blast/include/extensions/serialization)
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/blast/include/extensions/authoring)
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/blast/include/extensions/stress)
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/blast/include/extensions/authoringCommon)
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/blast/include/extensions/shaders)
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/blast/include/extensions/toolkit)
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/blast/include/extensions/lowlevel)
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_physx/src/thirdparty_physx/blast/include/extensions/globals)
endfunction()

macro(era_common_register program_name)
    target_link_libraries(${program_name} PRIVATE ${ENGINE_RELEASE_LIBS})

    target_include_directories(${program_name} PRIVATE ${ERA_ENGINE_PATH}/modules/shaders)
    target_include_directories(${program_name} PRIVATE ${ERA_ENGINE_PATH}/resources)
    target_include_directories(${program_name} PRIVATE ${ERA_ENGINE_PATH}/modules/thirdparty_ext)

    target_link_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/resources/lib/Release)
    target_link_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_ext/CUDA/lib/x64)

    target_compile_options(${program_name} PRIVATE /wd4305 /wd4244 /wd4267 /wd4099 /wd4005 /wd4804 /wd4312)

    target_include_directories(${program_name} PUBLIC 
        ${ERA_ENGINE_PATH}/modules/shaders/common
        ${ERA_ENGINE_PATH}/modules/shaders/particle_systems
        ${ERA_ENGINE_PATH}/modules/shaders/rs)

    add_definitions(/FI"${ERA_ENGINE_PATH}/resources/common/era_common.h")

    target_include_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_ext)
    target_include_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_ext/FBXSDK/include)
    target_include_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_ext/CUDA/include)
    target_include_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_ext/FidelityFX-SDK-FSR3-v3.0.3/sdk/include)
    target_include_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_ext/DLSS/include)
    target_include_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_ext/yaml-cpp/include)
    target_include_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_ext/directxtex)
    target_include_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/modules/thirdparty_ext/entt/src)
    target_include_directories(${program_name} PRIVATE ${ERA_ENGINE_PATH}/resources/common)
    target_include_directories(${program_name} PRIVATE ${ERA_ENGINE_PATH}/modules/shaders)

    target_link_options(${program_name} PRIVATE /OPT:REF /OPT:ICF)
endmacro()

function(declare_module module_name)
    file(GLOB_RECURSE SOURCES_${module_name} ${ERA_ENGINE_PATH}/modules/${module_name}/src/*.cpp)

    file(GLOB_RECURSE HEADERS_${module_name} ${ERA_ENGINE_PATH}/modules/${module_name}/src/*.h)

    set(sources_${module_name} ${SOURCES_${module_name}})
    set(headers_${module_name} ${HEADERS_${module_name}})
    set_source_files_properties(${SOURCES_${module_name}} ${HEADERS_${module_name}} PROPERTIES COMPILE_FLAGS -Od)
    add_library(${module_name} SHARED ${SOURCES_${module_name}} ${HEADERS_${module_name}})
    target_include_directories(${module_name} PUBLIC ${ERA_ENGINE_PATH}/modules/${module_name}/src)
    set_target_properties(${module_name}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${ERA_ENGINE_PATH}/_build/Release
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${ERA_ENGINE_PATH}/_build/Debug
        LIBRARY_OUTPUT_DIRECTORY ${ERA_ENGINE_PATH}/_build/Release
    )

    target_compile_definitions(${module_name} PRIVATE _UNICODE
        UNICODE
        _CRT_SECURE_NO_WARNINGS
        ENABLE_CPU_PROFILING=1
        ENABLE_DX_PROFILING=0
        ENABLE_MESSAGE_LOG=1
        PX_GPU_BROAD_PHASE=0
        SHADER_BIN_DIR=L"${ERA_ENGINE_PATH}/modules/shaders/bin/Release/"
        ENGINE_PATH=L"${ERA_ENGINE_PATH}"
        MESH_SHADER_SUPPORTED
        PX_PHYSX_STATIC_LIB
        ENABLE_FSR_WRAPPER
        FBXSDK_SHAREDd
        USE_NETCORE)

    era_common_register(${module_name})
    assign_source_group(${SOURCES_${module_name}} ${HEADERS_${module_name}})
endfunction()

function(declare_engine_program program_name)
    file(GLOB_RECURSE SOURCES_${program_name} ${ERA_ENGINE_PATH}/programs/${program_name}/src/*.cpp ${ERA_ENGINE_PATH}/modules/core/src/*.cpp)

    file(GLOB_RECURSE HEADERS_${program_name} ${ERA_ENGINE_PATH}/programs/${program_name}/src/*.h ${ERA_ENGINE_PATH}/modules/core/src/*.h)

    set(sources_${program_name} ${SOURCES_${program_name}})
    set(headers_${program_name} ${HEADERS_${program_name}})
    set_source_files_properties(${SOURCES_${program_name}} ${HEADERS_${program_name}} PROPERTIES COMPILE_FLAGS -Od)
    add_executable(${program_name} ${SOURCES_${program_name}} ${HEADERS_${program_name}})
    target_include_directories(${program_name} PRIVATE ${ERA_ENGINE_PATH}/programs/${program_name}/src)
    target_include_directories(${program_name} PRIVATE ${ERA_ENGINE_PATH}/modules/core/src)
    target_link_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/_build/Release)

    set_target_properties(${program_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ERA_ENGINE_PATH}/_build)
    set_target_properties(${program_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${ERA_ENGINE_PATH}/_build)
    era_common_register(${program_name})
    assign_source_group(${SOURCES_${program_name}} ${HEADERS_${program_name}})
    
    target_compile_definitions(${program_name} PRIVATE _UNICODE
        UNICODE
        _CRT_SECURE_NO_WARNINGS
        ENABLE_CPU_PROFILING=1
        ENABLE_DX_PROFILING=0
        PX_GPU_BROAD_PHASE=0
        SHADER_BIN_DIR=L"${ERA_ENGINE_PATH}/modules/shaders/bin/Release/"
        ENGINE_PATH=L"${ERA_ENGINE_PATH}"
        MESH_SHADER_SUPPORTED
        PX_PHYSX_STATIC_LIB
        ENABLE_FSR_WRAPPER
        FBXSDK_SHAREDd
        USE_NETCORE)

endfunction()

function(declare_support_program program_name)
    file(GLOB_RECURSE SOURCES_${program_name} ${ERA_ENGINE_PATH}/programs/${program_name}/src/*.cpp)

    file(GLOB_RECURSE HEADERS_${program_name} ${ERA_ENGINE_PATH}/programs/${program_name}/src/*.h)

    set(sources_${program_name} ${SOURCES_${program_name}})
    set(headers_${program_name} ${HEADERS_${program_name}})
    set_source_files_properties(${SOURCES_${program_name}} ${HEADERS_${program_name}} PROPERTIES COMPILE_FLAGS -Od)
    add_executable(${program_name} ${SOURCES_${program_name}} ${HEADERS_${program_name}})
    target_include_directories(${program_name} PRIVATE ${ERA_ENGINE_PATH}/programs/${program_name}/src)
    target_link_directories(${program_name} PUBLIC ${ERA_ENGINE_PATH}/_build/Release)
    set_target_properties(${program_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ERA_ENGINE_PATH}/_build)
    set_target_properties(${program_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${ERA_ENGINE_PATH}/_build)
    era_common_register(${program_name})
    assign_source_group(${SOURCES_${program_name}} ${HEADERS_${program_name}})
    target_compile_definitions(${program_name} PRIVATE _UNICODE
        UNICODE
        _CRT_SECURE_NO_WARNINGS
        ENABLE_CPU_PROFILING=1
        ENABLE_DX_PROFILING=0
        PX_GPU_BROAD_PHASE=0
        SHADER_BIN_DIR=L"${ERA_ENGINE_PATH}/modules/shaders/bin/Release/"
        ENGINE_PATH=L"${ERA_ENGINE_PATH}"
        MESH_SHADER_SUPPORTED
        PX_PHYSX_STATIC_LIB
        ENABLE_FSR_WRAPPER
        FBXSDK_SHAREDd
        USE_NETCORE)

endfunction()