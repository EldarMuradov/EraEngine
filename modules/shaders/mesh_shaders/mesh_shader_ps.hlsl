struct PSInput
{
	float3 color : COLOR0;
};

float4 main(PSInput IN) : SV_TARGET
{
	return float4(IN.color, 1.f);
}
