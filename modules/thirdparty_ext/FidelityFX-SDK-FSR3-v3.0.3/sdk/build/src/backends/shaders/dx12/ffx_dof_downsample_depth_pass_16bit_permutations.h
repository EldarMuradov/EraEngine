#include "ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225.h"
#include "ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808.h"

typedef union ffx_dof_downsample_depth_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_DOF_OPTION_MAX_RING_MERGE_LOG : 1;
        uint32_t FFX_DOF_OPTION_COMBINE_IN_PLACE : 1;
        uint32_t FFX_DOF_OPTION_REVERSE_DEPTH : 1;
    };
    uint32_t index;
} ffx_dof_downsample_depth_pass_16bit_PermutationKey;

typedef struct ffx_dof_downsample_depth_pass_16bit_PermutationInfo {
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
} ffx_dof_downsample_depth_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_dof_downsample_depth_pass_16bit_IndirectionTable[] = {
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
};

static const ffx_dof_downsample_depth_pass_16bit_PermutationInfo g_ffx_dof_downsample_depth_pass_16bit_PermutationInfo[] = {
    { g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_size, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_data, 1, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_CBVResourceNames, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_CBVResourceBindings, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_CBVResourceCounts, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_CBVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_TextureSRVResourceNames, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_TextureSRVResourceBindings, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_TextureSRVResourceCounts, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_TextureSRVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_TextureUAVResourceNames, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_TextureUAVResourceBindings, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_TextureUAVResourceCounts, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_BufferUAVResourceNames, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_BufferUAVResourceBindings, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_BufferUAVResourceCounts, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_BufferUAVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_SamplerResourceNames, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_SamplerResourceBindings, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_SamplerResourceCounts, g_ffx_dof_downsample_depth_pass_16bit_907b8fa2f6ef50035c017cc2493cc225_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_size, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_data, 1, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_CBVResourceNames, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_CBVResourceBindings, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_CBVResourceCounts, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_CBVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_TextureSRVResourceNames, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_TextureSRVResourceBindings, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_TextureSRVResourceCounts, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_TextureSRVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_TextureUAVResourceNames, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_TextureUAVResourceBindings, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_TextureUAVResourceCounts, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_BufferUAVResourceNames, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_BufferUAVResourceBindings, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_BufferUAVResourceCounts, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_BufferUAVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_SamplerResourceNames, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_SamplerResourceBindings, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_SamplerResourceCounts, g_ffx_dof_downsample_depth_pass_16bit_2f9bba04926322b93dc28748b12a4808_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

