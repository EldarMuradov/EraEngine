#include "ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9.h"
#include "ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b.h"

typedef union ffx_sssr_intersect_pass_PermutationKey {
    struct {
        uint32_t FFX_SSSR_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_sssr_intersect_pass_PermutationKey;

typedef struct ffx_sssr_intersect_pass_PermutationInfo {
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
} ffx_sssr_intersect_pass_PermutationInfo;

static const uint32_t g_ffx_sssr_intersect_pass_IndirectionTable[] = {
    1,
    0,
};

static const ffx_sssr_intersect_pass_PermutationInfo g_ffx_sssr_intersect_pass_PermutationInfo[] = {
    { g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_size, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_data, 1, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_CBVResourceNames, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_CBVResourceBindings, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_CBVResourceCounts, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_CBVResourceSpaces, 6, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_TextureSRVResourceNames, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_TextureSRVResourceBindings, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_TextureSRVResourceCounts, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_TextureSRVResourceSpaces, 1, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_TextureUAVResourceNames, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_TextureUAVResourceBindings, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_TextureUAVResourceCounts, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 2, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_BufferUAVResourceNames, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_BufferUAVResourceBindings, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_BufferUAVResourceCounts, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_BufferUAVResourceSpaces, 1, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_SamplerResourceNames, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_SamplerResourceBindings, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_SamplerResourceCounts, g_ffx_sssr_intersect_pass_d2390d8fabc7886b368c52cb04d94fa9_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_size, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_data, 1, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_CBVResourceNames, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_CBVResourceBindings, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_CBVResourceCounts, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_CBVResourceSpaces, 6, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_TextureSRVResourceNames, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_TextureSRVResourceBindings, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_TextureSRVResourceCounts, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_TextureSRVResourceSpaces, 1, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_TextureUAVResourceNames, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_TextureUAVResourceBindings, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_TextureUAVResourceCounts, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 2, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_BufferUAVResourceNames, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_BufferUAVResourceBindings, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_BufferUAVResourceCounts, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_BufferUAVResourceSpaces, 1, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_SamplerResourceNames, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_SamplerResourceBindings, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_SamplerResourceCounts, g_ffx_sssr_intersect_pass_5cb5873fa0f42551ab8d28853699aa7b_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

