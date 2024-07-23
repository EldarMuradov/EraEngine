#include "ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8.h"
#include "ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177.h"
#include "ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e.h"
#include "ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5.h"
#include "ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186.h"
#include "ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb.h"

typedef union ffx_vrs_imagegen_pass_PermutationKey {
    struct {
        uint32_t FFX_VRS_OPTION_ADDITIONALSHADINGRATES : 1;
        uint32_t FFX_VARIABLESHADING_TILESIZE : 2;
    };
    uint32_t index;
} ffx_vrs_imagegen_pass_PermutationKey;

typedef struct ffx_vrs_imagegen_pass_PermutationInfo {
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
} ffx_vrs_imagegen_pass_PermutationInfo;

static const uint32_t g_ffx_vrs_imagegen_pass_IndirectionTable[] = {
    0,
    3,
    2,
    5,
    1,
    4,
    0,
    0,
};

static const ffx_vrs_imagegen_pass_PermutationInfo g_ffx_vrs_imagegen_pass_PermutationInfo[] = {
    { g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_size, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_data, 1, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_CBVResourceNames, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_CBVResourceBindings, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_CBVResourceCounts, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_0145aff18895b362173a26df753b6ee8_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_size, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_data, 1, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_CBVResourceNames, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_CBVResourceBindings, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_CBVResourceCounts, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_670b9c7931ac9a3056cb4d0682563177_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_size, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_data, 1, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_CBVResourceNames, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_CBVResourceBindings, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_CBVResourceCounts, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_b1271e6f37510ac3005a6a65e52db74e_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_size, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_data, 1, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_CBVResourceNames, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_CBVResourceBindings, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_CBVResourceCounts, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_fa543349a07befa1062c0eea29bd9ae5_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_size, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_data, 1, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_CBVResourceNames, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_CBVResourceBindings, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_CBVResourceCounts, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_5cc31d79cfab258b573c0305f6798186_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_size, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_data, 1, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_CBVResourceNames, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_CBVResourceBindings, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_CBVResourceCounts, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_CBVResourceSpaces, 2, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_TextureSRVResourceNames, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_TextureSRVResourceBindings, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_TextureSRVResourceCounts, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_TextureSRVResourceSpaces, 1, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_TextureUAVResourceNames, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_TextureUAVResourceBindings, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_TextureUAVResourceCounts, g_ffx_vrs_imagegen_pass_3569efa6b2558ba9cae113b76473e2eb_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

