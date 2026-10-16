#include "cs.hlsli"
#include "camera.hlsli"
#include "light_culling_rs.hlsli"
#include "light_source.hlsli"
#include "material.hlsli"

/*
    This shader culls lights and decals against screen space tiles.
    The emitted indices are handled differently for lights and decals.

    - For lights we simply write a dense list of indices per tile.
    - For decals we output a bit mask, where a 1 means that this decal influences this surface point. This limits the total number of decals per frame, but this way we don't need
      a sorting step, to properly draw the decals.
*/

ConstantBuffer<CameraCB> camera	                : register(b0);
ConstantBuffer<LightCullingCB> cb	                : register(b1);

Texture2D<float> depth_buffer                        : register(t0);
StructuredBuffer<LightCullingViewFrustum> frusta : register(t1);

StructuredBuffer<PointLightCB> point_lights        : register(t2);
StructuredBuffer<SpotLightCB> spot_lights          : register(t3);
StructuredBuffer<PBRDecalCB> decals               : register(t4);

RWTexture2D<uint4> tiled_culling_grid                 : register(u0);
RWStructuredBuffer<uint> tiled_culling_index_counter   : register(u1);

RWStructuredBuffer<uint> tiled_objects_index_list      : register(u2);

groupshared uint group_min_depth;
groupshared uint group_max_depth;
groupshared LightCullingViewFrustum group_frustum;

groupshared float3 group_view_space_aabb_corners[8];
groupshared float3 group_view_space_aabb_center;
groupshared float3 group_view_space_aabb_extent;

groupshared uint group_tile_depth_mask;

groupshared uint group_objects_start_offset;

// Opaque
groupshared uint group_objects_list_opaque[MAX_NUM_INDICES_PER_TILE];
groupshared uint group_light_count_opaque;

// Transparent
groupshared uint group_objects_list_transparent[MAX_NUM_INDICES_PER_TILE];
groupshared uint group_light_count_transparent;

struct SpotLightBoundingVolume
{
    float3 tip;
    float height;
    float3 direction;
    float radius;
};

static SpotLightBoundingVolume get_spot_light_bounding_volume(SpotLightCB sl)
{
    SpotLightBoundingVolume result;
    result.tip = sl.position;
    result.direction = sl.direction;
    result.height = sl.max_distance;

    float oc = sl.get_outer_cutoff();
    result.radius = result.height * sqrt(1.f - oc * oc) / oc; // Same as height * tan(acos(oc)).
    return result;
}

static bool point_light_outside_plane(PointLightCB pl, LightCullingFrustumPlane plane)
{
    return dot(plane.N, pl.position) - plane.d < -pl.radius;
}

static bool point_light_inside_frustum(PointLightCB pl, LightCullingViewFrustum frustum, LightCullingFrustumPlane near_plane, LightCullingFrustumPlane far_plane)
{
    bool result = true;

    if (point_light_outside_plane(pl, near_plane)
        || point_light_outside_plane(pl, far_plane))
    {
        result = false;
    }

    for (int i = 0; i < 4 && result; ++i)
    {
        if (point_light_outside_plane(pl, frustum.planes[i]))
        {
            result = false;
        }
    }

    return result;
}

static bool sphere_in_aabb(vec3 sphere_center, float sphere_radius, float3 aabb_center, float3 aabb_extents)
{
    float3 delta = max(0, abs(aabb_center - sphere_center) - aabb_extents);
    float dist_sq = dot(delta, delta);

    return dist_sq <= sphere_radius * sphere_radius;
}

static bool point_outside_plane(float3 p, LightCullingFrustumPlane plane)
{
    return dot(plane.N, p) - plane.d < 0;
}

static bool spot_light_outside_plane(SpotLightBoundingVolume sl, LightCullingFrustumPlane plane)
{
    float3 m = normalize(cross(cross(plane.N, sl.direction), sl.direction));
    float3 Q = sl.tip + sl.direction * sl.height - m * sl.radius;
    return point_outside_plane(sl.tip, plane) && point_outside_plane(Q, plane);
}

static bool spot_light_inside_frustum(SpotLightBoundingVolume sl, LightCullingViewFrustum frustum, LightCullingFrustumPlane near_plane, LightCullingFrustumPlane far_plane)
{
    bool result = true;

    if (spot_light_outside_plane(sl, near_plane)
        || spot_light_outside_plane(sl, far_plane))
    {
        result = false;
    }

    for (int i = 0; i < 4 && result; ++i)
    {
        if (spot_light_outside_plane(sl, frustum.planes[i]))
        {
            result = false;
        }
    }

    return result;
}

static bool decal_outside_plane(PBRDecalCB decal, LightCullingFrustumPlane plane)
{
    float x = dot(decal.right, plane.N)     >= 0.f ? 1.f : -1.f;
    float y = dot(decal.up, plane.N)        >= 0.f ? 1.f : -1.f;
    float z = dot(decal.forward, plane.N)   >= 0.f ? 1.f : -1.f;

    float3 diag = x * decal.right + y * decal.up + z * decal.forward;

    float3 n_point = decal.position + diag;
    return point_outside_plane(n_point, plane);
}

