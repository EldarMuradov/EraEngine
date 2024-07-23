#include "ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa.h"
#include "ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625.h"

typedef union ffx_cacao_upscale_bilateral_5x5_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_CACAO_OPTION_APPLY_SMART : 1;
    };
    uint32_t index;
} ffx_cacao_upscale_bilateral_5x5_pass_16bit_PermutationKey;

typedef struct ffx_cacao_upscale_bilateral_5x5_pass_16bit_PermutationInfo {
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
} ffx_cacao_upscale_bilateral_5x5_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_IndirectionTable[] = {
    0,
    1,
};

static const ffx_cacao_upscale_bilateral_5x5_pass_16bit_PermutationInfo g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_PermutationInfo[] = {
    { g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_size, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_data, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_CBVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_CBVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_CBVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_CBVResourceSpaces, 3, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_TextureSRVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_TextureSRVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_TextureSRVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_TextureSRVResourceSpaces, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_TextureUAVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_TextureUAVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_TextureUAVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_SamplerResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_SamplerResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_SamplerResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_3b48e3002d1ed1c371ac2e65a40aaffa_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_size, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_data, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_CBVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_CBVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_CBVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_CBVResourceSpaces, 3, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_TextureSRVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_TextureSRVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_TextureSRVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_TextureSRVResourceSpaces, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_TextureUAVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_TextureUAVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_TextureUAVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_SamplerResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_SamplerResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_SamplerResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_16bit_2abcdb3cb03e297be35ad9d74fa75625_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

