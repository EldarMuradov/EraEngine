// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"

struct SpotLightCB;
struct PointLightCB;

namespace era_engine
{
    class DirectionalLight;

    struct ShadowMapViewport
    {
        uint16 x, y;
        uint16 size;
    };

    extern bool enable_static_shadow_map_caching;

    // Call after light view-projection-matrices are computed.
    NODISCARD uint64 get_light_movement_hash(const DirectionalLight& dl); // Whole light.
    NODISCARD uint64 get_light_movement_hash(const SpotLightCB& sl);
    NODISCARD uint64 get_light_movement_hash(const PointLightCB& pl);

    // Returns true, if static cache is available.
    NODISCARD std::pair<ShadowMapViewport, bool> assign_shadow_map_viewport(uint64 unique_light_id, uint64 light_movement_hash, uint32 size);
    void update_shadow_map_allocation_visualization();
}