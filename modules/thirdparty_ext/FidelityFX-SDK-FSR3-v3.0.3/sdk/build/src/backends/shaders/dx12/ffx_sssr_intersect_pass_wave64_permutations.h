#include "ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b.h"
#include "ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191.h"

typedef union ffx_sssr_intersect_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_SSSR_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_sssr_intersect_pass_wave64_PermutationKey;

typedef struct ffx_sssr_intersect_pass_wave64_PermutationInfo {
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
} ffx_sssr_intersect_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_sssr_intersect_pass_wave64_IndirectionTable[] = {
    0,
    1,
};

static const ffx_sssr_intersect_pass_wave64_PermutationInfo g_ffx_sssr_intersect_pass_wave64_PermutationInfo[] = {
    { g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_size, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_data, 1, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_CBVResourceNames, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_CBVResourceBindings, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_CBVResourceCounts, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_CBVResourceSpaces, 6, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_TextureSRVResourceNames, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_TextureSRVResourceBindings, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_TextureSRVResourceCounts, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_TextureSRVResourceSpaces, 1, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_TextureUAVResourceNames, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_TextureUAVResourceBindings, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_TextureUAVResourceCounts, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 2, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_BufferUAVResourceNames, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_BufferUAVResourceBindings, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_BufferUAVResourceCounts, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_BufferUAVResourceSpaces, 1, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_SamplerResourceNames, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_SamplerResourceBindings, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_SamplerResourceCounts, g_ffx_sssr_intersect_pass_wave64_b99034f9243f34cc9d46c4e24ae4345b_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_size, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_data, 1, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_CBVResourceNames, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_CBVResourceBindings, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_CBVResourceCounts, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_CBVResourceSpaces, 6, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_TextureSRVResourceNames, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_TextureSRVResourceBindings, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_TextureSRVResourceCounts, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_TextureSRVResourceSpaces, 1, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_TextureUAVResourceNames, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_TextureUAVResourceBindings, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_TextureUAVResourceCounts, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 2, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_BufferUAVResourceNames, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_BufferUAVResourceBindings, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_BufferUAVResourceCounts, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_BufferUAVResourceSpaces, 1, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_SamplerResourceNames, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_SamplerResourceBindings, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_SamplerResourceCounts, g_ffx_sssr_intersect_pass_wave64_353d003507e41b1bb4c42948b4cfa191_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

