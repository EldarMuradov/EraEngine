#include "ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930.h"
#include "ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4.h"

typedef union ffx_dof_downsample_depth_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_DOF_OPTION_MAX_RING_MERGE_LOG : 1;
        uint32_t FFX_DOF_OPTION_COMBINE_IN_PLACE : 1;
        uint32_t FFX_DOF_OPTION_REVERSE_DEPTH : 1;
    };
    uint32_t index;
} ffx_dof_downsample_depth_pass_wave64_PermutationKey;

typedef struct ffx_dof_downsample_depth_pass_wave64_PermutationInfo {
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
} ffx_dof_downsample_depth_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_dof_downsample_depth_pass_wave64_IndirectionTable[] = {
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
};

static const ffx_dof_downsample_depth_pass_wave64_PermutationInfo g_ffx_dof_downsample_depth_pass_wave64_PermutationInfo[] = {
    { g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_size, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_data, 1, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_CBVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_CBVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_CBVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_CBVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_TextureSRVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_TextureSRVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_TextureSRVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_TextureSRVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_TextureUAVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_TextureUAVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_TextureUAVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_BufferUAVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_BufferUAVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_BufferUAVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_BufferUAVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_SamplerResourceNames, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_SamplerResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_SamplerResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_266dbd38fafe601248abe6be129bf930_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_size, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_data, 1, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_CBVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_CBVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_CBVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_CBVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_TextureSRVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_TextureSRVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_TextureSRVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_TextureSRVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_TextureUAVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_TextureUAVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_TextureUAVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_BufferUAVResourceNames, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_BufferUAVResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_BufferUAVResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_BufferUAVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_SamplerResourceNames, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_SamplerResourceBindings, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_SamplerResourceCounts, g_ffx_dof_downsample_depth_pass_wave64_60614e4341e8d0895ff6cdf191602ae4_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

