#ifndef LIGHT_PROBE_RS_H
#define LIGHT_PROBE_RS_H

#include "../common/light_probe.hlsli"

#define LIGHT_PROBE_BLOCK_SIZE LIGHT_PROBE_TOTAL_RESOLUTION

struct LightProbeGridVisualizationCB
{
	mat4 mvp;
	vec2 uv_scale;
	float cell_size;
	uint32 count_x;
	uint32 count_y;
};

#define LIGHT_PROBE_GRID_VISUALIZATION_RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
	"DENY_HULL_SHADER_ROOT_ACCESS |" \
	"DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
	"DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
	"RootConstants(num32BitConstants=32, b0, visibility=SHADER_VISIBILITY_VERTEX), " \
	"DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_PIXEL), " \
	"StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR," \
        "visibility=SHADER_VISIBILITY_PIXEL)"

#define LIGHT_PROBE_GRID_VISUALIZATION_RS_CB			0
#define LIGHT_PROBE_GRID_VISUALIZATION_RS_IRRADIANCE	1

struct LightProbeRayVisualizationCB
{
	mat4 mvp;
	float cell_size;
	uint32 count_x;
	uint32 count_y;
};

#define LIGHT_PROBE_RAY_VISUALIZATION_RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
	"DENY_HULL_SHADER_ROOT_ACCESS |" \
	"DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
	"DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
	"RootConstants(num32BitConstants=32, b0, visibility=SHADER_VISIBILITY_VERTEX), " \
	"DescriptorTable(SRV(t0, numDescriptors=2), visibility=SHADER_VISIBILITY_VERTEX)"

#define LIGHT_PROBE_RAY_VISUALIZATION_RS_CB			0
#define LIGHT_PROBE_RAY_VISUALIZATION_RS_RAYS		1

#define LIGHT_PROBE_TEST_SAMPLE_RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
	"DENY_HULL_SHADER_ROOT_ACCESS |" \
	"DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
	"DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
	"RootConstants(num32BitConstants=32, b0, visibility=SHADER_VISIBILITY_VERTEX), " \
	"RootConstants(num32BitConstants=8, b1, visibility=SHADER_VISIBILITY_PIXEL), " \
	"DescriptorTable(SRV(t0, numDescriptors=2), visibility=SHADER_VISIBILITY_PIXEL), " \
	"StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR," \
        "visibility=SHADER_VISIBILITY_PIXEL)"

#define LIGHT_PROBE_TEST_SAMPLE_RS_TRANSFORM	0
#define LIGHT_PROBE_TEST_SAMPLE_RS_GRID			1
#define LIGHT_PROBE_TEST_SAMPLE_RS_TEXTURES		2

struct LightProbeUpdateCB
{
	uint32 count_x;
	uint32 count_y;
};

#define LIGHT_PROBE_UPDATE_RS \
	"RootFlags(0)," \
	"RootConstants(num32BitConstants=2, b0), " \
	"DescriptorTable(SRV(t0, numDescriptors=2), UAV(u0, numDescriptors=1))"

#define LIGHT_PROBE_UPDATE_RS_CB				0
#define LIGHT_PROBE_UPDATE_RS_RAYTRACE_RESULTS	1

struct LightProbeTraceCB
{
	mat4 ray_rotation;
	LightProbeGridCB grid;
	uint32 sample_sky_from_texture;
};

#endif