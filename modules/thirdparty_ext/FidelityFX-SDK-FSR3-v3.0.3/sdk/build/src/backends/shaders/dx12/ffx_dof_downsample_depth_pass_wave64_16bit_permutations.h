#include "ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d.h"
#include "ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c.h"

typedef union ffx_dof_downsample_depth_pass_wave64_16bit_PermutationKey {
    struct {
        uint32_t FFX_DOF_OPTION_MAX_RING_MERGE_LOG : 1;
        uint32_t FFX_DOF_OPTION_COMBINE_IN_PLACE : 1;
        uint32_t FFX_DOF_OPTION_REVERSE_DEPTH : 1;
    };
    uint32_t index;
} ffx_dof_downsample_depth_pass_wave64_16bit_PermutationKey;

typedef struct ffx_dof_downsample_depth_pass_wave64_16bit_PermutationInfo {
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
} ffx_dof_downsample_depth_pass_wave64_16bit_PermutationInfo;

static const uint32_t g_ffx_dof_downsample_depth_pass_wave64_16bit_IndirectionTable[] = {
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
};

static const ffx_dof_downsample_depth_pass_wave64_16bit_PermutationInfo g_ffx_dof_downsample_depth_pass_wave64_16bit_PermutationInfo[] = {
    { g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_size, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_data, 1, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_CBVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_CBVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_CBVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_CBVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_TextureSRVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_TextureSRVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_TextureSRVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_TextureSRVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_TextureUAVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_TextureUAVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_TextureUAVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_BufferUAVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_BufferUAVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_BufferUAVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_BufferUAVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_SamplerResourceNames, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_SamplerResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_SamplerResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_16bit_712e0b53ac2f8a4f3cc97f4f2fe19c7d_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_size, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_data, 1, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_CBVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_CBVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_CBVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_CBVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_TextureSRVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_TextureSRVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_TextureSRVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_TextureSRVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_TextureUAVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_TextureUAVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_TextureUAVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_BufferUAVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_BufferUAVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_BufferUAVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_BufferUAVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_SamplerResourceNames, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_SamplerResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_SamplerResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_16bit_1a05b75faf6e41235952acfec40d196c_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

