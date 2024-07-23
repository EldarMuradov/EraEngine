#include "ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c.h"
#include "ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a.h"

typedef union ffx_sssr_classify_tiles_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_SSSR_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_sssr_classify_tiles_pass_16bit_PermutationKey;

typedef struct ffx_sssr_classify_tiles_pass_16bit_PermutationInfo {
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
} ffx_sssr_classify_tiles_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_sssr_classify_tiles_pass_16bit_IndirectionTable[] = {
    0,
    1,
};

static const ffx_sssr_classify_tiles_pass_16bit_PermutationInfo g_ffx_sssr_classify_tiles_pass_16bit_PermutationInfo[] = {
    { g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_size, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_data, 1, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_CBVResourceNames, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_CBVResourceBindings, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_CBVResourceCounts, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_CBVResourceSpaces, 5, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_TextureSRVResourceNames, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_TextureSRVResourceBindings, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_TextureSRVResourceCounts, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_TextureSRVResourceSpaces, 2, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_TextureUAVResourceNames, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_TextureUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_TextureUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 3, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_BufferUAVResourceNames, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_BufferUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_BufferUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_BufferUAVResourceSpaces, 1, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_SamplerResourceNames, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_SamplerResourceBindings, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_SamplerResourceCounts, g_ffx_sssr_classify_tiles_pass_16bit_79fa561224fd9ce485337e44181fc69c_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_size, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_data, 1, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_CBVResourceNames, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_CBVResourceBindings, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_CBVResourceCounts, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_CBVResourceSpaces, 5, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_TextureSRVResourceNames, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_TextureSRVResourceBindings, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_TextureSRVResourceCounts, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_TextureSRVResourceSpaces, 2, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_TextureUAVResourceNames, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_TextureUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_TextureUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 3, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_BufferUAVResourceNames, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_BufferUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_BufferUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_BufferUAVResourceSpaces, 1, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_SamplerResourceNames, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_SamplerResourceBindings, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_SamplerResourceCounts, g_ffx_sssr_classify_tiles_pass_16bit_6b43c8a5f5378c065fdbee3874483b6a_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

