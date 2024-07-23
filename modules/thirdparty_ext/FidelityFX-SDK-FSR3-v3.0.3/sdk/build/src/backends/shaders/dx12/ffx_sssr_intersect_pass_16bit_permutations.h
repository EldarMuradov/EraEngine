#include "ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230.h"
#include "ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47.h"

typedef union ffx_sssr_intersect_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_SSSR_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_sssr_intersect_pass_16bit_PermutationKey;

typedef struct ffx_sssr_intersect_pass_16bit_PermutationInfo {
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
} ffx_sssr_intersect_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_sssr_intersect_pass_16bit_IndirectionTable[] = {
    0,
    1,
};

static const ffx_sssr_intersect_pass_16bit_PermutationInfo g_ffx_sssr_intersect_pass_16bit_PermutationInfo[] = {
    { g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_size, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_data, 1, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_CBVResourceNames, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_CBVResourceBindings, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_CBVResourceCounts, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_CBVResourceSpaces, 6, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_TextureSRVResourceNames, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_TextureSRVResourceBindings, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_TextureSRVResourceCounts, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_TextureSRVResourceSpaces, 1, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_TextureUAVResourceNames, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_TextureUAVResourceBindings, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_TextureUAVResourceCounts, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 2, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_BufferUAVResourceNames, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_BufferUAVResourceBindings, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_BufferUAVResourceCounts, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_BufferUAVResourceSpaces, 1, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_SamplerResourceNames, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_SamplerResourceBindings, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_SamplerResourceCounts, g_ffx_sssr_intersect_pass_16bit_114c138e7a442ea9706370043299f230_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_size, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_data, 1, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_CBVResourceNames, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_CBVResourceBindings, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_CBVResourceCounts, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_CBVResourceSpaces, 6, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_TextureSRVResourceNames, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_TextureSRVResourceBindings, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_TextureSRVResourceCounts, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_TextureSRVResourceSpaces, 1, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_TextureUAVResourceNames, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_TextureUAVResourceBindings, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_TextureUAVResourceCounts, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 2, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_BufferUAVResourceNames, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_BufferUAVResourceBindings, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_BufferUAVResourceCounts, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_BufferUAVResourceSpaces, 1, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_SamplerResourceNames, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_SamplerResourceBindings, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_SamplerResourceCounts, g_ffx_sssr_intersect_pass_16bit_f0886a44e8b6d1426f2d2c90dda90f47_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

