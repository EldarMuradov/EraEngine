#include "ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45.h"
#include "ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541.h"

typedef union ffx_dof_blur_pass_wave64_16bit_PermutationKey {
    struct {
        uint32_t FFX_DOF_OPTION_MAX_RING_MERGE_LOG : 1;
        uint32_t FFX_DOF_OPTION_COMBINE_IN_PLACE : 1;
        uint32_t FFX_DOF_OPTION_REVERSE_DEPTH : 1;
    };
    uint32_t index;
} ffx_dof_blur_pass_wave64_16bit_PermutationKey;

typedef struct ffx_dof_blur_pass_wave64_16bit_PermutationInfo {
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
} ffx_dof_blur_pass_wave64_16bit_PermutationInfo;

static const uint32_t g_ffx_dof_blur_pass_wave64_16bit_IndirectionTable[] = {
    1,
    0,
    1,
    0,
    1,
    0,
    1,
    0,
};

static const ffx_dof_blur_pass_wave64_16bit_PermutationInfo g_ffx_dof_blur_pass_wave64_16bit_PermutationInfo[] = {
    { g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_size, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_data, 1, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_CBVResourceNames, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_CBVResourceBindings, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_CBVResourceCounts, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_CBVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_TextureSRVResourceNames, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_TextureSRVResourceBindings, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_TextureSRVResourceCounts, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_TextureSRVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_TextureUAVResourceNames, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_TextureUAVResourceBindings, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_TextureUAVResourceCounts, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_BufferUAVResourceNames, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_BufferUAVResourceBindings, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_BufferUAVResourceCounts, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_BufferUAVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_SamplerResourceNames, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_SamplerResourceBindings, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_SamplerResourceCounts, g_ffx_dof_blur_pass_wave64_16bit_f775cb7930d53f954bdb9a6c2ffd8e45_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_size, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_data, 1, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_CBVResourceNames, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_CBVResourceBindings, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_CBVResourceCounts, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_CBVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_TextureSRVResourceNames, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_TextureSRVResourceBindings, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_TextureSRVResourceCounts, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_TextureSRVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_TextureUAVResourceNames, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_TextureUAVResourceBindings, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_TextureUAVResourceCounts, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_BufferUAVResourceNames, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_BufferUAVResourceBindings, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_BufferUAVResourceCounts, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_BufferUAVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_SamplerResourceNames, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_SamplerResourceBindings, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_SamplerResourceCounts, g_ffx_dof_blur_pass_wave64_16bit_4760facb332648c7d0f4524a9f1fa541_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

