#include "ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328.h"
#include "ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607.h"

typedef union ffx_dof_composite_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_DOF_OPTION_MAX_RING_MERGE_LOG : 1;
        uint32_t FFX_DOF_OPTION_COMBINE_IN_PLACE : 1;
        uint32_t FFX_DOF_OPTION_REVERSE_DEPTH : 1;
    };
    uint32_t index;
} ffx_dof_composite_pass_16bit_PermutationKey;

typedef struct ffx_dof_composite_pass_16bit_PermutationInfo {
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
} ffx_dof_composite_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_dof_composite_pass_16bit_IndirectionTable[] = {
    1,
    1,
    0,
    0,
    1,
    1,
    0,
    0,
};

static const ffx_dof_composite_pass_16bit_PermutationInfo g_ffx_dof_composite_pass_16bit_PermutationInfo[] = {
    { g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_size, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_data, 1, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_CBVResourceNames, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_CBVResourceBindings, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_CBVResourceCounts, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_CBVResourceSpaces, 2, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_TextureSRVResourceNames, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_TextureSRVResourceBindings, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_TextureSRVResourceCounts, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_TextureSRVResourceSpaces, 3, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_TextureUAVResourceNames, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_TextureUAVResourceBindings, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_TextureUAVResourceCounts, g_ffx_dof_composite_pass_16bit_dd468cf9ee0d9c7fbcfedbbeba69c328_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_size, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_data, 1, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_CBVResourceNames, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_CBVResourceBindings, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_CBVResourceCounts, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_CBVResourceSpaces, 3, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_TextureSRVResourceNames, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_TextureSRVResourceBindings, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_TextureSRVResourceCounts, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_TextureSRVResourceSpaces, 3, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_TextureUAVResourceNames, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_TextureUAVResourceBindings, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_TextureUAVResourceCounts, g_ffx_dof_composite_pass_16bit_b526a3883c18ab05a1cd6f7b966da607_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