static bool decal_inside_frustum(PBRDecalCB decal, LightCullingViewFrustum frustum, LightCullingFrustumPlane near_plane, LightCullingFrustumPlane far_plane)
{
    bool result = true;

    if (decal_outside_plane(decal, near_plane)
        || decal_outside_plane(decal, far_plane))
    {
        result = false;
    }

    for (int i = 0; i < 4 && result; ++i)
    {		
        if (decal_outside_plane(decal, frustum.planes[i]))
        {
            result = false;
        }
    }	
        
    return result;
}

static uint get_light_mask(float sphere_center_depth, float radius, float depth_range_recip, float near_z)
{
    const float pl_min = sphere_center_depth - radius;
    const float pl_max = sphere_center_depth + radius;
    const uint pl_mask_index_min = max(0, min(31, floor((pl_min - near_z) * depth_range_recip)));
    const uint pl_mask_index_max = max(0, min(31, floor((pl_max - near_z) * depth_range_recip)));

    // Set all bits between plMaskIndexMin and (inclusive) plMaskIndexMax
    uint light_mask = 0xFFFFFFFF;
    light_mask >>= 31 - (pl_mask_index_max - pl_mask_index_min);
    light_mask <<= pl_mask_index_min;
    return light_mask;
}

#define group_append_light(type, index)                               \
    {                                                               \
        uint v;                                                     \
        InterlockedAdd(group_light_count_##type, 1, v);                \
        if (v < MAX_NUM_LIGHTS_PER_TILE)                            \
        {                                                           \
            group_objects_list_##type[TILE_LIGHT_OFFSET + v] = index;  \
        }                                                           \
    }

// We flip the index, such that the first set bit corresponds to the front-most decal. We render the decals front to back, such that we can early exit when alpha >= 1.
#define group_append_decal(type, index)                               \
    {                                                               \
        const uint v = MAX_NUM_TOTAL_DECALS - index - 1;            \
        const uint bucket = v >> 5u;                                \
        const uint bit = v & ((1u << 5u) - 1);                      \
        InterlockedOr(group_objects_list_##type[bucket], 1u << bit);   \
    }

#define group_append_light_opaque(index) group_append_light(opaque, index)
#define group_append_light_transparent(index) group_append_light(transparent, index)
#define group_append_decal_opaque(index) group_append_decal(opaque, index)
#define group_append_decal_transparent(index) group_append_decal(transparent, index)

[RootSignature(LIGHT_CULLING_RS)]
[numthreads(LIGHT_CULLING_TILE_SIZE, LIGHT_CULLING_TILE_SIZE, 1)]
void main(CSInput IN)
{
    uint i;

    // Initialize
    if (IN.group_index == 0)
    {
        group_min_depth = asuint(0.9999999f);
        group_max_depth = 0;

        group_light_count_opaque = 0;
        group_light_count_transparent = 0;

        group_tile_depth_mask = 0;

        group_frustum = frusta[IN.group_id.y * cb.num_thread_groups_x + IN.group_id.x];
    }

    // Initialize decal masks to zero.
    for (i = IN.group_index; i < NUM_DECAL_BUCKETS; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE)
    {
        group_objects_list_opaque[i] = 0;
    }
    for (i = IN.group_index; i < NUM_DECAL_BUCKETS; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE)
    {
        group_objects_list_transparent[i] = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    // Determine minimum and maximum depth.
    uint2 screen_size;
    depth_buffer.GetDimensions(screen_size.x, screen_size.y);

    const float f_depth = depth_buffer[min(IN.dispatch_thread_id.xy, screen_size - 1)];
    // Since the depth is between 0 and 1, we can perform min and max operations on the bit pattern as uint. This is because native HLSL does not support floating point atomics.
    if (f_depth != 1.f)
    {
        const uint u_depth = asuint(f_depth);

        InterlockedMin(group_min_depth, u_depth);
        InterlockedMax(group_max_depth, u_depth);
    }
    GroupMemoryBarrierWithGroupSync();

    const float min_depth = asfloat(group_min_depth);
    const float max_depth = asfloat(group_max_depth);

    const float3 forward = camera.forward.xyz;
    const float3 camera_pos = camera.position.xyz;

    const float near_z = camera.depth_buffer_depth_to_eye_depth(min_depth); // Positive.
    const float far_z  = camera.depth_buffer_depth_to_eye_depth(max_depth); // Positive.
    const float my_z   = camera.depth_buffer_depth_to_eye_depth(f_depth);   // Positive.

    const float depth_range_recip = 31.f / (far_z - near_z);
    const uint depth_mask_index = max(0, min(31, floor((my_z - near_z) * depth_range_recip)));
    InterlockedOr(group_tile_depth_mask, 1u << depth_mask_index);

    const LightCullingFrustumPlane camera_near_plane = {  forward,  dot(forward, camera_pos + camera.projection_params.x * forward) };
    const LightCullingFrustumPlane near_plane       = {  forward,  dot(forward, camera_pos + near_z * forward) };
    const LightCullingFrustumPlane far_plane        = { -forward, -dot(forward, camera_pos + far_z  * forward) };

    if (IN.group_index < 8)
    {
        const uint x = IN.group_index & 0x1;
        const uint y = (IN.group_index >> 1) & 0x1;
        const uint z = (IN.group_index >> 2);

        const uint2 coord = (IN.group_id.xy + uint2(x, y)) * LIGHT_CULLING_TILE_SIZE;
        const float2 uv = (float2(coord) + 0.5f) / screen_size;

        group_view_space_aabb_corners[IN.group_index] = camera.restore_view_space_position(uv, lerp(min_depth, max_depth, z));
    }

    GroupMemoryBarrierWithGroupSync();

    if (IN.group_index == 0)
    {
        float3 min_aabb = 10000000;
        float3 max_aabb = -10000000;

        for (uint i = 0; i < 8; ++i)
        {
            min_aabb = min(min_aabb, group_view_space_aabb_corners[i]);
            max_aabb = max(max_aabb, group_view_space_aabb_corners[i]);
        }

        group_view_space_aabb_center = (max_aabb + min_aabb) * 0.5f;
        group_view_space_aabb_extent = max_aabb - group_view_space_aabb_center;
    }

    GroupMemoryBarrierWithGroupSync();

    // Decals
    const uint num_decals = cb.num_decals;
    for (i = IN.group_index; i < num_decals; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE)
    {
        PBRDecalCB d = decals[i];
        if (decal_inside_frustum(d, group_frustum, camera_near_plane, far_plane))
        {
            group_append_decal_transparent(i);

            if (!decal_outside_plane(d, near_plane))
            {
                group_append_decal_opaque(i);
            }
        }
    }

    // Point lights
    const uint num_point_lights = cb.num_point_lights;
    for (i = IN.group_index; i < num_point_lights; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE)
    {
        PointLightCB pl = point_lights[i];
        if (point_light_inside_frustum(pl, group_frustum, camera_near_plane, far_plane))
        {
            group_append_light_transparent(i);

            pl.position = mul(camera.view, float4(pl.position, 1.f)).xyz;

            if (sphere_in_aabb(pl.position, pl.radius, group_view_space_aabb_center, group_view_space_aabb_extent))
            {
                const uint light_mask = get_light_mask(-pl.position.z, pl.radius, depth_range_recip, near_z);

                if (light_mask & group_tile_depth_mask)
                {
                    group_append_light_opaque(i);
                }
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();
    const uint num_tile_point_lights_opaque = group_light_count_opaque;
    const uint num_tile_point_lights_transparent = group_light_count_transparent;

    // Spot lights
    const uint num_spot_lights = cb.num_spot_lights;
    for (i = IN.group_index; i < num_spot_lights; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE)
    {
        const SpotLightCB sl = spot_lights[i];
        const SpotLightBoundingVolume sl_bv = get_spot_light_bounding_volume(sl);
        if (spot_light_inside_frustum(sl_bv, group_frustum, camera_near_plane, far_plane))
        {
            group_append_light_transparent(i);

            const float oc = sl.get_outer_cutoff();
            const float radius = sl.max_distance * 0.5f / (oc * oc);
            const float3 center = mul(camera.view, float4(sl.position + sl.direction * radius, 1.f)).xyz;

            if (sphere_in_aabb(center, radius, group_view_space_aabb_center, group_view_space_aabb_extent))
            {
                const uint light_mask = get_light_mask(-center.z, radius, depth_range_recip, near_z);

                if (light_mask & group_tile_depth_mask)
                {
                    group_append_light_opaque(i);
                }
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();
    const uint num_tile_spot_lights_opaque = group_light_count_opaque - num_tile_point_lights_opaque;
    const uint num_tile_spot_lights_transparent = group_light_count_transparent - num_tile_point_lights_transparent;

    const uint total_index_count_opaque = group_light_count_opaque + NUM_DECAL_BUCKETS;
    const uint total_index_count_transparent = group_light_count_transparent + NUM_DECAL_BUCKETS;
    const uint total_index_count = total_index_count_opaque + total_index_count_transparent;
    if (IN.group_index == 0)
    {
        InterlockedAdd(tiled_culling_index_counter[0], total_index_count, group_objects_start_offset);

        tiled_culling_grid[IN.group_id.xy] = uint4(
            group_objects_start_offset,
            (num_tile_point_lights_opaque << 20) | (num_tile_spot_lights_opaque << 10),
            group_objects_start_offset + total_index_count_opaque, // Transparent objects are directly after opaques.
            (num_tile_point_lights_transparent << 20) | (num_tile_spot_lights_transparent << 10)
        );
    }

    GroupMemoryBarrierWithGroupSync();

    const uint offset_opaque = group_objects_start_offset + 0;
    for (i = IN.group_index; i < total_index_count_opaque; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE)
    {
        tiled_objects_index_list[offset_opaque + i] = group_objects_list_opaque[i];
    }

    const uint offset_transparent = group_objects_start_offset + total_index_count_opaque;
    for (i = IN.group_index; i < total_index_count_transparent; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE)
    {
        tiled_objects_index_list[offset_transparent + i] = group_objects_list_transparent[i];
    }
}
