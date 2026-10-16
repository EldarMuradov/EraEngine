#ifndef SKINNING_RS_HLSLI
#define SKINNING_RS_HLSLI

struct SkinningCB
{
    uint32 first_joint;
    uint32 num_joints;
    uint32 first_vertex;
    uint32 num_vertices;
    uint32 write_offset;
};

#define SKINNING_RS \
    "RootConstants(b0, num32BitConstants = 5), " \
    "SRV(t0), " \
    "SRV(t1), " \
    "SRV(t2), " \
    "UAV(u0), " \
    "UAV(u1)"

#define SKINNING_RS_CB			                0
#define SKINNING_RS_INPUT_VERTEX_BUFFER0        1
#define SKINNING_RS_INPUT_VERTEX_BUFFER1        2
#define SKINNING_RS_MATRICES                    3
#define SKINNING_RS_OUTPUT0                     4
#define SKINNING_RS_OUTPUT1                     5

struct ClothSkinningCB
{
    uint32 grid_size_x;
    uint32 grid_size_y;
    uint32 write_offset;
};

#define CLOTH_SKINNING_RS \
    "RootConstants(b0, num32BitConstants = 3), " \
    "SRV(t0), " \
    "UAV(u0), " \
    "UAV(u1)"
    
#define CLOTH_SKINNING_RS_CB                    0
#define CLOTH_SKINNING_RS_INPUT                 1
#define CLOTH_SKINNING_RS_OUTPUT0               2
#define CLOTH_SKINNING_RS_OUTPUT1               3

struct MeshPosition
{
    vec3 position;
};

struct MeshOthers
{
    vec2 uv;
    vec3 normal;
    vec3 tangent;
};

struct SkinnedMeshPosition
{
    vec3 position;
};

struct SkinnedMeshOthers
{
    vec2 uv;
    vec3 normal;
    vec3 tangent;
    uint32 skin_indices;
    uint32 skin_weights;
};
#endif