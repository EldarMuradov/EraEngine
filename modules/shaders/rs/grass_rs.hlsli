#ifndef GRASS_RS_HLSLI
#define GRASS_RS_HLSLI

#include "indirect.hlsli"

struct GrassBlade
{
    vec3 position;
    uint32 misc; // Type, lod
    vec2 facing; // sin(angle), cos(angle)
    float height;
    uint32 wind_strength_prev_wind_strength;

#ifdef HLSL
    void initialize(float3 position_, float angle, uint type, float lod, float height_, float wind_strength, float prev_frame_wind_strength)
    {
        position = position_;
        sincos(angle, facing.x, facing.y);
        misc = (type << 16) | f32tof16(lod);
        height = height_;
        wind_strength_prev_wind_strength = (f32tof16(wind_strength) << 16) | f32tof16(prev_frame_wind_strength);
    }

    uint type()
    {
        return misc >> 16;
    }

    float lod()
    {
        return f16tof32(misc & 0xFFFF);
    }

    float wind_strength()
    {
        return f16tof32(wind_strength_prev_wind_strength >> 16);
    }

    float prev_frame_wind_strength()
    {
        return f16tof32(wind_strength_prev_wind_strength & 0xFFFF);
    }
#endif
};

struct GrassDraw
{
    D3D12_DRAW_ARGUMENTS draw;
};

struct GrassCB
{
    vec2 wind_direction;

    uint32 num_vertices;
    float half_width;
};

#ifdef NO_DEPTH_PREPASS
#define GRASS_OBJECT_ID ", RootConstants(num32BitConstants=1, b3, visibility=SHADER_VISIBILITY_PIXEL)"
#else
#define GRASS_OBJECT_ID ""
#endif

#define GRASS_RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
    "DENY_HULL_SHADER_ROOT_ACCESS |" \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
    "RootConstants(num32BitConstants=4, b0, visibility=SHADER_VISIBILITY_VERTEX), " \
    "SRV(t0, visibility=SHADER_VISIBILITY_VERTEX), " \
    "CBV(b1), " \
    "CBV(b2, visibility=SHADER_VISIBILITY_PIXEL), " \
    "DescriptorTable(SRV(t0, space=2, numDescriptors=7), visibility=SHADER_VISIBILITY_PIXEL), " \
	"StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR," \
        "visibility=SHADER_VISIBILITY_PIXEL), " \
    "StaticSampler(s1," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT," \
        "visibility=SHADER_VISIBILITY_PIXEL)" \
    GRASS_OBJECT_ID

#define GRASS_RS_CB                 0
#define GRASS_RS_BLADES             1
#define GRASS_RS_CAMERA             2
#define GRASS_RS_LIGHTING           3
#define GRASS_RS_FRAME_CONSTANTS    4
#define GRASS_RS_OBJECT_ID          5

#define GRASS_DEPTH_ONLY_RS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
    "DENY_HULL_SHADER_ROOT_ACCESS |" \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
    "RootConstants(num32BitConstants=7, b0, visibility=SHADER_VISIBILITY_VERTEX), " \
    "RootConstants(num32BitConstants=1, b1, visibility=SHADER_VISIBILITY_VERTEX), " \
    "CBV(b0, space=1), " \
    "SRV(t0, visibility=SHADER_VISIBILITY_VERTEX)"

#define GRASS_DEPTH_ONLY_RS_CB              0
#define GRASS_DEPTH_ONLY_RS_OBJECT_ID       1
#define GRASS_DEPTH_ONLY_RS_CAMERA          2
#define GRASS_DEPTH_ONLY_RS_BLADES          3

#define GRASS_GENERATION_BLOCK_SIZE 8

struct GrassGenerationCommonCB
{
    vec4 frustum_planes[6];
    vec3 camera_position;
    float chunk_size;
    float amplitude_scale;
    float uv_scale;

    float lod_change_start_distance;
    float lod_change_end_distance;

    float cull_start_distance;
    float cull_end_distance;

    float time;
    float prev_frame_time;
    float base_height;
};

struct GrassGenerationCB
{
    vec3 chunk_corner;
    uint32 lod_index;
};

#define GRASS_GENERATION_RS \
	"RootConstants(num32BitConstants=4, b0), " \
	"CBV(b1), " \
	"DescriptorTable(SRV(t0, numDescriptors=2), UAV(u0, numDescriptors=3)), " \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

#define GRASS_GENERATION_RS_CB        0
#define GRASS_GENERATION_RS_COMMON    1
#define GRASS_GENERATION_RS_RESOURCES 2

#define GRASS_CREATE_DRAW_CALLS_BLOCK_SIZE 32

struct GrassCreateDrawCallsCB
{
    uint32 max_num_instances;
};

#define GRASS_CREATE_DRAW_CALLS_RS \
    "RootConstants(num32BitConstants=1, b0), " \
	"UAV(u0), " \
	"SRV(t0)"

#define GRASS_CREATE_DRAW_CALLS_RS_CB			    0
#define GRASS_CREATE_DRAW_CALLS_RS_OUTPUT			1
#define GRASS_CREATE_DRAW_CALLS_RS_MESH_COUNTS		2
#endif