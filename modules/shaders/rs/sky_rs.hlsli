#ifndef SKY_RS_HLSLI
#define SKY_RS_HLSLI

struct SkyTransformCB
{
	mat4 vp;
	mat4 prev_frame_vp;
};

struct SkyCB
{
	vec2 jitter;
	vec2 prev_frame_jitter;
	float intensity;
	vec3 sun_direction;
};

#define SKY_PROCEDURAL_RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
	"DENY_HULL_SHADER_ROOT_ACCESS |" \
	"DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
	"DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
	"RootConstants(num32BitConstants=32, b0, visibility=SHADER_VISIBILITY_VERTEX)," \
	"RootConstants(num32BitConstants=8, b1, visibility=SHADER_VISIBILITY_PIXEL)"

#define SKY_TEXTURE_RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
	"DENY_HULL_SHADER_ROOT_ACCESS |" \
	"DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
	"DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
	"RootConstants(num32BitConstants=32, b0, visibility=SHADER_VISIBILITY_VERTEX)," \
	"RootConstants(num32BitConstants=8, b1, visibility=SHADER_VISIBILITY_PIXEL)," \
	"StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL)," \
	"DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_PIXEL)"

#define SKY_RS_TRANSFORM	0
#define SKY_RS_CB			1
#define SKY_RS_TEX			2

#endif