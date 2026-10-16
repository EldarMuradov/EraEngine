#ifndef DATA_T
#define DATA_T float4
#endif

#define NUM_WEIGHTS 2

static const float kernel_offsets[2] = { 0.f, 1.33333333333333f };
static const float blur_weights[2] = { 0.29411764705882354f, 0.35294117647058826f };

#include "gaussian_blur_common.hlsli"