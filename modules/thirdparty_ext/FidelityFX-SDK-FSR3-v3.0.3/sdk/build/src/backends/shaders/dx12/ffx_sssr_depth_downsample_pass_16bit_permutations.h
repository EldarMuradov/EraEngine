#include "ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9.h"
#include "ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a.h"

typedef union ffx_sssr_depth_downsample_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_SSSR_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_sssr_depth_downsample_pass_16bit_PermutationKey;

typedef struct ffx_sssr_depth_downsample_pass_16bit_PermutationInfo {
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
} ffx_sssr_depth_downsample_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_sssr_depth_downsample_pass_16bit_IndirectionTable[] = {
    0,
    1,
};

static const ffx_sssr_depth_downsample_pass_16bit_PermutationInfo g_ffx_sssr_depth_downsample_pass_16bit_PermutationInfo[] = {
    { g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_size, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_data, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_TextureSRVResourceNames, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_TextureSRVResourceBindings, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_TextureSRVResourceCounts, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_TextureSRVResourceSpaces, 1, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_TextureUAVResourceNames, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_TextureUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_TextureUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_BufferUAVResourceNames, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_BufferUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_BufferUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_16bit_8000bae44a889fcab522630375b326a9_BufferUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_size, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_data, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_TextureSRVResourceNames, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_TextureSRVResourceBindings, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_TextureSRVResourceCounts, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_TextureSRVResourceSpaces, 1, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_TextureUAVResourceNames, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_TextureUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_TextureUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_BufferUAVResourceNames, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_BufferUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_BufferUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_16bit_126922058dab2f9900f01b9b878a5a4a_BufferUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

