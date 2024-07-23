#include "ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14.h"
#include "ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c.h"

typedef union ffx_dof_blur_pass_PermutationKey {
    struct {
        uint32_t FFX_DOF_OPTION_MAX_RING_MERGE_LOG : 1;
        uint32_t FFX_DOF_OPTION_COMBINE_IN_PLACE : 1;
        uint32_t FFX_DOF_OPTION_REVERSE_DEPTH : 1;
    };
    uint32_t index;
} ffx_dof_blur_pass_PermutationKey;

typedef struct ffx_dof_blur_pass_PermutationInfo {
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
} ffx_dof_blur_pass_PermutationInfo;

static const uint32_t g_ffx_dof_blur_pass_IndirectionTable[] = {
    1,
    0,
    1,
    0,
    1,
    0,
    1,
    0,
};

static const ffx_dof_blur_pass_PermutationInfo g_ffx_dof_blur_pass_PermutationInfo[] = {
    { g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_size, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_data, 1, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_CBVResourceNames, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_CBVResourceBindings, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_CBVResourceCounts, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_CBVResourceSpaces, 2, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_TextureSRVResourceNames, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_TextureSRVResourceBindings, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_TextureSRVResourceCounts, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_TextureSRVResourceSpaces, 2, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_TextureUAVResourceNames, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_TextureUAVResourceBindings, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_TextureUAVResourceCounts, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_BufferUAVResourceNames, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_BufferUAVResourceBindings, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_BufferUAVResourceCounts, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_BufferUAVResourceSpaces, 2, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_SamplerResourceNames, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_SamplerResourceBindings, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_SamplerResourceCounts, g_ffx_dof_blur_pass_14c74669f337af801c7a77f195664d14_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_size, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_data, 1, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_CBVResourceNames, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_CBVResourceBindings, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_CBVResourceCounts, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_CBVResourceSpaces, 2, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_TextureSRVResourceNames, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_TextureSRVResourceBindings, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_TextureSRVResourceCounts, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_TextureSRVResourceSpaces, 2, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_TextureUAVResourceNames, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_TextureUAVResourceBindings, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_TextureUAVResourceCounts, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_BufferUAVResourceNames, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_BufferUAVResourceBindings, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_BufferUAVResourceCounts, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_BufferUAVResourceSpaces, 2, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_SamplerResourceNames, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_SamplerResourceBindings, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_SamplerResourceCounts, g_ffx_dof_blur_pass_bbfb446065cea149ebd269f61947623c_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

