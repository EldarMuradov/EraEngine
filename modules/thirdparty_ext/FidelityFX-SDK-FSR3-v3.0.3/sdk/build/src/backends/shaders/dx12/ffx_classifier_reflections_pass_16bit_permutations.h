#include "ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17.h"
#include "ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3.h"

typedef union ffx_classifier_reflections_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_CLASSIFIER_OPTION_INVERTED_DEPTH : 1;
        uint32_t FFX_CLASSIFIER_OPTION_CLASSIFIER_MODE : 1;
    };
    uint32_t index;
} ffx_classifier_reflections_pass_16bit_PermutationKey;

typedef struct ffx_classifier_reflections_pass_16bit_PermutationInfo {
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
} ffx_classifier_reflections_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_classifier_reflections_pass_16bit_IndirectionTable[] = {
    1,
    0,
    1,
    0,
};

static const ffx_classifier_reflections_pass_16bit_PermutationInfo g_ffx_classifier_reflections_pass_16bit_PermutationInfo[] = {
    { g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_size, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_data, 1, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_CBVResourceNames, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_CBVResourceBindings, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_CBVResourceCounts, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_CBVResourceSpaces, 7, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_TextureSRVResourceNames, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_TextureSRVResourceBindings, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_TextureSRVResourceCounts, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_TextureSRVResourceSpaces, 3, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_TextureUAVResourceNames, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_TextureUAVResourceBindings, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_TextureUAVResourceCounts, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 4, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_BufferUAVResourceNames, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_BufferUAVResourceBindings, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_BufferUAVResourceCounts, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_BufferUAVResourceSpaces, 2, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_SamplerResourceNames, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_SamplerResourceBindings, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_SamplerResourceCounts, g_ffx_classifier_reflections_pass_16bit_e5404920e19bb78e9d96dfd1548aab17_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_size, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_data, 1, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_CBVResourceNames, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_CBVResourceBindings, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_CBVResourceCounts, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_CBVResourceSpaces, 7, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_TextureSRVResourceNames, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_TextureSRVResourceBindings, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_TextureSRVResourceCounts, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_TextureSRVResourceSpaces, 3, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_TextureUAVResourceNames, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_TextureUAVResourceBindings, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_TextureUAVResourceCounts, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 4, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_BufferUAVResourceNames, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_BufferUAVResourceBindings, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_BufferUAVResourceCounts, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_BufferUAVResourceSpaces, 2, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_SamplerResourceNames, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_SamplerResourceBindings, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_SamplerResourceCounts, g_ffx_classifier_reflections_pass_16bit_168f8f78783c17404179d3e0ed2d05c3_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

