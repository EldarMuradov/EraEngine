#ifndef PARTICLES_HLSLI
#define PARTICLES_HLSLI

static float get_rel_life(float life, float max_life)
{
	float rel_life = saturate((max_life - life) / max_life);
	return rel_life;
}

#endif