# DirectXTex (static): DirectXTex. Included as <DirectXTex/DirectXTex.h>.

set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_SAMPLE OFF CACHE BOOL "" FORCE)

add_subdirectory(directxtex)

target_include_directories(DirectXTex INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/directxtex>)

era_thirdparty_folder(DirectXTex)
