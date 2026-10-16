#include "particles_rs.hlsli"

[RootSignature(PARTICLES_RENDERING_RS)]
float4 main(VSOutput IN) : SV_TARGET
{
	return pixel_shader(IN);
}
