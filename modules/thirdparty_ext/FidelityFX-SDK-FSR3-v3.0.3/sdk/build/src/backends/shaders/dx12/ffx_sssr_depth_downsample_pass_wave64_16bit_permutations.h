#include "ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864.h"
#include "ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19.h"

typedef union ffx_sssr_depth_downsample_pass_wave64_16bit_PermutationKey {
    struct {
        uint32_t FFX_SSSR_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_sssr_depth_downsample_pass_wave64_16bit_PermutationKey;

typedef struct ffx_sssr_depth_downsample_pass_wave64_16bit_PermutationInfo {
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
} ffx_sssr_depth_downsample_pass_wave64_16bit_PermutationInfo;

static const uint32_t g_ffx_sssr_depth_downsample_pass_wave64_16bit_IndirectionTable[] = {
    0,
    1,
};

static const ffx_sssr_depth_downsample_pass_wave64_16bit_PermutationInfo g_ffx_sssr_depth_downsample_pass_wave64_16bit_PermutationInfo[] = {
    { g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_size, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_data, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_TextureSRVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_TextureSRVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_TextureSRVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_TextureSRVResourceSpaces, 1, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_TextureUAVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_TextureUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_TextureUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_BufferUAVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_BufferUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_BufferUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_16bit_8db705029ff324a09196b6915a920864_BufferUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_size, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_data, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_TextureSRVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_TextureSRVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_TextureSRVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_TextureSRVResourceSpaces, 1, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_TextureUAVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_TextureUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_TextureUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 1, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_BufferUAVResourceNames, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_BufferUAVResourceBindings, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_BufferUAVResourceCounts, g_ffx_sssr_depth_downsample_pass_wave64_16bit_caa575fc708760277c4f413208eadb19_BufferUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

