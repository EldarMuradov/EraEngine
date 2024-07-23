#include "ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98.h"
#include "ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88.h"

typedef union ffx_dof_composite_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_DOF_OPTION_MAX_RING_MERGE_LOG : 1;
        uint32_t FFX_DOF_OPTION_COMBINE_IN_PLACE : 1;
        uint32_t FFX_DOF_OPTION_REVERSE_DEPTH : 1;
    };
    uint32_t index;
} ffx_dof_composite_pass_wave64_PermutationKey;

typedef struct ffx_dof_composite_pass_wave64_PermutationInfo {
    const uint32_t       blobSize;
    const unsigned char* blobData;


    const uint32_t  numConstantBuffers;
    const char**    constantBufferNames;
    const uint32_t* constantBufferBindings;
    const uint32_t* constantBufferCounts;
    const uint32_t* constantBufferSpaces;

    const uint32_t  numSRVTextures;
    const char**    srvTextureNames;
    const uint32_t* srvTextureBindings;
    const uint32_t* srvTextureCounts;
    const uint32_t* srvTextureSpaces;

    const uint32_t  numUAVTextures;
    const char**    uavTextureNames;
    const uint32_t* uavTextureBindings;
    const uint32_t* uavTextureCounts;
    const uint32_t* uavTextureSpaces;

    const uint32_t  numSRVBuffers;
    const char**    srvBufferNames;
    const uint32_t* srvBufferBindings;
    const uint32_t* srvBufferCounts;
    const uint32_t* srvBufferSpaces;

    const uint32_t  numUAVBuffers;
    const char**    uavBufferNames;
    const uint32_t* uavBufferBindings;
    const uint32_t* uavBufferCounts;
    const uint32_t* uavBufferSpaces;

    const uint32_t  numSamplers;
    const char**    samplerNames;
    const uint32_t* samplerBindings;
    const uint32_t* samplerCounts;
    const uint32_t* samplerSpaces;

    const uint32_t  numRTAccelerationStructures;
    const char**    rtAccelerationStructureNames;
    const uint32_t* rtAccelerationStructureBindings;
    const uint32_t* rtAccelerationStructureCounts;
    const uint32_t* rtAccelerationStructureSpaces;
} ffx_dof_composite_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_dof_composite_pass_wave64_IndirectionTable[] = {
    0,
    0,
    1,
    1,
    0,
    0,
    1,
    1,
};

static const ffx_dof_composite_pass_wave64_PermutationInfo g_ffx_dof_composite_pass_wave64_PermutationInfo[] = {
    { g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_size, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_data, 1, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_CBVResourceNames, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_CBVResourceBindings, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_CBVResourceCounts, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_CBVResourceSpaces, 3, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_TextureSRVResourceNames, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_TextureSRVResourceBindings, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_TextureSRVResourceCounts, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_TextureSRVResourceSpaces, 3, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_TextureUAVResourceNames, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_TextureUAVResourceBindings, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_TextureUAVResourceCounts, g_ffx_dof_composite_pass_wave64_998aa298ec2dd6ba3a99190b64e83a98_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_size, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_data, 1, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_CBVResourceNames, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_CBVResourceBindings, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_CBVResourceCounts, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_CBVResourceSpaces, 2, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_TextureSRVResourceNames, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_TextureSRVResourceBindings, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_TextureSRVResourceCounts, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_TextureSRVResourceSpaces, 3, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_TextureUAVResourceNames, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_TextureUAVResourceBindings, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_TextureUAVResourceCounts, g_ffx_dof_composite_pass_wave64_6d1428a206f297497197e10d91122b88_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

