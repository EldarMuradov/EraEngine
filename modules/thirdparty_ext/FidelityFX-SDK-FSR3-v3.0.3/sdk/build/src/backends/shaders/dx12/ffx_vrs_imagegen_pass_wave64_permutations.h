#include "ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e.h"
#include "ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1.h"
#include "ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0.h"
#include "ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229.h"
#include "ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4.h"
#include "ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb.h"

typedef union ffx_vrs_imagegen_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_VRS_OPTION_ADDITIONALSHADINGRATES : 1;
        uint32_t FFX_VARIABLESHADING_TILESIZE : 2;
    };
    uint32_t index;
} ffx_vrs_imagegen_pass_wave64_PermutationKey;

typedef struct ffx_vrs_imagegen_pass_wave64_PermutationInfo {
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
} ffx_vrs_imagegen_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_vrs_imagegen_pass_wave64_IndirectionTable[] = {
    1,
    5,
    2,
    4,
    0,
    3,
    0,
    0,
};

static const ffx_vrs_imagegen_pass_wave64_PermutationInfo g_ffx_vrs_imagegen_pass_wave64_PermutationInfo[] = {
    { g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_size, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_data, 1, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_CBVResourceNames, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_CBVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_CBVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_d78a2643053df9273cc68b340c6d542e_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_size, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_data, 1, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_CBVResourceNames, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_CBVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_CBVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_62ccc548233a9700663e2f13d9da7de1_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_size, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_data, 1, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_CBVResourceNames, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_CBVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_CBVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_80877cfcfc70111c5ca7a14ef0d46af0_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_size, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_data, 1, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_CBVResourceNames, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_CBVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_CBVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_8070c9686d04b3f89c7d5d5b81fcb229_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_size, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_data, 1, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_CBVResourceNames, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_CBVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_CBVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_d3123240ee48688a6a32b58a807d56d4_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_size, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_data, 1, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_CBVResourceNames, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_CBVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_CBVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_wave64_bb23aa5828ea61151498c3125ba348eb_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

