#ifndef RT_REFLECTIONS_RS_HLSLI
#define RT_REFLECTIONS_RS_HLSLI

#include "../common/light_probe.hlsli"

struct RTReflectionsCB
{
	uint32 sample_sky_from_texture;
	uint32 frame_index;
};

#define RT_REFLECTIONS_RS_RESOURCES	0
#define RT_REFLECTIONS_RS_CB		1
#define RT_REFLECTIONS_RS_CAMERA	2
#define RT_REFLECTIONS_RS_LIGHTING	3

#endif
