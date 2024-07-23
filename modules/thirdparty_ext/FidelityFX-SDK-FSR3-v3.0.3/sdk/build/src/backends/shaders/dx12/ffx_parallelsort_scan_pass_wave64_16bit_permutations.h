#include "ffx_parallelsort_scan_pass_wave64_16bit_b0d5318ba31abe4566c49e3038b7061c.h"

typedef union ffx_parallelsort_scan_pass_wave64_16bit_PermutationKey {
    struct {
        uint32_t FFX_PARALLELSORT_OPTION_HAS_PAYLOAD : 1;
    };
    uint32_t index;
} ffx_parallelsort_scan_pass_wave64_16bit_PermutationKey;

typedef struct ffx_parallelsort_scan_pass_wave64_16bit_PermutationInfo {
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
} ffx_parallelsort_scan_pass_wave64_16bit_PermutationInfo;

static const uint32_t g_ffx_parallelsort_scan_pass_wave64_16bit_IndirectionTable[] = {
    0,
    0,
};

static const ffx_parallelsort_scan_pass_wave64_16bit_PermutationInfo g_ffx_parallelsort_scan_pass_wave64_16bit_PermutationInfo[] = {
    { g_ffx_parallelsort_scan_pass_wave64_16bit_b0d5318ba31abe4566c49e3038b7061c_size, g_ffx_parallelsort_scan_pass_wave64_16bit_b0d5318ba31abe4566c49e3038b7061c_data, 1, g_ffx_parallelsort_scan_pass_wave64_16bit_b0d5318ba31abe4566c49e3038b7061c_CBVResourceNames, g_ffx_parallelsort_scan_pass_wave64_16bit_b0d5318ba31abe4566c49e3038b7061c_CBVResourceBindings, g_ffx_parallelsort_scan_pass_wave64_16bit_b0d5318ba31abe4566c49e3038b7061c_CBVResourceCounts, g_ffx_parallelsort_scan_pass_wave64_16bit_b0d5318ba31abe4566c49e3038b7061c_CBVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, g_ffx_parallelsort_scan_pass_wave64_16bit_b0d5318ba31abe4566c49e3038b7061c_BufferUAVResourceNames, g_ffx_parallelsort_scan_pass_wave64_16bit_b0d5318ba31abe4566c49e3038b7061c_BufferUAVResourceBindings, g_ffx_parallelsort_scan_pass_wave64_16bit_b0d5318ba31abe4566c49e3038b7061c_BufferUAVResourceCounts, g_ffx_parallelsort_scan_pass_wave64_16bit_b0d5318ba31abe4566c49e3038b7061c_BufferUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

