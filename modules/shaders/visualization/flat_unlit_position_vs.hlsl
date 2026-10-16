struct TransformCB
{
	float4x4 mvp;
};

ConstantBuffer<TransformCB> transform : register(b0);

struct VSInput
{
	float3 position		: POSITION;
};

struct VSOutput
{
	float3 color		: COLOR;
	float4 position		: SV_POSITION;
};

VSOutput main(VSInput IN)
{
	VSOutput OUT;
	OUT.position = mul(transform.mvp, float4(IN.position, 1.f));
	OUT.color = float3(1.f, 1.f, 1.f);
	return OUT;
}
