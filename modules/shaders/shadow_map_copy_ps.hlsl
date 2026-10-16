#define SHADOW_MAP_COPY_RS \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"DENY_VERTEX_SHADER_ROOT_ACCESS |" \
"DENY_HULL_SHADER_ROOT_ACCESS |" \
"DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
"DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
"RootConstants(num32BitConstants=4, b0, visibility=SHADER_VISIBILITY_PIXEL),"  \
"StaticSampler(s0," \
	"addressU = TEXTURE_ADDRESS_CLAMP," \
	"addressV = TEXTURE_ADDRESS_CLAMP," \
	"addressW = TEXTURE_ADDRESS_CLAMP," \
	"filter = FILTER_MIN_MAG_MIP_POINT,"	\
	"visibility=SHADER_VISIBILITY_PIXEL)," \
"DescriptorTable(SRV(t0, numDescriptors=1, flags = DESCRIPTORS_VOLATILE), visibility=SHADER_VISIBILITY_PIXEL)"

struct PSInput
{
	float2 uv	: TEXCOORDS;
};

cbuffer ShadowMapCopyCB : register(b0)
{
	float2 uv_offset;
	float2 uv_scale;
};

SamplerState tex_sampler				: register(s0);
Texture2D<float> shadow_map			: register(t0);

[RootSignature(SHADOW_MAP_COPY_RS)]
float main(PSInput IN) : SV_Depth
{
	return shadow_map.Sample(tex_sampler, IN.uv * uv_scale + uv_offset);
}