#include "ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1.h"
#include "ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714.h"

typedef union ffx_classifier_reflections_pass_PermutationKey {
    struct {
        uint32_t FFX_CLASSIFIER_OPTION_INVERTED_DEPTH : 1;
        uint32_t FFX_CLASSIFIER_OPTION_CLASSIFIER_MODE : 1;
    };
    uint32_t index;
} ffx_classifier_reflections_pass_PermutationKey;

typedef struct ffx_classifier_reflections_pass_PermutationInfo {
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
} ffx_classifier_reflections_pass_PermutationInfo;

static const uint32_t g_ffx_classifier_reflections_pass_IndirectionTable[] = {
    0,
    1,
    0,
    1,
};

static const ffx_classifier_reflections_pass_PermutationInfo g_ffx_classifier_reflections_pass_PermutationInfo[] = {
    { g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_size, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_data, 1, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_CBVResourceNames, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_CBVResourceBindings, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_CBVResourceCounts, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_CBVResourceSpaces, 7, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_TextureSRVResourceNames, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_TextureSRVResourceBindings, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_TextureSRVResourceCounts, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_TextureSRVResourceSpaces, 3, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_TextureUAVResourceNames, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_TextureUAVResourceBindings, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_TextureUAVResourceCounts, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 4, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_BufferUAVResourceNames, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_BufferUAVResourceBindings, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_BufferUAVResourceCounts, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_BufferUAVResourceSpaces, 2, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_SamplerResourceNames, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_SamplerResourceBindings, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_SamplerResourceCounts, g_ffx_classifier_reflections_pass_37129985131f31ddf4cadd8ca8975da1_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_size, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_data, 1, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_CBVResourceNames, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_CBVResourceBindings, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_CBVResourceCounts, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_CBVResourceSpaces, 7, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_TextureSRVResourceNames, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_TextureSRVResourceBindings, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_TextureSRVResourceCounts, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_TextureSRVResourceSpaces, 3, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_TextureUAVResourceNames, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_TextureUAVResourceBindings, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_TextureUAVResourceCounts, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 4, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_BufferUAVResourceNames, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_BufferUAVResourceBindings, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_BufferUAVResourceCounts, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_BufferUAVResourceSpaces, 2, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_SamplerResourceNames, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_SamplerResourceBindings, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_SamplerResourceCounts, g_ffx_classifier_reflections_pass_5038070e6b85b36f64f60f964afd2714_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

