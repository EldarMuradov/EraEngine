#include "ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2.h"
#include "ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136.h"

typedef union ffx_sssr_depth_downsample_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_SSSR_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_sssr_depth_downsample_pass_wave64_PermutationKey;

typedef struct ffx_sssr_depth_downsample_pass_wave64_PermutationInfo {
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
} ffx_sssr_depth_downsample_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_sssr_depth_downsample_pass_wave64_IndirectionTable[] = {
    0,
    1,
};

static const ffx_sssr_depth_downsample_pass_wave64_PermutationInfo g_ffx_sssr_depth_downsample_pass_wave64_PermutationInfo[] = {
    { g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_size, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_data, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_TextureSRVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_TextureSRVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_TextureSRVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_TextureSRVResourceSpaces, 1, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_TextureUAVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_TextureUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_TextureUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_BufferUAVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_BufferUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_BufferUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_93f9f08a48659914cb0a197be56313f2_BufferUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_size, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_data, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_TextureSRVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_TextureSRVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_TextureSRVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_TextureSRVResourceSpaces, 1, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_TextureUAVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_TextureUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_TextureUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_BufferUAVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_BufferUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_BufferUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_4e99c118e21248e0c2ae035053f57136_BufferUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

