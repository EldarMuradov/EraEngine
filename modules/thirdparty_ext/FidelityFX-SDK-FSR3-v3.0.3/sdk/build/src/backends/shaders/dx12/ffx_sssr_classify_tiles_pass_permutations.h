#include "ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2.h"
#include "ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e.h"

typedef union ffx_sssr_classify_tiles_pass_PermutationKey {
    struct {
        uint32_t FFX_SSSR_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_sssr_classify_tiles_pass_PermutationKey;

typedef struct ffx_sssr_classify_tiles_pass_PermutationInfo {
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
} ffx_sssr_classify_tiles_pass_PermutationInfo;

static const uint32_t g_ffx_sssr_classify_tiles_pass_IndirectionTable[] = {
    1,
    0,
};

static const ffx_sssr_classify_tiles_pass_PermutationInfo g_ffx_sssr_classify_tiles_pass_PermutationInfo[] = {
    { g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_size, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_data, 1, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_CBVResourceNames, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_CBVResourceBindings, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_CBVResourceCounts, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_CBVResourceSpaces, 5, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_TextureSRVResourceNames, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_TextureSRVResourceBindings, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_TextureSRVResourceCounts, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_TextureSRVResourceSpaces, 2, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_TextureUAVResourceNames, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_TextureUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_TextureUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 3, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_BufferUAVResourceNames, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_BufferUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_BufferUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_BufferUAVResourceSpaces, 1, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_SamplerResourceNames, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_SamplerResourceBindings, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_SamplerResourceCounts, g_ffx_sssr_classify_tiles_pass_c6aebc164e99d8ee4718b6a89770ddc2_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_size, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_data, 1, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_CBVResourceNames, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_CBVResourceBindings, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_CBVResourceCounts, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_CBVResourceSpaces, 5, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_TextureSRVResourceNames, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_TextureSRVResourceBindings, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_TextureSRVResourceCounts, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_TextureSRVResourceSpaces, 2, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_TextureUAVResourceNames, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_TextureUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_TextureUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 3, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_BufferUAVResourceNames, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_BufferUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_BufferUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_BufferUAVResourceSpaces, 1, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_SamplerResourceNames, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_SamplerResourceBindings, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_SamplerResourceCounts, g_ffx_sssr_classify_tiles_pass_82f6849783afcb1aade0a4a7b1c8407e_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

