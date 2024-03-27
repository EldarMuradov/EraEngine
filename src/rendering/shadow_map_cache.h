// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"

struct shadow_map_viewport
{
	uint16 x, y;
	uint16 size;
};

extern bool enableStaticShadowMapCaching;

// Call after light view-projection-matrices are computed.
NODISCARD uint64 getLightMovementHash(const struct directional_light& dl); // Whole light.
NODISCARD uint64 getLightMovementHash(const struct spot_light_cb& sl);
NODISCARD uint64 getLightMovementHash(const struct point_light_cb& pl);

// Returns true, if static cache is available.
NODISCARD std::pair<shadow_map_viewport, bool> assignShadowMapViewport(uint64 uniqueLightID, uint64 lightMovementHash, uint32 size);
void updateShadowMapAllocationVisualization();