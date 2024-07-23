#include "ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a.h"
#include "ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab.h"

typedef union ffx_dof_blur_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_DOF_OPTION_MAX_RING_MERGE_LOG : 1;
        uint32_t FFX_DOF_OPTION_COMBINE_IN_PLACE : 1;
        uint32_t FFX_DOF_OPTION_REVERSE_DEPTH : 1;
    };
    uint32_t index;
} ffx_dof_blur_pass_wave64_PermutationKey;

typedef struct ffx_dof_blur_pass_wave64_PermutationInfo {
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
} ffx_dof_blur_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_dof_blur_pass_wave64_IndirectionTable[] = {
    0,
    1,
    0,
    1,
    0,
    1,
    0,
    1,
};

static const ffx_dof_blur_pass_wave64_PermutationInfo g_ffx_dof_blur_pass_wave64_PermutationInfo[] = {
    { g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_size, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_data, 1, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_CBVResourceNames, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_CBVResourceBindings, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_CBVResourceCounts, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_CBVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_TextureSRVResourceNames, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_TextureSRVResourceBindings, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_TextureSRVResourceCounts, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_TextureSRVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_TextureUAVResourceNames, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_TextureUAVResourceBindings, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_TextureUAVResourceCounts, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_BufferUAVResourceNames, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_BufferUAVResourceBindings, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_BufferUAVResourceCounts, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_BufferUAVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_SamplerResourceNames, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_SamplerResourceBindings, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_SamplerResourceCounts, g_ffx_dof_blur_pass_wave64_df2e5e593239890d992b1f3bd96bf13a_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_size, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_data, 1, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_CBVResourceNames, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_CBVResourceBindings, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_CBVResourceCounts, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_CBVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_TextureSRVResourceNames, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_TextureSRVResourceBindings, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_TextureSRVResourceCounts, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_TextureSRVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_TextureUAVResourceNames, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_TextureUAVResourceBindings, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_TextureUAVResourceCounts, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_BufferUAVResourceNames, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_BufferUAVResourceBindings, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_BufferUAVResourceCounts, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_BufferUAVResourceSpaces, 2, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_SamplerResourceNames, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_SamplerResourceBindings, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_SamplerResourceCounts, g_ffx_dof_blur_pass_wave64_d2de776c3fe01eb70ecacf714c495bab_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

