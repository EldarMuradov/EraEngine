# Prebuilt PhysX 5 SDK (era::physx) and NvBlast (era::blast).
# Debug binaries are the PhysX "checked" build; other configurations use Release binaries.

set(PHYSX_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# --- PhysX -----------------------------------------------------------------------------------------

set(PHYSX_LIBRARIES
    LowLevel_static_64
    LowLevelAABB_static_64
    LowLevelDynamics_static_64
    PhysX_64
    PhysXCharacterKinematic_static_64
    PhysXPvdSDK_static_64
    PhysXCooking_64
    PhysXCommon_64
    PhysXExtensions_static_64
    PhysXFoundation_64
    PhysXTask_static_64
    PhysXVehicle_static_64
    PVDRuntime_64
    SceneQuery_static_64
    SimulationController_static_64
    SnippetUtils_static_64
    PhysXArticulationGpu_static_64
    PhysXBroadphaseGpu_static_64
    PhysXCommonGpu_static_64
    PhysXCudaContextManager_static_64
    PhysXGpu_64
    PhysXNarrowphaseGpu_static_64
    PhysXSimulationControllerGpu_static_64
    PhysXSolverGpu_static_64
)
list(TRANSFORM PHYSX_LIBRARIES PREPEND "${PHYSX_ROOT}/lib/${ERA_THIRDPARTY_CONFIG}/")
list(TRANSFORM PHYSX_LIBRARIES APPEND ".lib")

add_library(era::physx INTERFACE IMPORTED GLOBAL)
target_include_directories(era::physx INTERFACE ${PHYSX_ROOT}/src/physx)
target_link_libraries(era::physx INTERFACE ${PHYSX_LIBRARIES})
era_add_runtime(era::physx DIRS "${PHYSX_ROOT}/bin/${ERA_THIRDPARTY_CONFIG}")

# --- NvBlast (Release binaries only) -----------------------------------------------------------------

set(BLAST_LIBRARIES
    NvBlast
    NvBlastExtAssetUtils
    NvBlastExtAuthoring
    NvBlastExtSerialization
    NvBlastExtShaders
    NvBlastExtStress
    NvBlastExtTkSerialization
    NvBlastGlobals
    NvBlastTk
)
set(BLAST_RUNTIME ${BLAST_LIBRARIES})
list(TRANSFORM BLAST_LIBRARIES PREPEND "${PHYSX_ROOT}/blast/bin/")
list(TRANSFORM BLAST_LIBRARIES APPEND ".lib")
list(TRANSFORM BLAST_RUNTIME PREPEND "${PHYSX_ROOT}/blast/bin/")
list(TRANSFORM BLAST_RUNTIME APPEND ".dll")

set(BLAST_INCLUDE ${PHYSX_ROOT}/src/blast/include)

add_library(era::blast INTERFACE IMPORTED GLOBAL)
target_include_directories(era::blast INTERFACE
    ${BLAST_INCLUDE}
    ${BLAST_INCLUDE}/shared/NvFoundation
    ${BLAST_INCLUDE}/lowlevel
    ${BLAST_INCLUDE}/globals
    ${BLAST_INCLUDE}/toolkit
    ${BLAST_INCLUDE}/extensions
    ${BLAST_INCLUDE}/extensions/assetutils
    ${BLAST_INCLUDE}/extensions/serialization
    ${BLAST_INCLUDE}/extensions/stress
    ${BLAST_INCLUDE}/extensions/shaders
    ${BLAST_INCLUDE}/extensions/authoring
    ${BLAST_INCLUDE}/extensions/authoringCommon
)
target_link_libraries(era::blast INTERFACE ${BLAST_LIBRARIES})
era_add_runtime(era::blast FILES ${BLAST_RUNTIME})
