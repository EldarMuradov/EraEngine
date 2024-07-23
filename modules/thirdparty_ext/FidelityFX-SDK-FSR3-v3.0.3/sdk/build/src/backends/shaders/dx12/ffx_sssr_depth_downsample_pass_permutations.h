#include "ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28.h"
#include "ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31.h"

typedef union ffx_sssr_depth_downsample_pass_PermutationKey {
    struct {
        uint32_t FFX_SSSR_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_sssr_depth_downsample_pass_PermutationKey;

typedef struct ffx_sssr_depth_downsample_pass_PermutationInfo {
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
} ffx_sssr_depth_downsample_pass_PermutationInfo;

static const uint32_t g_ffx_sssr_depth_downsample_pass_IndirectionTable[] = {
    0,
    1,
};

static const ffx_sssr_depth_downsample_pass_PermutationInfo g_ffx_sssr_depth_downsample_pass_PermutationInfo[] = {
    { g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_size, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_data, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_TextureSRVResourceNames, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_TextureSRVResourceBindings, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_TextureSRVResourceCounts, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_TextureSRVResourceSpaces, 1, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_TextureUAVResourceNames, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_TextureUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_TextureUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_BufferUAVResourceNames, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_BufferUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_BufferUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_1d098148150fb90eb9f46afdc5efce28_BufferUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_size, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_data, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_TextureSRVResourceNames, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_TextureSRVResourceBindings, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_TextureSRVResourceCounts, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_TextureSRVResourceSpaces, 1, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_TextureUAVResourceNames, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_TextureUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_TextureUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_BufferUAVResourceNames, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_BufferUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_BufferUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_37cffbb85cd96aea414a0cb474a35e31_BufferUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

