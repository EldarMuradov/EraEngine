#include "ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc.h"
#include "ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1.h"

typedef union ffx_sssr_intersect_pass_wave64_16bit_PermutationKey {
    struct {
        uint32_t FFX_SSSR_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_sssr_intersect_pass_wave64_16bit_PermutationKey;

typedef struct ffx_sssr_intersect_pass_wave64_16bit_PermutationInfo {
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
} ffx_sssr_intersect_pass_wave64_16bit_PermutationInfo;

static const uint32_t g_ffx_sssr_intersect_pass_wave64_16bit_IndirectionTable[] = {
    0,
    1,
};

static const ffx_sssr_intersect_pass_wave64_16bit_PermutationInfo g_ffx_sssr_intersect_pass_wave64_16bit_PermutationInfo[] = {
    { g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_size, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_data, 1, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_CBVResourceNames, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_CBVResourceBindings, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_CBVResourceCounts, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_CBVResourceSpaces, 6, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_TextureSRVResourceNames, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_TextureSRVResourceBindings, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_TextureSRVResourceCounts, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_TextureSRVResourceSpaces, 1, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_TextureUAVResourceNames, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_TextureUAVResourceBindings, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_TextureUAVResourceCounts, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 2, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_BufferUAVResourceNames, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_BufferUAVResourceBindings, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_BufferUAVResourceCounts, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_BufferUAVResourceSpaces, 1, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_SamplerResourceNames, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_SamplerResourceBindings, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_SamplerResourceCounts, g_ffx_sssr_intersect_pass_wave64_16bit_07e940924073f882fa96ba2a9516e1cc_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_size, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_data, 1, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_CBVResourceNames, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_CBVResourceBindings, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_CBVResourceCounts, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_CBVResourceSpaces, 6, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_TextureSRVResourceNames, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_TextureSRVResourceBindings, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_TextureSRVResourceCounts, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_TextureSRVResourceSpaces, 1, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_TextureUAVResourceNames, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_TextureUAVResourceBindings, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_TextureUAVResourceCounts, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 2, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_BufferUAVResourceNames, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_BufferUAVResourceBindings, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_BufferUAVResourceCounts, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_BufferUAVResourceSpaces, 1, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_SamplerResourceNames, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_SamplerResourceBindings, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_SamplerResourceCounts, g_ffx_sssr_intersect_pass_wave64_16bit_b408cef9a7ac27083697fb49e981f0c1_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

