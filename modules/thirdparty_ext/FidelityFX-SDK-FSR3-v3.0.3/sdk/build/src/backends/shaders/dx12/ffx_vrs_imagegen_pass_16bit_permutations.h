#include "ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673.h"
#include "ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189.h"
#include "ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712.h"
#include "ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7.h"
#include "ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b.h"
#include "ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84.h"

typedef union ffx_vrs_imagegen_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_VRS_OPTION_ADDITIONALSHADINGRATES : 1;
        uint32_t FFX_VARIABLESHADING_TILESIZE : 2;
    };
    uint32_t index;
} ffx_vrs_imagegen_pass_16bit_PermutationKey;

typedef struct ffx_vrs_imagegen_pass_16bit_PermutationInfo {
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
} ffx_vrs_imagegen_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_vrs_imagegen_pass_16bit_IndirectionTable[] = {
    1,
    4,
    0,
    3,
    2,
    5,
    0,
    0,
};

static const ffx_vrs_imagegen_pass_16bit_PermutationInfo g_ffx_vrs_imagegen_pass_16bit_PermutationInfo[] = {
    { g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_size, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_data, 1, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_CBVResourceNames, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_CBVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_CBVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_fc9bb11684628e4efb746aace443d673_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_size, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_data, 1, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_CBVResourceNames, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_CBVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_CBVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_8affe00359f1a1d4c606c899db5f4189_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_size, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_data, 1, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_CBVResourceNames, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_CBVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_CBVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_e7dac9800195287048baac34c1316712_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_size, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_data, 1, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_CBVResourceNames, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_CBVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_CBVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_9c1f03927636f1dbd28774dc22df89e7_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_size, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_data, 1, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_CBVResourceNames, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_CBVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_CBVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_bc4387c8cb786b28b1d51e6d27ee492b_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_size, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_data, 1, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_CBVResourceNames, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_CBVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_CBVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_16bit_5b1adb1c4d91fd44b9cf53c567f52a84_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

