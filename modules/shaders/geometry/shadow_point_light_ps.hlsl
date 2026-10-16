struct PSInput
{
	float clip_depth		: CLIP_DEPTH;
};

void main(PSInput IN)
{
	clip(IN.clip_depth);
}
