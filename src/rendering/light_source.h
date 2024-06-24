// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"
#include "core/camera.h"
#include "core/reflect.h"
#include "light_source.hlsli"

struct directional_light
{
	void updateMatrices(const render_camera& camera);

	vec4 cascadeDistances;
	vec4 bias;
	vec4 blendDistances;
	vec4 shadowMapViewports[MAX_NUM_SUN_SHADOW_CASCADES];

	mat4 viewProjs[MAX_NUM_SUN_SHADOW_CASCADES];

	vec3 color;
	vec3 direction;

	float intensity;
	float negativeZOffset = 750.0f;

	uint32 shadowDimensions = 2048;
	uint32 numShadowCascades;

	bool stabilize;
};
REFLECT_STRUCT(directional_light,
	(color, "Color"),
	(intensity, "Intensity"),
	(direction, "Direction"),
	(numShadowCascades, "Cascades"),
	(cascadeDistances, "Cascade distances"),
	(bias, "Bias"),
	(blendDistances, "Blend distances"),
	(shadowDimensions, "Shadow dimensions"),
	(stabilize, "Stabilize")
);

struct point_light_component
{
	point_light_component() {}
	point_light_component(vec3 color, float intensity, float radius, bool castsShadow = false, uint32 shadowMapResolution = 512)
		: color(color), intensity(intensity), radius(radius), castsShadow(castsShadow), shadowMapResolution(shadowMapResolution) {}
	point_light_component(const point_light_component&) = default;

	vec3 color{};

	float intensity{};
	float radius{};

	uint32 shadowMapResolution{};

	bool castsShadow{};
};
REFLECT_STRUCT(point_light_component,
	(color, "Color"),
	(intensity, "Intensity"),
	(radius, "Radius"),
	(castsShadow, "Casts shadow"),
	(shadowMapResolution, "Shadow resolution")
);

struct spot_light_component
{
	spot_light_component() {}
	spot_light_component(vec3 color, float intensity, float distance, float innerAngle, float outerAngle, bool castsShadow = false, uint32 shadowMapResolution = 512)
		: color(color), intensity(intensity), distance(distance), innerAngle(innerAngle), outerAngle(outerAngle), castsShadow(castsShadow), shadowMapResolution(shadowMapResolution) {}
	spot_light_component(const spot_light_component&) = default;

	vec3 color;

	float intensity;
	float distance;
	float innerAngle;
	float outerAngle;

	uint32 shadowMapResolution;

	bool castsShadow;
};
REFLECT_STRUCT(spot_light_component,
	(color, "Color"),
	(intensity, "Intensity"),
	(distance, "Distance"),
	(innerAngle, "Inner angle"),
	(outerAngle, "Outer angle"),
	(castsShadow, "Casts shadow"),
	(shadowMapResolution, "Shadow resolution")
);

NODISCARD mat4 getSpotLightViewProjectionMatrix(const spot_light_cb& sl);