#ifndef POST_PROCESSING_RS_HLSLI
#define POST_PROCESSING_RS_HLSLI
#define POST_PROCESSING_BLOCK_SIZE 16

// ----------------------------------------
// BLOOM
// ----------------------------------------
struct BloomThresholdCB
{
    vec2 inv_dimensions;
    float threshold;
};

#define BLOOM_THRESHOLD_RS \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=3, b0),"  \
    "StaticSampler(s0," \
	    "addressU = TEXTURE_ADDRESS_CLAMP," \
	    "addressV = TEXTURE_ADDRESS_CLAMP," \
	    "addressW = TEXTURE_ADDRESS_CLAMP," \
	    "filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1) )"

#define BLOOM_THRESHOLD_RS_CB           0
#define BLOOM_THRESHOLD_RS_TEXTURES     1

struct BloomCombineCB
{
    vec2 inv_dimensions;
    float strength;
};
#define BLOOM_COMBINE_RS \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=3, b0),"  \
    "StaticSampler(s0," \
	    "addressU = TEXTURE_ADDRESS_CLAMP," \
	    "addressV = TEXTURE_ADDRESS_CLAMP," \
	    "addressW = TEXTURE_ADDRESS_CLAMP," \
	    "filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 2) )"


#define BLOOM_COMBINE_RS_CB           0
#define BLOOM_COMBINE_RS_TEXTURES     1

// ----------------------------------------
// BLIT
// ----------------------------------------

struct BlitCB
{
    vec2 inv_dimensions;
};

#define BLIT_RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 2), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"


#define BLIT_RS_CB                    0
#define BLIT_RS_TEXTURES              1

// ----------------------------------------
// GAUSSIAN BLUR
// ----------------------------------------

struct GaussianBlurCB
{
    vec2 inv_dimensions;
    float step_scale;
    uint32 direction_and_source_mip_level; // Direction (0 is horizontal, 1 is vertical) | sourceMipLevel.
};

#define GAUSSIAN_BLUR_RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 4), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"
#define GAUSSIAN_BLUR_RS_CB           0
#define GAUSSIAN_BLUR_RS_TEXTURES     1

// ----------------------------------------
// MORPHOLOGY
// ----------------------------------------

struct MorphologyCB
{
    uint32 radius;
    uint32 direction; // 0 = x, 1 = y.
    uint32 dim_in_direction;
};

#define MORPHOLOGY_MAX_RADIUS 16
#define MORPHOLOGY_BLOCK_SIZE 128

#define MORPHOLOGY_RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 3), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1) )"

#define MORPHOLOGY_RS_CB              0
#define MORPHOLOGY_RS_TEXTURES        1

// ----------------------------------------
// HIERARCHICAL LINEAR DEPTH
// ----------------------------------------

struct HierarchicalLinearDepthCB
{
    vec4 projection_params;
    vec2 inv_dimensions;
};

#define HIERARCHICAL_LINEAR_DEPTH_RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 8), " \
    "DescriptorTable( UAV(u0, numDescriptors = 6), SRV(t0, numDescriptors = 1) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

#define HIERARCHICAL_LINEAR_DEPTH_RS_CB           0
#define HIERARCHICAL_LINEAR_DEPTH_RS_TEXTURES     1

// ----------------------------------------
// TEMPORAL ANTI-ALIASING
// ----------------------------------------

struct TAACB
{
    vec4 projection_params;
    vec2 dimensions;
};

#define TAA_RS \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=8, b0),"  \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 4) ), " \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

#define TAA_RS_CB               0
#define TAA_RS_TEXTURES         1

// ----------------------------------------
// HBAO
// ----------------------------------------

struct HBAOCB
{
    uint32 screen_width;
    uint32 screen_height;
    float depth_buffer_mip_level;
    uint32 num_rays;
    vec2 ray_delta_rotation;
    uint32 max_num_steps_per_ray;
    float radius;
    float strength;
    float seed;
};

#define HBAO_RS \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=10, b0),"  \
    "CBV(b1), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1) ), " \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

#define HBAO_RS_CB                  0
#define HBAO_RS_CAMERA              1
#define HBAO_RS_TEXTURES            2

// ----------------------------------------
// SCREEN-SPACE SHADOWS
// ----------------------------------------

struct SSSCB
{
    vec2 inv_dimensions;
    uint32 num_steps;
    float ray_distance;
    vec3 light_direction;
    float thickness;
    float max_distance_from_camera;
    float distance_fadeout_range;
    float inv_border_fadeout;
    float seed;
};

#define SSS_RS \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=12, b0),"  \
    "CBV(b1), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_POINT)"

#define SSS_RS_CB                   0
#define SSS_RS_CAMERA               1
#define SSS_RS_TEXTURES             2

// ----------------------------------------
// SHADOW BLUR
// ----------------------------------------

struct ShadowBlurCB
{
    vec2 dimensions;
    vec2 inv_dimensions;
};

#define SHADOW_BLUR_X_RS \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=4, b0),"  \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 2) ), " \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_POINT), " \
    "StaticSampler(s1," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

#define SHADOW_BLUR_Y_RS \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=4, b0),"  \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 4) ), " \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_POINT), " \
    "StaticSampler(s1," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

#define SHADOW_BLUR_RS_CB                  0
#define SHADOW_BLUR_RS_TEXTURES            1

// ----------------------------------------
// SPECULAR AMBIENT
// ----------------------------------------

struct SpecularAmbientCB
{
    vec2 inv_dimensions;
};

#define SPECULAR_AMBIENT_RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 2), " \
    "CBV(b1), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 7) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"


#define SPECULAR_AMBIENT_RS_CB           0
#define SPECULAR_AMBIENT_RS_CAMERA       1
#define SPECULAR_AMBIENT_RS_TEXTURES     2

// ----------------------------------------
// TONEMAPPING
// ----------------------------------------

struct TonemapCB
{
    float A;
    float B;
    float C;
    float D;
    float E;
    float F;
    float inv_evaluated_linear_white;
    float exp_exposure;

    vec3 evaluate(vec3 x)
    {
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - (E / F);
    }

    vec3 tonemap(vec3 color)
    {
        color *= exp_exposure;
        return evaluate(color) * inv_evaluated_linear_white;
    }
};

#define TONEMAP_RS \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=8, b0),"  \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1) )"

#define TONEMAP_RS_CB               0
#define TONEMAP_RS_TEXTURES         1

// ----------------------------------------
// PRESENT
// ----------------------------------------

#define present_sdr 0
#define present_hdr 1

struct PresentCB
{
    uint32 display_mode;
    float standard_nits;
    float sharpen_strength;
    uint32 offset; // x-offset | y-offset.
};

#define PRESENT_RS \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=4, b0),"  \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1) )"

#define PRESENT_RS_CB               0
#define PRESENT_RS_TEXTURES         1

// ----------------------------------------
// DEPTH SOBEL
// ----------------------------------------

struct DepthSobelCB
{
    vec4 projection_params;
    float threshold;
};

#define DEPTH_SOBEL_RS \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=8, b0),"  \
    "DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1) )"

#define DEPTH_SOBEL_RS_CB           0
#define DEPTH_SOBEL_RS_TEXTURES     1
#endif