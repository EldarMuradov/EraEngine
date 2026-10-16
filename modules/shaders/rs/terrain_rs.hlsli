#ifndef TERRAIN_RS_HLSLI
#define TERRAIN_RS_HLSLI

// Must match heightmap_collider.h
#define TERRAIN_LOD_0_VERTICES_PER_DIMENSION 129u
#define TERRAIN_MAX_LOD 5u

struct TerrainCB
{
	vec3 min_corner;
	uint32 lod;
	float chunk_size;
	float amplitude_scale;
	uint32 scale_down_by_lods; // 4 * 8 bit.
};

struct TerrainWaterPlaneCB
{
	vec4 water_min_max_xz[4];
	vec4 water_heights;
	uint32 num_water_planes;
};

#define SCALE_DOWN_BY_LODS(neg_x, pos_x, neg_z, pos_z) ((neg_x << 24) | (pos_x << 16) | (neg_z << 8) | pos_z)
#define DECODE_LOD_SCALE(v, neg_x, pos_x, neg_z, pos_z) neg_x = v >> 24, pos_x = (v >> 16) & 0xFF, neg_z = (v >> 8) & 0xFF, pos_z = v & 0xFF;

struct TerrainTransformCB
{
	mat4 vp;
};

#define TERRAIN_RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
	"DENY_HULL_SHADER_ROOT_ACCESS |" \
	"DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
	"DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
	"RootConstants(num32BitConstants=16, b0, visibility=SHADER_VISIBILITY_VERTEX)," \
	"RootConstants(num32BitConstants=7, b1)," \
	"CBV(b2)," \
	"DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_VERTEX)," \
	"DescriptorTable(SRV(t1, numDescriptors=1), visibility=SHADER_VISIBILITY_PIXEL)," \
	"CBV(b1, space=1), " \
	"CBV(b2, space=1), " \
	"DescriptorTable(SRV(t0, space=1, numDescriptors=9), visibility=SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t0, space=2, numDescriptors=7), visibility=SHADER_VISIBILITY_PIXEL), " \
	"StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR," \
        "visibility=SHADER_VISIBILITY_PIXEL), " \
	"StaticSampler(s1," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_ANISOTROPIC," \
        "visibility=SHADER_VISIBILITY_PIXEL), " \
    "StaticSampler(s2," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT," \
        "visibility=SHADER_VISIBILITY_PIXEL)"


#define TERRAIN_RS_TRANSFORM		0
#define TERRAIN_RS_CB				1
#define TERRAIN_RS_WATER			2
#define TERRAIN_RS_HEIGHTMAP		3
#define TERRAIN_RS_NORMALMAP		4
#define TERRAIN_RS_CAMERA			5
#define TERRAIN_RS_LIGHTING			6
#define TERRAIN_RS_TEXTURES			7
#define TERRAIN_RS_FRAME_CONSTANTS  8

#define TERRAIN_DEPTH_ONLY_RS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
    "DENY_HULL_SHADER_ROOT_ACCESS |" \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
    "RootConstants(num32BitConstants=32, b0, visibility=SHADER_VISIBILITY_VERTEX), " \
    "RootConstants(num32BitConstants=7, b1, visibility=SHADER_VISIBILITY_VERTEX), " \
    "RootConstants(num32BitConstants=1, b2, visibility=SHADER_VISIBILITY_VERTEX), " \
    "CBV(b0, space=1), " \
	"DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_VERTEX)"

#define TERRAIN_DEPTH_ONLY_RS_TRANSFORM		0
#define TERRAIN_DEPTH_ONLY_RS_CB			1
#define TERRAIN_DEPTH_ONLY_RS_OBJECT_ID     2
#define TERRAIN_DEPTH_ONLY_RS_CAMERA		3
#define TERRAIN_DEPTH_ONLY_RS_HEIGHTMAP		4

#define TERRAIN_SHADOW_RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
    "DENY_HULL_SHADER_ROOT_ACCESS |" \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
    "DENY_PIXEL_SHADER_ROOT_ACCESS)," \
    "RootConstants(num32BitConstants=16, b0, visibility=SHADER_VISIBILITY_VERTEX), " \
	"RootConstants(num32BitConstants=7, b1, visibility=SHADER_VISIBILITY_VERTEX), " \
	"DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_VERTEX)"

#define TERRAIN_SHADOW_RS_TRANSFORM		0
#define TERRAIN_SHADOW_RS_CB			1
#define TERRAIN_SHADOW_RS_HEIGHTMAP		2

#define TERRAIN_OUTLINE_RS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
    "DENY_HULL_SHADER_ROOT_ACCESS |" \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
    "DENY_PIXEL_SHADER_ROOT_ACCESS)," \
    "RootConstants(num32BitConstants=16, b0, visibility=SHADER_VISIBILITY_VERTEX), " \
    "RootConstants(num32BitConstants=7, b1, visibility=SHADER_VISIBILITY_VERTEX), " \
	"DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_VERTEX)"

#define TERRAIN_OUTLINE_RS_TRANSFORM		0
#define TERRAIN_OUTLINE_RS_CB				1
#define TERRAIN_OUTLINE_RS_HEIGHTMAP		2

struct TerrainGenerationSettingsCB
{
	vec2 domain_warp_noise_offset;
	vec2 noise_offset;

	uint32 height_width;
	uint32 height_height;
	uint32 normal_width;
	uint32 normal_height;

	float position_scale;
	float normal_scale;

	float scale;

	float domain_warp_strength;

	uint32 domain_warp_octaves;

	uint32 noise_octaves;
};

struct TerrainGenerationCB
{
	vec2 min_corner;
};

#define TERRAIN_GENERATION_RS \
	"RootConstants(num32BitConstants=2, b0), " \
	"CBV(b1), " \
	"DescriptorTable(UAV(u0, numDescriptors=2))"

#define TERRAIN_GENERATION_RS_CB		0
#define TERRAIN_GENERATION_RS_SETTINGS	1
#define TERRAIN_GENERATION_RS_TEXTURES	2

#endif