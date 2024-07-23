#include "ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe.h"
#include "ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed.h"

typedef union ffx_dof_blur_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_DOF_OPTION_MAX_RING_MERGE_LOG : 1;
        uint32_t FFX_DOF_OPTION_COMBINE_IN_PLACE : 1;
        uint32_t FFX_DOF_OPTION_REVERSE_DEPTH : 1;
    };
    uint32_t index;
} ffx_dof_blur_pass_16bit_PermutationKey;

typedef struct ffx_dof_blur_pass_16bit_PermutationInfo {
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
} ffx_dof_blur_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_dof_blur_pass_16bit_IndirectionTable[] = {
    0,
    1,
    0,
    1,
    0,
    1,
    0,
    1,
};

static const ffx_dof_blur_pass_16bit_PermutationInfo g_ffx_dof_blur_pass_16bit_PermutationInfo[] = {
    { g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_size, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_data, 1, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_CBVResourceNames, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_CBVResourceBindings, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_CBVResourceCounts, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_CBVResourceSpaces, 2, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_TextureSRVResourceNames, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_TextureSRVResourceBindings, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_TextureSRVResourceCounts, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_TextureSRVResourceSpaces, 2, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_TextureUAVResourceNames, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_TextureUAVResourceBindings, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_TextureUAVResourceCounts, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_BufferUAVResourceNames, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_BufferUAVResourceBindings, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_BufferUAVResourceCounts, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_BufferUAVResourceSpaces, 2, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_SamplerResourceNames, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_SamplerResourceBindings, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_SamplerResourceCounts, g_ffx_dof_blur_pass_16bit_78d499ddb986109e0b7acd7ca5a562fe_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_size, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_data, 1, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_CBVResourceNames, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_CBVResourceBindings, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_CBVResourceCounts, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_CBVResourceSpaces, 2, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_TextureSRVResourceNames, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_TextureSRVResourceBindings, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_TextureSRVResourceCounts, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_TextureSRVResourceSpaces, 2, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_TextureUAVResourceNames, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_TextureUAVResourceBindings, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_TextureUAVResourceCounts, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_BufferUAVResourceNames, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_BufferUAVResourceBindings, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_BufferUAVResourceCounts, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_BufferUAVResourceSpaces, 2, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_SamplerResourceNames, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_SamplerResourceBindings, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_SamplerResourceCounts, g_ffx_dof_blur_pass_16bit_668aab11fe753f9cf6ec9e5f727e9bed_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

