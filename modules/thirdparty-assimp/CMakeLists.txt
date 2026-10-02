# Prebuilt assimp (era::assimp).

set(ASSIMP_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(ASSIMP_NAME "$<IF:$<CONFIG:Debug>,assimp-vc143-mtd,assimp-vc143-mt>")

add_library(era::assimp INTERFACE IMPORTED GLOBAL)
target_include_directories(era::assimp INTERFACE ${ASSIMP_ROOT}/include)
target_link_libraries(era::assimp INTERFACE "${ASSIMP_ROOT}/lib/${ERA_THIRDPARTY_CONFIG}/${ASSIMP_NAME}.lib")
era_add_runtime(era::assimp FILES
    "${ASSIMP_ROOT}/bin/${ERA_THIRDPARTY_CONFIG}/${ASSIMP_NAME}.dll"
    "${ASSIMP_ROOT}/bin/${ERA_THIRDPARTY_CONFIG}/${ASSIMP_NAME}.pdb")
