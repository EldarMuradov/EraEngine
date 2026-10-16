#include "transform.hlsli"

#define MESH_RS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
	"DENY_VERTEX_SHADER_ROOT_ACCESS |" \
    "DENY_HULL_SHADER_ROOT_ACCESS |" \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
    "DENY_PIXEL_SHADER_ROOT_ACCESS)," \
    "RootConstants(num32BitConstants=32, b0), " \
    "SRV(t0), " \
    "SRV(t1), " \
    "SRV(t2), " \
    "SRV(t3)"

struct MeshOutput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
};

struct MeshVertex
{
    float3 position;
    float3 normal;
};

struct MeshletInfo
{
    uint num_vertices;
    uint first_vertex;
    uint num_primitives;
    uint first_primitive;
};

ConstantBuffer<TransformCB> transform      : register(b0);

StructuredBuffer<MeshVertex> vertices      : register(t0);
StructuredBuffer<MeshletInfo> meshlets     : register(t1);
StructuredBuffer<uint> unique_vertex_indices  : register(t2);
StructuredBuffer<uint> primitive_indices     : register(t3);

[outputtopology("triangle")]
[numthreads(128, 1, 1)]
[RootSignature(MESH_RS)]
void main(
    in uint group_thread_id : SV_GroupThreadID, 
    in uint group_id : SV_GroupID, 
    out vertices MeshOutput out_verts[64], 
    out indices uint3 out_indices[126]
)
{
    MeshletInfo m = meshlets[group_id];

    SetMeshOutputCounts(m.num_vertices, m.num_primitives);

    if (group_thread_id < m.num_primitives)
    {
        uint packed_primitive = primitive_indices[m.first_primitive + group_thread_id];

        // Unpacks a 10 bits per index triangle from a 32-bit uint.
        out_indices[group_thread_id] = uint3(packed_primitive & 0x3FF, (packed_primitive >> 10) & 0x3FF, (packed_primitive >> 20) & 0x3FF);
    }

    if (group_thread_id < m.num_vertices)
    {
        uint vertex_index = unique_vertex_indices[m.first_vertex + group_thread_id];
        uint meshlet_index = group_id;

        // Color based on meshlet index for visualization.
        out_verts[group_thread_id].color = float3(
            float(meshlet_index & 1),
            float(meshlet_index & 3) / 4,
            float(meshlet_index & 7) / 8);

        out_verts[group_thread_id].pos = mul(transform.mvp, float4(vertices[vertex_index].position, 1.f));
    }
}
