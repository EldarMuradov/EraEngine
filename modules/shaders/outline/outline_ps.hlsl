#include "outline_rs.hlsli"

struct PSInput
{
	float2 uv				: TEXCOORDS;
	float4 screen_position	: SV_POSITION;
};

ConstantBuffer<OutlineDrawerCB> outline : register(b0);

Texture2D<uint2> stencil	: register(t0);

static const int selected = (1 << 0); // TODO: This must be the same as in renderer.

static uint sample_at(int2 tex_coords)
{
	uint result = 1;
	if (tex_coords.x >= 0 && tex_coords.y >= 0 && tex_coords.x < outline.width && tex_coords.y < outline.height)
	{
		result = (stencil[tex_coords].y & selected) != 0;
	}
	return result;
}

[RootSignature(OUTLINE_DRAWER_RS)]
float4 main(PSInput IN) : SV_TARGET
{
	int2 tex_coords = int2(IN.screen_position.xy);

	uint s = 0;

	[unroll]
	for (int x = -2; x <= 2; ++x)
	{
		for (int y = -2; y <= 2; ++y)
		{
			s += sample_at(tex_coords + int2(x, y));
		}
	}

	if (s == 25)
	{
		discard;
	}

	return float4(1.f, 0, 0.f, 1.f);
}
