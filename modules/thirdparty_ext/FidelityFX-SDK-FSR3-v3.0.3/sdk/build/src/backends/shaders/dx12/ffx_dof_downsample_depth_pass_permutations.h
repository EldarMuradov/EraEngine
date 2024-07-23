#include "ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09.h"
#include "ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0.h"

typedef union ffx_dof_downsample_depth_pass_PermutationKey {
    struct {
        uint32_t FFX_DOF_OPTION_MAX_RING_MERGE_LOG : 1;
        uint32_t FFX_DOF_OPTION_COMBINE_IN_PLACE : 1;
        uint32_t FFX_DOF_OPTION_REVERSE_DEPTH : 1;
    };
    uint32_t index;
} ffx_dof_downsample_depth_pass_PermutationKey;

typedef struct ffx_dof_downsample_depth_pass_PermutationInfo {
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
} ffx_dof_downsample_depth_pass_PermutationInfo;

static const uint32_t g_ffx_dof_downsample_depth_pass_IndirectionTable[] = {
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
};

static const ffx_dof_downsample_depth_pass_PermutationInfo g_ffx_dof_downsample_depth_pass_PermutationInfo[] = {
    { g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_size, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_data, 1, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_CBVResourceNames, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_CBVResourceBindings, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_CBVResourceCounts, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_CBVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_TextureSRVResourceNames, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_TextureSRVResourceBindings, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_TextureSRVResourceCounts, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_TextureSRVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_TextureUAVResourceNames, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_TextureUAVResourceBindings, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_TextureUAVResourceCounts, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_BufferUAVResourceNames, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_BufferUAVResourceBindings, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_BufferUAVResourceCounts, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_BufferUAVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_SamplerResourceNames, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_SamplerResourceBindings, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_SamplerResourceCounts, g_ffx_dof_downsample_depth_pass_98fac8b59ee56162540f384502bb4b09_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_size, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_data, 1, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_CBVResourceNames, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_CBVResourceBindings, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_CBVResourceCounts, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_CBVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_TextureSRVResourceNames, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_TextureSRVResourceBindings, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_TextureSRVResourceCounts, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_TextureSRVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_TextureUAVResourceNames, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_TextureUAVResourceBindings, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_TextureUAVResourceCounts, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_BufferUAVResourceNames, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_BufferUAVResourceBindings, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_BufferUAVResourceCounts, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_BufferUAVResourceSpaces, 1, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_SamplerResourceNames, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_SamplerResourceBindings, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_SamplerResourceCounts, g_ffx_dof_downsample_depth_pass_87dcfabec0343293017be0328531a9e0_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

