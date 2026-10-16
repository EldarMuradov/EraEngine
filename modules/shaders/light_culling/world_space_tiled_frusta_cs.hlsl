#include "cs.hlsli"
#include "camera.hlsli"
#include "light_culling_rs.hlsli"

#define BLOCK_SIZE 16

ConstantBuffer<CameraCB> camera	: register(b0);
ConstantBuffer<FrustaCB> cb	    : register(b1);
RWStructuredBuffer<LightCullingViewFrustum> out_frusta  : register(u0);

static LightCullingFrustumPlane get_plane(float3 p0, float3 p1, float3 p2)
{
    // The plane normal points to the inside of the frustum.

    LightCullingFrustumPlane plane;

    float3 v0 = p1 - p0;
    float3 v2 = p2 - p0;

    plane.N = normalize(cross(v0, v2));
    plane.d = dot(plane.N, p0);

    return plane;
}

[RootSignature(WORLD_SPACE_TILED_FRUSTA_RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CSInput IN)
{
    float2 inv_screen_dims = camera.inv_screen_dims;

    float2 screen_tl = inv_screen_dims * (IN.dispatch_thread_id.xy * LIGHT_CULLING_TILE_SIZE);
    float2 screen_tr = inv_screen_dims * (float2(IN.dispatch_thread_id.x + 1, IN.dispatch_thread_id.y) * LIGHT_CULLING_TILE_SIZE);
    float2 screen_bl = inv_screen_dims * (float2(IN.dispatch_thread_id.x, IN.dispatch_thread_id.y + 1) * LIGHT_CULLING_TILE_SIZE);
    float2 screen_br = inv_screen_dims * (float2(IN.dispatch_thread_id.x + 1, IN.dispatch_thread_id.y + 1) * LIGHT_CULLING_TILE_SIZE);

    // Points on near plane.
    float3 tl = camera.restore_world_space_position(screen_tl, 0.f);
    float3 tr = camera.restore_world_space_position(screen_tr, 0.f);
    float3 bl = camera.restore_world_space_position(screen_bl, 0.f);
    float3 br = camera.restore_world_space_position(screen_br, 0.f);

    float3 camera_pos = camera.position.xyz;
    LightCullingViewFrustum frustum;
    frustum.planes[0] = get_plane(camera_pos, bl, tl);
    frustum.planes[1] = get_plane(camera_pos, tr, br);
    frustum.planes[2] = get_plane(camera_pos, tl, tr);
    frustum.planes[3] = get_plane(camera_pos, br, bl);

    if (IN.dispatch_thread_id.x < cb.num_threads_x && IN.dispatch_thread_id.y < cb.num_threads_y)
    {
        uint index = IN.dispatch_thread_id.y * cb.num_threads_x + IN.dispatch_thread_id.x;
        out_frusta[index] = frustum;
    }
}