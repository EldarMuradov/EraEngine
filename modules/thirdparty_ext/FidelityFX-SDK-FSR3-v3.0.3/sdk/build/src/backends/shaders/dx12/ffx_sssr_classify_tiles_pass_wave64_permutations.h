#include "ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a.h"
#include "ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9.h"

typedef union ffx_sssr_classify_tiles_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_SSSR_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_sssr_classify_tiles_pass_wave64_PermutationKey;

typedef struct ffx_sssr_classify_tiles_pass_wave64_PermutationInfo {
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
} ffx_sssr_classify_tiles_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_sssr_classify_tiles_pass_wave64_IndirectionTable[] = {
    1,
    0,
};

static const ffx_sssr_classify_tiles_pass_wave64_PermutationInfo g_ffx_sssr_classify_tiles_pass_wave64_PermutationInfo[] = {
    { g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_size, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_data, 1, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_CBVResourceNames, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_CBVResourceBindings, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_CBVResourceCounts, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_CBVResourceSpaces, 5, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_TextureSRVResourceNames, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_TextureSRVResourceBindings, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_TextureSRVResourceCounts, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_TextureSRVResourceSpaces, 2, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_TextureUAVResourceNames, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_TextureUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_TextureUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 3, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_BufferUAVResourceNames, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_BufferUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_BufferUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_BufferUAVResourceSpaces, 1, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_SamplerResourceNames, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_SamplerResourceBindings, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_SamplerResourceCounts, g_ffx_sssr_classify_tiles_pass_wave64_d838c79889a2ef2fb9926aa32b54ad4a_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_size, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_data, 1, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_CBVResourceNames, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_CBVResourceBindings, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_CBVResourceCounts, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_CBVResourceSpaces, 5, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_TextureSRVResourceNames, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_TextureSRVResourceBindings, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_TextureSRVResourceCounts, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_TextureSRVResourceSpaces, 2, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_TextureUAVResourceNames, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_TextureUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_TextureUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 3, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_BufferUAVResourceNames, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_BufferUAVResourceBindings, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_BufferUAVResourceCounts, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_BufferUAVResourceSpaces, 1, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_SamplerResourceNames, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_SamplerResourceBindings, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_SamplerResourceCounts, g_ffx_sssr_classify_tiles_pass_wave64_53d3633edf89a457918dc263ff0ba7d9_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

