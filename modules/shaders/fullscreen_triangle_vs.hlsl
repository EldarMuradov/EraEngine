struct VSInput
{
	uint vertex_id	: SV_VertexID;
};

struct VSOutput
{
	float2 uv		: TEXCOORDS;
	float4 position : SV_Position;
};

VSOutput main(VSInput IN)
{
	VSOutput OUT;

	float x = float((IN.vertex_id & 1) << 2) - 1.f;
	float y = 1.f - float((IN.vertex_id & 2) << 1);
	float u = x * 0.5f + 0.5f;
	float v = y * 0.5f + 0.5f;
	OUT.position = float4(x, -y, 0.f, 1.f);
	OUT.uv = float2(u, v);

	return OUT;
}