#include "ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a.h"

typedef union ffx_denoiser_resolve_temporal_reflections_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_DENOISER_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_denoiser_resolve_temporal_reflections_pass_16bit_PermutationKey;

typedef struct ffx_denoiser_resolve_temporal_reflections_pass_16bit_PermutationInfo {
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
} ffx_denoiser_resolve_temporal_reflections_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_IndirectionTable[] = {
    0,
    0,
};

static const ffx_denoiser_resolve_temporal_reflections_pass_16bit_PermutationInfo g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_PermutationInfo[] = {
    { g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_size, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_data, 1, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_CBVResourceNames, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_CBVResourceBindings, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_CBVResourceCounts, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_CBVResourceSpaces, 6, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_TextureSRVResourceNames, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_TextureSRVResourceBindings, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_TextureSRVResourceCounts, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_TextureSRVResourceSpaces, 2, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_TextureUAVResourceNames, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_TextureUAVResourceBindings, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_TextureUAVResourceCounts, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_BufferUAVResourceNames, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_BufferUAVResourceBindings, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_BufferUAVResourceCounts, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_BufferUAVResourceSpaces, 1, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_SamplerResourceNames, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_SamplerResourceBindings, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_SamplerResourceCounts, g_ffx_denoiser_resolve_temporal_reflections_pass_16bit_c9becb63dd0e760988503a14cc47d97a_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

