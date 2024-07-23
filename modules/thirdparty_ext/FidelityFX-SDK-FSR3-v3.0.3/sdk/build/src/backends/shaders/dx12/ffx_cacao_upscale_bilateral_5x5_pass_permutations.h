#include "ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28.h"
#include "ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1.h"

typedef union ffx_cacao_upscale_bilateral_5x5_pass_PermutationKey {
    struct {
        uint32_t FFX_CACAO_OPTION_APPLY_SMART : 1;
    };
    uint32_t index;
} ffx_cacao_upscale_bilateral_5x5_pass_PermutationKey;

typedef struct ffx_cacao_upscale_bilateral_5x5_pass_PermutationInfo {
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
} ffx_cacao_upscale_bilateral_5x5_pass_PermutationInfo;

static const uint32_t g_ffx_cacao_upscale_bilateral_5x5_pass_IndirectionTable[] = {
    1,
    0,
};

static const ffx_cacao_upscale_bilateral_5x5_pass_PermutationInfo g_ffx_cacao_upscale_bilateral_5x5_pass_PermutationInfo[] = {
    { g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_size, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_data, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_CBVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_CBVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_CBVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_CBVResourceSpaces, 3, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_TextureSRVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_TextureSRVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_TextureSRVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_TextureSRVResourceSpaces, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_TextureUAVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_TextureUAVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_TextureUAVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_SamplerResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_SamplerResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_SamplerResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_ff193273e9f6d45463466d49f812af28_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_size, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_data, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_CBVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_CBVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_CBVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_CBVResourceSpaces, 3, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_TextureSRVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_TextureSRVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_TextureSRVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_TextureSRVResourceSpaces, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_TextureUAVResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_TextureUAVResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_TextureUAVResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_SamplerResourceNames, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_SamplerResourceBindings, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_SamplerResourceCounts, g_ffx_cacao_upscale_bilateral_5x5_pass_0f0078731f8d2cdbbd9d388232b829c1_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

