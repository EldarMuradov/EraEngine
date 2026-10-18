#include "geometry/mesh_builder.h"

#include "core/color.h"

#include "asset/model_asset.h"

#define pushVertex(pos, uv, nor, tan, skin, col)           \
    if (vertex_flags & MESH_CREATION_FLAGS_WITH_POSITIONS) \
    {                                                      \
        *position_ptr++ = pos;                             \
    }                                                      \
    if (vertex_flags & MESH_CREATION_FLAGS_WITH_UVS)       \
    {                                                      \
        *(vec2*)others_ptr = uv;                           \
        others_ptr += sizeof(vec2);                        \
    }                                                      \
    if (vertex_flags & MESH_CREATION_FLAGS_WITH_NORMALS)   \
    {                                                      \
        *(vec3*)others_ptr = nor;                          \
        others_ptr += sizeof(vec3);                        \
    }                                                      \
    if (vertex_flags & MESH_CREATION_FLAGS_WITH_TANGENTS)  \
    {                                                      \
        *(vec3*)others_ptr = tan;                          \
        others_ptr += sizeof(vec3);                        \
    }                                                      \
    if (vertex_flags & MESH_CREATION_FLAGS_WITH_SKIN)      \
    {                                                      \
        *(animation::SkinningWeights*)others_ptr = skin;   \
        others_ptr += sizeof(animation::SkinningWeights);  \
    }                                                      \
    if (vertex_flags & MESH_CREATION_FLAGS_WITH_COLORS)    \
    {                                                      \
        *(uint32*)others_ptr = col;                        \
        others_ptr += sizeof(uint32);                      \
    }

#define triangle(triangle_type, index_type, a, b, c)                                                                                                        \
    *(triangle_type*)index_ptr = flip_winding_order                                                                                                         \
                                     ? triangle_type{(index_type)((a) + index_offset), (index_type)((c) + index_offset), (index_type)((b) + index_offset)}  \
                                     : triangle_type{(index_type)((a) + index_offset), (index_type)((b) + index_offset), (index_type)((c) + index_offset)}; \
    index_ptr += sizeof(triangle_type);

#define pushTriangle(a, b, c)                               \
    if (index_type == MESH_INDEX_UINT16)                    \
    {                                                       \
        triangle(IndexedTriangle16, uint16, (a), (b), (c)); \
    }                                                       \
    else                                                    \
    {                                                       \
        triangle(IndexedTriangle32, uint32, (a), (b), (c)); \
    }

namespace era_engine
{
    using namespace animation;

    struct VertexInfo
    {
        uint32 others_size;
        uint32 skin_offset;
    };

    static VertexInfo get_vertex_info(uint32 flags)
    {
        VertexInfo result = {};
        if (flags & MESH_CREATION_FLAGS_WITH_UVS)
        {
            result.others_size += sizeof(vec2);
        }
        if (flags & MESH_CREATION_FLAGS_WITH_NORMALS)
        {
            result.others_size += sizeof(vec3);
        }
        if (flags & MESH_CREATION_FLAGS_WITH_TANGENTS)
        {
            result.others_size += sizeof(vec3);
        }
        if (flags & MESH_CREATION_FLAGS_WITH_SKIN)
        {
            result.skin_offset = result.others_size;
            result.others_size += sizeof(SkinningWeights);
        }
        if (flags & MESH_CREATION_FLAGS_WITH_COLORS)
        {
            result.others_size += sizeof(uint32);
        }
        return result;
    }

    MeshBuilder::MeshBuilder(uint32 vertex_flags, MeshIndexType index_type)
    {
        position_arena.initialize(0, gb(1));
        others_arena.initialize(0, gb(1));
        index_arena.initialize(0, gb(1));

        this->vertex_flags = vertex_flags;
        this->index_type = index_type;
        index_size = (index_type == MESH_INDEX_UINT16) ? sizeof(uint16) : sizeof(uint32);

        VertexInfo info = get_vertex_info(vertex_flags);
        others_size = info.others_size;
        skin_offset = info.skin_offset;
    }

    MeshBuilder::~MeshBuilder()
    {
    }

    void MeshBuilder::push_quad(const QuadMeshDesc& desc, bool flip_winding_order)
    {
        vec3 center = desc.center;
        vec2 radius = desc.radius;
        quat rotation = desc.rotation;

        auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(4, 2);

        vec3 x_axis = rotation * vec3(1.f, 0.f, 0.f);
        vec3 y_axis = rotation * vec3(0.f, 1.f, 0.f);
        vec3 z_axis = rotation * vec3(0.f, 0.f, 1.f);

        vec3 x = x_axis * radius.x;
        vec3 y = y_axis * radius.y;

        pushVertex(center - x - y, vec2(0.f, 0.f), z_axis, y_axis, {}, 0);
        pushVertex(center + x - y, vec2(1.f, 0.f), z_axis, y_axis, {}, 0);
        pushVertex(center - x + y, vec2(0.f, 1.f), z_axis, y_axis, {}, 0);
        pushVertex(center + x + y, vec2(1.f, 1.f), z_axis, y_axis, {}, 0);

        pushTriangle(0, 1, 2);
        pushTriangle(1, 3, 2);
    }

    void MeshBuilder::push_box(const BoxMeshDesc& desc, bool flip_winding_order)
    {
        vec3 center = desc.center;
        vec3 radius = desc.radius;
        quat rotation = desc.rotation;

        vec3 x_axis = rotation * vec3(1.f, 0.f, 0.f);
        vec3 y_axis = rotation * vec3(0.f, 1.f, 0.f);
        vec3 z_axis = rotation * vec3(0.f, 0.f, 1.f);

        vec3 x = x_axis * radius.x;
        vec3 y = y_axis * radius.y;
        vec3 z = z_axis * radius.z;

        if ((vertex_flags & MESH_CREATION_FLAGS_WITH_POSITIONS) && !(vertex_flags & MESH_CREATION_FLAGS_WITH_UVS) && !(vertex_flags & MESH_CREATION_FLAGS_WITH_NORMALS) && !(vertex_flags & MESH_CREATION_FLAGS_WITH_TANGENTS))
        {
            auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(8, 12);

            pushVertex(center - x - y + z, {}, {}, {}, {}, 0); // 0
            pushVertex(center + x - y + z, {}, {}, {}, {}, 0); // x
            pushVertex(center - x + y + z, {}, {}, {}, {}, 0); // y
            pushVertex(center + x + y + z, {}, {}, {}, {}, 0); // xy
            pushVertex(center - x - y - z, {}, {}, {}, {}, 0); // z
            pushVertex(center + x - y - z, {}, {}, {}, {}, 0); // xz
            pushVertex(center - x + y - z, {}, {}, {}, {}, 0); // yz
            pushVertex(center + x + y - z, {}, {}, {}, {}, 0); // xyz

            pushTriangle(0, 1, 2);
            pushTriangle(1, 3, 2);
            pushTriangle(1, 5, 3);
            pushTriangle(5, 7, 3);
            pushTriangle(5, 4, 7);
            pushTriangle(4, 6, 7);
            pushTriangle(4, 0, 6);
            pushTriangle(0, 2, 6);
            pushTriangle(2, 3, 6);
            pushTriangle(3, 7, 6);
            pushTriangle(4, 5, 0);
            pushTriangle(5, 1, 0);
        }
        else
        {
            auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(24, 12);

            pushVertex(center - x - y + z, vec2(0.f, 0.f), z_axis, y_axis, {}, 0);
            pushVertex(center + x - y + z, vec2(1.f, 0.f), z_axis, y_axis, {}, 0);
            pushVertex(center - x + y + z, vec2(0.f, 1.f), z_axis, y_axis, {}, 0);
            pushVertex(center + x + y + z, vec2(1.f, 1.f), z_axis, y_axis, {}, 0);
            pushVertex(center + x - y + z, vec2(0.f, 0.f), x_axis, y_axis, {}, 0);
            pushVertex(center + x - y - z, vec2(1.f, 0.f), x_axis, y_axis, {}, 0);
            pushVertex(center + x + y + z, vec2(0.f, 1.f), x_axis, y_axis, {}, 0);
            pushVertex(center + x + y - z, vec2(1.f, 1.f), x_axis, y_axis, {}, 0);
            pushVertex(center + x - y - z, vec2(0.f, 0.f), -z_axis, y_axis, {}, 0);
            pushVertex(center - x - y - z, vec2(1.f, 0.f), -z_axis, y_axis, {}, 0);
            pushVertex(center + x + y - z, vec2(0.f, 1.f), -z_axis, y_axis, {}, 0);
            pushVertex(center - x + y - z, vec2(1.f, 1.f), -z_axis, y_axis, {}, 0);
            pushVertex(center - x - y - z, vec2(0.f, 0.f), -x_axis, y_axis, {}, 0);
            pushVertex(center - x - y + z, vec2(1.f, 0.f), -x_axis, y_axis, {}, 0);
            pushVertex(center - x + y - z, vec2(0.f, 1.f), -x_axis, y_axis, {}, 0);
            pushVertex(center - x + y + z, vec2(1.f, 1.f), -x_axis, y_axis, {}, 0);
            pushVertex(center - x + y + z, vec2(0.f, 0.f), y_axis, x_axis, {}, 0);
            pushVertex(center + x + y + z, vec2(1.f, 0.f), y_axis, x_axis, {}, 0);
            pushVertex(center - x + y - z, vec2(0.f, 1.f), y_axis, x_axis, {}, 0);
            pushVertex(center + x + y - z, vec2(1.f, 1.f), y_axis, x_axis, {}, 0);
            pushVertex(center - x - y - z, vec2(0.f, 0.f), -y_axis, x_axis, {}, 0);
            pushVertex(center + x - y - z, vec2(1.f, 0.f), -y_axis, x_axis, {}, 0);
            pushVertex(center - x - y + z, vec2(0.f, 1.f), -y_axis, x_axis, {}, 0);
            pushVertex(center + x - y + z, vec2(1.f, 1.f), -y_axis, x_axis, {}, 0);

            pushTriangle(0, 1, 2);
            pushTriangle(1, 3, 2);
            pushTriangle(4, 5, 6);
            pushTriangle(5, 7, 6);
            pushTriangle(8, 9, 10);
            pushTriangle(9, 11, 10);
            pushTriangle(12, 13, 14);
            pushTriangle(13, 15, 14);
            pushTriangle(16, 17, 18);
            pushTriangle(17, 19, 18);
            pushTriangle(20, 21, 22);
            pushTriangle(21, 23, 22);
        }
    }

    void MeshBuilder::push_tesselated_box(const TesselatedBoxMeshDesc& desc, bool flip_winding_order)
    {
        const uint32 num_vertices_per_edge = desc.num_intervals + 1;
        const uint32 num_vertices = 6 * num_vertices_per_edge * num_vertices_per_edge - 12 * num_vertices_per_edge + 8;
        const uint32 num_triangles = desc.num_intervals * desc.num_intervals * 2 * 6;

        auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(num_vertices, num_triangles);

        float distance_between_vertices = 1.f / desc.num_intervals;

        uint32 vertex_index = 0;
        uint32 triangle_index = 0;

        vec3 center = desc.center;
        quat rotation = desc.rotation;
        vec3 radius = desc.radius;

        // Upper face
        for (uint32 z = 0; z < num_vertices_per_edge; ++z)
        {
            for (uint32 x = 0; x < num_vertices_per_edge; ++x)
            {
                ASSERT(vertex_index < num_vertices);
                pushVertex(center + rotation * (radius * vec3(x * distance_between_vertices, 1.f, z * distance_between_vertices)), {}, {}, {}, {}, 0);

                if (z < num_vertices_per_edge - 1 && x < num_vertices_per_edge - 1)
                {
                    ASSERT(triangle_index < num_triangles);
                    pushTriangle(z * num_vertices_per_edge + x, (z + 1) * num_vertices_per_edge + x, (z + 1) * num_vertices_per_edge + (x + 1));
                    ASSERT(triangle_index < num_triangles);
                    pushTriangle((z + 1) * num_vertices_per_edge + (x + 1), z * num_vertices_per_edge + (x + 1), z * num_vertices_per_edge + x);

                    triangle_index += 2;
                }

                ++vertex_index;
            }
        }

        const uint32 ring_size = (num_vertices_per_edge - 1) * 4;

        // Sides
        uint32 y = 1;

        vec3 directions[] = {vec3(1.f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), vec3(-1.f, 0.f, 0.f), vec3(0.f, 0.f, -1.f)};
        uint32 direction_index = 0;

        vec3 current_pos(0.f, 1.f - y * distance_between_vertices, 0.f);

        uint32 ring_start = vertex_index;

        while (y < num_vertices_per_edge)
        {
            vec3 direction = directions[direction_index];
            for (uint32 i = 0; i < num_vertices_per_edge - 1; ++i)
            {
                ASSERT(vertex_index < num_vertices);
                pushVertex(desc.center + desc.rotation * (radius * current_pos), {}, {}, {}, {}, 0);
                current_pos += distance_between_vertices * direction;

                uint32 top, top_left;
                if (y == 1)
                {
                    switch (direction_index)
                    {
                        case 0:
                            top = i;
                            top_left = top + 1;
                            break;
                        case 1:
                            top = i * num_vertices_per_edge + (num_vertices_per_edge - 1);
                            top_left = top + num_vertices_per_edge;
                            break;
                        case 2:
                            top = num_vertices_per_edge * num_vertices_per_edge - 1 - i;
                            top_left = top - 1;
                            break;
                        case 3:
                            top = (num_vertices_per_edge - 1 - i) * num_vertices_per_edge;
                            top_left = top - num_vertices_per_edge;
                            break;
                    }
                }
                else
                {
                    top = vertex_index - ring_size;
                    top_left = top + 1;
                    if (ring_start == top_left)
                    {
                        top_left -= ring_size;
                    }
                }

                uint32 left = vertex_index + 1;
                if (ring_start + ring_size == left)
                {
                    left -= ring_size;
                }

                ASSERT(triangle_index < num_triangles);
                pushTriangle(top_left, left, vertex_index);
                ASSERT(triangle_index < num_triangles);
                pushTriangle(vertex_index, top, top_left);

                triangle_index += 2;

                ++vertex_index;
            }

            direction_index = (direction_index + 1) % 4;
            if (direction_index == 0)
            {
                ++y;
                ring_start = vertex_index;
                current_pos.y = 1.f - y * distance_between_vertices;
            }
        }

        // Bottom face
        uint32 first_of_last_ring = vertex_index - ring_size;
        for (uint32 z = 1; z < num_vertices_per_edge - 1; ++z)
        {
            for (uint32 x = 1; x < num_vertices_per_edge - 1; ++x)
            {
                ASSERT(vertex_index < num_vertices);
                pushVertex(desc.center + desc.rotation * (radius * vec3(x * distance_between_vertices, 0.f, z * distance_between_vertices)), {}, {}, {}, {}, 0);

                uint32 top = vertex_index - (num_vertices_per_edge - 2);
                if (z == 1)
                {
                    top = first_of_last_ring + x;
                }

                uint32 right = vertex_index + 1;
                uint32 top_right = vertex_index + 1 - (num_vertices_per_edge - 2);
                if (x == num_vertices_per_edge - 2)
                {
                    right = first_of_last_ring + num_vertices_per_edge - 1 + z;
                    top_right = right - 1;
                }
                else if (z == 1)
                {
                    top_right = top + 1;
                }

                // Only valid if x == 1
                uint32 left = first_of_last_ring + ring_size - z;

                if (x == 1)
                {
                    uint32 top_left = left + 1;
                    if (first_of_last_ring + ring_size == top_left)
                    {
                        top_left -= ring_size;
                    }
                    ASSERT(triangle_index < num_triangles);
                    pushTriangle(top_left, top, vertex_index);
                    ASSERT(triangle_index < num_triangles);
                    pushTriangle(vertex_index, left, top_left);

                    triangle_index += 2;
                }

                ASSERT(triangle_index < num_triangles);
                pushTriangle(top, top_right, right);
                ASSERT(triangle_index < num_triangles);
                pushTriangle(right, vertex_index, top);

                triangle_index += 2;

                if (z == num_vertices_per_edge - 2)
                {
                    uint32 bottom = first_of_last_ring + (num_vertices_per_edge - 1) * 3 - x;
                    uint32 bottom_left = bottom + 1;
                    uint32 bottom_right = bottom - 1;

                    if (x == 1)
                    {
                        ASSERT(triangle_index < num_triangles);
                        pushTriangle(left, vertex_index, bottom);
                        ASSERT(triangle_index < num_triangles);
                        pushTriangle(bottom, bottom_left, left);

                        triangle_index += 2;
                    }

                    ASSERT(triangle_index < num_triangles);
                    pushTriangle(vertex_index, right, bottom_right);
                    ASSERT(triangle_index < num_triangles);
                    pushTriangle(bottom_right, bottom, vertex_index);

                    triangle_index += 2;
                }

                ++vertex_index;
            }
        }

        if (desc.num_intervals == 1)
        {
            // Connect last two faces
            ASSERT(num_vertices == 8);

            pushTriangle(4, 5, 7);
            pushTriangle(5, 6, 7);

            triangle_index += 2;
        }

        ASSERT(vertex_index == num_vertices);
        ASSERT(triangle_index == num_triangles);
    }

    void MeshBuilder::push_sphere(const SphereMeshDesc& desc, bool flip_winding_order)
    {
        uint32 slices = desc.slices;
        uint32 rows = desc.rows;
        vec3 center = desc.center;
        float radius = desc.radius;

        ASSERT(slices > 2);
        ASSERT(rows > 0);

        float vert_delta_angle = M_PI / (rows + 1);
        float horz_delta_angle = 2.f * M_PI / slices;

        auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(slices * rows + 2, 2 * rows * slices);

        // Vertices
        pushVertex(center + vec3(0.f, -radius, 0.f), direction_to_panorama_uv(vec3(0.f, -1.f, 0.f)), vec3(0.f, -1.f, 0.f), vec3(1.f, 0.f, 0.f), {}, 0);

        for (uint32 y = 0; y < rows; ++y)
        {
            float vert_angle = (y + 1) * vert_delta_angle - M_PI;
            float vertex_y = cos(vert_angle);
            float current_circle_radius = sin(vert_angle);
            for (uint32 x = 0; x < slices; ++x)
            {
                float horz_angle = x * horz_delta_angle;
                float vertex_x = cos(horz_angle) * current_circle_radius;
                float vertex_z = sin(horz_angle) * current_circle_radius;
                vec3 pos(vertex_x * radius, vertex_y * radius, vertex_z * radius);
                vec3 nor(vertex_x, vertex_y, vertex_z);

                pushVertex(center + pos, direction_to_panorama_uv(nor), normalize(nor), normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
            }
        }

        pushVertex(center + vec3(0.f, radius, 0.f), direction_to_panorama_uv(vec3(0.f, 1.f, 0.f)), vec3(0.f, 1.f, 0.f), vec3(1.f, 0.f, 0.f), {}, 0);

        uint32 last_vertex = slices * rows + 2;

        // Indices
        for (uint32 x = 0; x < slices - 1; ++x)
        {
            pushTriangle(0, x + 1, x + 2);
        }
        pushTriangle(0, slices, 1);

        for (uint32 y = 0; y < rows - 1; ++y)
        {
            for (uint32 x = 0; x < slices - 1; ++x)
            {
                pushTriangle(y * slices + 1 + x, (y + 1) * slices + 2 + x, y * slices + 2 + x);
                pushTriangle(y * slices + 1 + x, (y + 1) * slices + 1 + x, (y + 1) * slices + 2 + x);
            }
            pushTriangle((uint32)(y * slices + slices), (uint32)((y + 1) * slices + 1), (uint32)(y * slices + 1));
            pushTriangle((uint32)(y * slices + slices), (uint32)((y + 1) * slices + slices), (uint32)((y + 1) * slices + 1));
        }
        for (uint32 x = 0; x < slices - 1; ++x)
        {
            pushTriangle(last_vertex - 2 - x, last_vertex - 3u - x, last_vertex - 1);
        }
        pushTriangle(last_vertex - 1 - slices, last_vertex - 2, last_vertex - 1);
    }

    void MeshBuilder::push_ico_sphere(const IcosphereMeshDesc& desc, bool flip_winding_order)
    {
        vec3 center = desc.center;
        float radius = desc.radius;
        uint32 refinement = desc.refinement;

        struct Vert
        {
            vec3 p;
            vec3 n;
            vec3 t;
        };

        std::vector<Vert> vertices;
        std::vector<IndexedTriangle32> triangles;

        const float t = (1.f + sqrt(5.f)) / 2.f;

#define push_ico_vertex(p)                                     \
    {                                                          \
        vec3 nor = normalize(p);                               \
        vec3 px = nor * radius;                                \
        vec3 tan = normalize(cross(vec3(0.f, 1.f, 0.f), nor)); \
        vertices.push_back({px, nor, tan});                    \
    }

        push_ico_vertex(vec3(-1.f, t, 0));
        push_ico_vertex(vec3(1.f, t, 0));
        push_ico_vertex(vec3(-1.f, -t, 0));
        push_ico_vertex(vec3(1.f, -t, 0));

        push_ico_vertex(vec3(0, -1.f, t));
        push_ico_vertex(vec3(0, 1.f, t));
        push_ico_vertex(vec3(0, -1.f, -t));
        push_ico_vertex(vec3(0, 1.f, -t));

        push_ico_vertex(vec3(t, 0, -1.f));
        push_ico_vertex(vec3(t, 0, 1.f));
        push_ico_vertex(vec3(-t, 0, -1.f));
        push_ico_vertex(vec3(-t, 0, 1.f));

        triangles.push_back({0, 11, 5});
        triangles.push_back({0, 5, 1});
        triangles.push_back({0, 1, 7});
        triangles.push_back({0, 7, 10});
        triangles.push_back({0, 10, 11});
        triangles.push_back({1, 5, 9});
        triangles.push_back({5, 11, 4});
        triangles.push_back({11, 10, 2});
        triangles.push_back({10, 7, 6});
        triangles.push_back({7, 1, 8});
        triangles.push_back({3, 9, 4});
        triangles.push_back({3, 4, 2});
        triangles.push_back({3, 2, 6});
        triangles.push_back({3, 6, 8});
        triangles.push_back({3, 8, 9});
        triangles.push_back({4, 9, 5});
        triangles.push_back({2, 4, 11});
        triangles.push_back({6, 2, 10});
        triangles.push_back({8, 6, 7});
        triangles.push_back({9, 8, 1});

        std::unordered_map<uint32, uint32> midpoints;

        auto get_middle_point = [&midpoints, &vertices, radius](uint32 a, uint32 b)
        {
            uint32 edge = (min(a, b) << 16) | (max(a, b));
            auto it = midpoints.find(edge);
            if (it == midpoints.end())
            {
                vec3 point1 = vertices[a].p;
                vec3 point2 = vertices[b].p;

                vec3 center = 0.5f * (point1 + point2);
                push_ico_vertex(center);

                uint32 index = (uint32)vertices.size() - 1;

                midpoints.insert({edge, index});
                return index;
            }

            return it->second;
        };

        for (uint32 r = 0; r < refinement; ++r)
        {
            std::vector<IndexedTriangle32> refined_triangles;

            for (uint32 tri = 0; tri < (uint32)triangles.size(); ++tri)
            {
                IndexedTriangle32& t = triangles[tri];

                uint32 a = get_middle_point(t.a, t.b);
                uint32 b = get_middle_point(t.b, t.c);
                uint32 c = get_middle_point(t.c, t.a);

                refined_triangles.push_back({t.a, a, c});
                refined_triangles.push_back({t.b, b, a});
                refined_triangles.push_back({t.c, c, b});
                refined_triangles.push_back({a, b, c});
            }

            triangles = refined_triangles;
        }

        auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive((uint32)vertices.size(), (uint32)triangles.size());

        for (const Vert& v : vertices)
        {
            pushVertex(v.p + center, {}, v.n, v.t, {}, 0);
        }

        for (auto t : triangles)
        {
            pushTriangle(t.a, t.b, t.c);
        }

#undef push_ico_vertex
    }

    void MeshBuilder::push_capsule(const CapsuleMeshDesc& desc, bool flip_winding_order)
    {
        uint32 slices = desc.slices;
        uint32 rows = desc.rows;
        float height = desc.height;
        float radius = desc.radius;
        quat rotation = desc.rotation;
        vec3 center = desc.center;

        ASSERT(slices > 2);
        ASSERT(rows > 0);
        ASSERT(rows % 2 == 1);

        float vert_delta_angle = M_PI / (rows + 1);
        float horz_delta_angle = 2.f * M_PI / slices;
        float half_height = 0.5f * height;
        float tex_stretch = radius / (radius + half_height);

        auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(slices * (rows + 1) + 2, 2 * (rows + 1) * slices);

        // Vertices
        pushVertex(rotation * vec3(0.f, -radius - half_height, 0.f) + center,
            direction_to_panorama_uv(vec3(0.f, -1.f, 0.f)),
            rotation * vec3(0.f, -1.f, 0.f),
            rotation * vec3(1.f, 0.f, 0.f), {}, 0);

        for (uint32 y = 0; y < rows / 2 + 1; ++y)
        {
            float vert_angle = (y + 1) * vert_delta_angle - M_PI;
            float vertex_y = cosf(vert_angle);
            float current_circle_radius = sinf(vert_angle);
            for (uint32 x = 0; x < slices; ++x)
            {
                float horz_angle = x * horz_delta_angle;
                float vertex_x = cosf(horz_angle) * current_circle_radius;
                float vertex_z = sinf(horz_angle) * current_circle_radius;
                vec3 pos(vertex_x * radius, vertex_y * radius - half_height, vertex_z * radius);
                vec3 nor(vertex_x, vertex_y, vertex_z);

                vec2 uv = direction_to_panorama_uv(nor);
                uv.y *= tex_stretch;
                pushVertex(rotation * pos + center, uv, rotation * normalize(nor), rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
            }
        }
        for (uint32 y = 0; y < rows / 2 + 1; ++y)
        {
            float vert_angle = (y + rows / 2 + 1) * vert_delta_angle - M_PI;
            float vertex_y = cosf(vert_angle);
            float current_circle_radius = sinf(vert_angle);
            for (uint32 x = 0; x < slices; ++x)
            {
                float horz_angle = x * horz_delta_angle;
                float vertex_x = cosf(horz_angle) * current_circle_radius;
                float vertex_z = sinf(horz_angle) * current_circle_radius;
                vec3 pos(vertex_x * radius, vertex_y * radius + half_height, vertex_z * radius);
                vec3 nor(vertex_x, vertex_y, vertex_z);

                vec2 uv = direction_to_panorama_uv(nor);
                uv.y *= tex_stretch;
                pushVertex(rotation * pos + center, uv, rotation * normalize(nor), rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
            }
        }
        pushVertex(rotation * vec3(0.f, radius + half_height, 0.f) + center,
            direction_to_panorama_uv(vec3(0.f, 1.f, 0.f)),
            rotation * vec3(0.f, 1.f, 0.f),
            rotation * vec3(1.f, 0.f, 0.f), {}, 0);

        uint32 last_vertex = slices * (rows + 1) + 2;

        // Indices
        for (uint32 x = 0; x < slices - 1; ++x)
        {
            pushTriangle(0, x + 1, x + 2);
        }
        pushTriangle(0, slices, 1);
        for (uint32 y = 0; y < rows; ++y)
        {
            for (uint32 x = 0; x < slices - 1; ++x)
            {
                pushTriangle(y * slices + 1 + x, (y + 1) * slices + 2 + x, y * slices + 2 + x);
                pushTriangle(y * slices + 1 + x, (y + 1) * slices + 1 + x, (y + 1) * slices + 2 + x);
            }
            pushTriangle(y * slices + slices, (y + 1) * slices + 1, y * slices + 1);
            pushTriangle(y * slices + slices, (y + 1) * slices + slices, (y + 1) * slices + 1);
        }
        for (uint32 x = 0; x < slices - 1; ++x)
        {
            pushTriangle(last_vertex - 2 - x, last_vertex - 3 - x, last_vertex - 1);
        }
        pushTriangle(last_vertex - 1 - slices, last_vertex - 2, last_vertex - 1);
    }

    struct Sincos
    {
        float sin, cos;
    };

    void MeshBuilder::push_cylinder(const CylinderMeshDesc& desc, bool flip_winding_order)
    {
        uint32 slices = desc.slices;
        float height = desc.height;
        float radius = desc.radius;
        quat rotation = desc.rotation;
        vec3 center = desc.center;

        ASSERT(slices > 2);

        float horz_delta_angle = 2.f * M_PI / slices;
        float half_height = height * 0.5f;

        auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(4 * slices + 2, 4 * slices);

        Sincos* angles = (Sincos*)alloca(sizeof(Sincos) * slices);
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            angles[x] = {sinf(horz_angle), cosf(horz_angle)};
        }

        pushVertex(center + rotation * vec3(0.f, -half_height, 0.f), vec2(0.25f, 0.75f), rotation * vec3(0.f, -1.f, 0.f), rotation * vec3(1.f, 0.f, 0.f), {}, 0);

        // Bottom row, normal down
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = angles[x].cos;
            float vertex_z = angles[x].sin;
            vec3 pos(vertex_x * radius, -half_height, vertex_z * radius);
            vec3 nor(0.f, -1.f, 0.f);

            pushVertex(center + rotation * pos, remap(vec2(vertex_x, vertex_z), -1.f, 1.f, vec2(0.f, 0.5f), vec2(0.5f, 1.f)), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        // Bottom row, normal around
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = angles[x].cos;
            float vertex_z = angles[x].sin;
            vec3 pos(vertex_x * radius, -half_height, vertex_z * radius);
            vec3 nor(vertex_x, 0.f, vertex_z);

            pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.5f), rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top row, normal around
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = angles[x].cos;
            float vertex_z = angles[x].sin;
            vec3 pos(vertex_x * radius, half_height, vertex_z * radius);
            vec3 nor(vertex_x, 0.f, vertex_z);

            pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.f), rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top row, normal up
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = angles[x].cos;
            float vertex_z = angles[x].sin;
            vec3 pos(vertex_x * radius, half_height, vertex_z * radius);
            vec3 nor(0.f, 1.f, 0.f);

            pushVertex(center + rotation * pos, remap(vec2(vertex_x, vertex_z), -1.f, 1.f, vec2(0.5f, 0.5f), vec2(1.f, 1.f)), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        pushVertex(center + rotation * vec3(0.f, half_height, 0.f), vec2(0.75f, 0.75f), rotation * vec3(0.f, 1.f, 0.f), rotation * vec3(1.f, 0.f, 0.f), {}, 0);

        uint32 last_vertex = 4 * slices + 2;

        // Indices
        for (uint32 x = 0; x < slices - 1u; ++x)
        {
            pushTriangle(0, x + 1, x + 2);
        }
        pushTriangle(0, slices, 1);

        for (uint32 x = 0; x < slices - 1u; ++x)
        {
            pushTriangle(slices + 1 + x, 2 * slices + 2 + x, slices + 2 + x);
            pushTriangle(slices + 1 + x, 2 * slices + 1 + x, 2 * slices + 2 + x);
        }
        pushTriangle(slices + slices, 2 * slices + 1, slices + 1);
        pushTriangle(slices + slices, 2 * slices + slices, 2 * slices + 1);

        for (uint32 x = 0; x < slices - 1u; ++x)
        {
            pushTriangle(last_vertex - 2 - x, last_vertex - 3 - x, last_vertex - 1);
        }
        pushTriangle(last_vertex - 1 - slices, last_vertex - 2, last_vertex - 1);
    }

    void MeshBuilder::push_hollow_cylinder(const HollowCylinderMeshDesc& desc, bool flip_winding_order)
    {
        uint32 slices = desc.slices;
        float height = desc.height;
        float radius = desc.radius;
        float inner_radius = desc.inner_radius;
        quat rotation = desc.rotation;
        vec3 center = desc.center;

        ASSERT(slices > 2);

        float horz_delta_angle = 2.f * M_PI / slices;
        float half_height = height * 0.5f;

        auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(8 * slices, 8 * slices);

        Sincos* angles = (Sincos*)alloca(sizeof(Sincos) * slices);
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            angles[x] = {sinf(horz_angle), cosf(horz_angle)};
        }

        float radius_relation = inner_radius / radius;

        // Bottom outer row, normal down
        for (uint32 x = 0; x < slices; ++x)
        {
            float vertex_x = angles[x].cos;
            float vertex_z = angles[x].sin;
            vec3 pos(vertex_x * radius, -half_height, vertex_z * radius);
            vec3 nor(0.f, -1.f, 0.f);

            pushVertex(center + rotation * pos, remap(vec2(vertex_x, vertex_z), -1.f, 1.f, vec2(0.f, 0.5f), vec2(0.5f, 1.f)), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        // Bottom row, normal around
        for (uint32 x = 0; x < slices; ++x)
        {
            float vertex_x = angles[x].cos;
            float vertex_z = angles[x].sin;
            vec3 pos(vertex_x * radius, -half_height, vertex_z * radius);
            vec3 nor(vertex_x, 0.f, vertex_z);

            pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.5f), rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top row, normal around
        for (uint32 x = 0; x < slices; ++x)
        {
            float vertex_x = angles[x].cos;
            float vertex_z = angles[x].sin;
            vec3 pos(vertex_x * radius, half_height, vertex_z * radius);
            vec3 nor(vertex_x, 0.f, vertex_z);

            pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.f), rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top row, normal up
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * radius, half_height, vertex_z * radius);
            vec3 nor(0.f, 1.f, 0.f);

            pushVertex(center + rotation * pos, remap(vec2(vertex_x, vertex_z), -1.f, 1.f, vec2(0.5f, 0.5f), vec2(1.f, 1.f)), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        vec2 inner_uv_min = vec2(0.25f, 0.75f) - 0.25f * radius_relation;
        vec2 inner_uv_max = vec2(0.25f, 0.75f) + 0.25f * radius_relation;

        // Bottom inner row, normal down
        for (uint32 x = 0; x < slices; ++x)
        {
            float vertex_x = angles[x].cos;
            float vertex_z = angles[x].sin;
            vec3 pos(vertex_x * inner_radius, -half_height, vertex_z * inner_radius);
            vec3 nor(0.f, -1.f, 0.f);

            pushVertex(center + rotation * pos, remap(vec2(vertex_x, vertex_z), -1.f, 1.f, inner_uv_min, inner_uv_max), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        // Bottom inner row, normal inside
        for (uint32 x = 0; x < slices; ++x)
        {
            float vertex_x = angles[x].cos;
            float vertex_z = angles[x].sin;
            vec3 pos(vertex_x * inner_radius, -half_height, vertex_z * inner_radius);
            vec3 nor(-vertex_x, 0.f, -vertex_z);

            pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.5f), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        // Top inner row, normal inside
        for (uint32 x = 0; x < slices; ++x)
        {
            float vertex_x = angles[x].cos;
            float vertex_z = angles[x].sin;
            vec3 pos(vertex_x * inner_radius, half_height, vertex_z * inner_radius);
            vec3 nor(-vertex_x, 0.f, -vertex_z);

            pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.f), rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top inner row, normal up
        for (uint32 x = 0; x < slices; ++x)
        {
            float vertex_x = angles[x].cos;
            float vertex_z = angles[x].sin;
            vec3 pos(vertex_x * inner_radius, half_height, vertex_z * inner_radius);
            vec3 nor(0.f, 1.f, 0.f);

            pushVertex(center + rotation * pos, remap(vec2(vertex_x, vertex_z), -1.f, 1.f, inner_uv_min + vec2(0.5f, 0.f), inner_uv_max + vec2(0.5f, 0.f)), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        // Indices
        uint32 first_inner_bottom_vertex = 4 * slices;

        for (uint32 x = 0; x < slices - 1u; ++x)
        {
            pushTriangle(first_inner_bottom_vertex + x, x, x + 1);
            pushTriangle(first_inner_bottom_vertex + x, x + 1, first_inner_bottom_vertex + x + 1);
        }
        pushTriangle(first_inner_bottom_vertex + slices - 1u, slices - 1u, 0);
        pushTriangle(first_inner_bottom_vertex + slices - 1u, 0, first_inner_bottom_vertex);

        for (uint32 x = 0; x < slices - 1u; ++x)
        {
            pushTriangle(slices + x, 2 * slices + 1 + x, slices + 1 + x);
            pushTriangle(slices + x, 2 * slices + x, 2 * slices + 1 + x);
        }
        pushTriangle(slices + slices - 1, 2 * slices, slices);
        pushTriangle(slices + slices - 1, 2 * slices + slices - 1, 2 * slices);

        uint32 first_inner_top_vertex = 7 * slices;
        uint32 first_outer_top_vertex = 3 * slices;

        for (uint32 x = 0; x < slices - 1u; ++x)
        {
            pushTriangle(first_inner_top_vertex + x, first_outer_top_vertex + x + 1, first_outer_top_vertex + x);
            pushTriangle(first_inner_top_vertex + x, first_inner_top_vertex + x + 1, first_outer_top_vertex + x + 1);
        }
        pushTriangle(first_inner_top_vertex + slices - 1u, first_outer_top_vertex + 0, first_outer_top_vertex + slices - 1u);
        pushTriangle(first_inner_top_vertex + slices - 1u, first_inner_top_vertex, first_outer_top_vertex + 0);

        first_inner_bottom_vertex = 5 * slices;
        first_inner_top_vertex = 6 * slices;

        for (uint32 x = 0; x < slices - 1u; ++x)
        {
            pushTriangle(first_inner_bottom_vertex + x, first_inner_bottom_vertex + 1 + x, first_inner_top_vertex + 1 + x);
            pushTriangle(first_inner_bottom_vertex + x, first_inner_top_vertex + 1 + x, first_inner_top_vertex + x);
        }
        pushTriangle(first_inner_bottom_vertex + slices - 1, first_inner_bottom_vertex, first_inner_top_vertex);
        pushTriangle(first_inner_bottom_vertex + slices - 1, first_inner_top_vertex, first_inner_top_vertex + slices - 1);
    }

    void MeshBuilder::push_arrow(const ArrowMeshDesc& desc, bool flip_winding_order)
    {
        float shaft_length = desc.shaft_length;
        float shaft_radius = desc.shaft_radius;
        float head_length = desc.head_length;
        float head_radius = desc.head_radius;
        quat rotation = desc.rotation;
        vec3 base = desc.base;
        uint32 slices = desc.slices;

        ASSERT(slices > 2);

        float horz_delta_angle = 2.f * M_PI / slices;

        auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(7 * slices + 1, 7 * slices);

        vec2 uv(0.f, 0.f);
        pushVertex(base, uv, rotation * vec3(0.f, -1.f, 0.f), rotation * vec3(1.f, 0.f, 0.f), {}, 0);

        // Bottom row, normal down
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * shaft_radius, 0.f, vertex_z * shaft_radius);
            vec3 nor(0.f, -1.f, 0.f);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        // Bottom row, normal around
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * shaft_radius, 0.f, vertex_z * shaft_radius);
            vec3 nor(vertex_x, 0.f, vertex_z);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top row, normal around
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * shaft_radius, shaft_length, vertex_z * shaft_radius);
            vec3 nor(vertex_x, 0.f, vertex_z);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top row, normal down
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * shaft_radius, shaft_length, vertex_z * shaft_radius);
            vec3 nor(0.f, -1.f, 0.f);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        // Top outer row, normal down
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * head_radius, shaft_length, vertex_z * head_radius);
            vec3 nor(0.f, -1.f, 0.f);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        vec2 normal_2d = normalize(vec2(head_length, head_radius));

        // Top outer row, normal around
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * head_radius, shaft_length, vertex_z * head_radius);
            vec3 nor(vertex_x * normal_2d.x, normal_2d.y, vertex_z * normal_2d.x);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        // Top vertex
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(0.f, shaft_length + head_length, 0.f);
            vec3 nor(vertex_x * normal_2d.x, normal_2d.y, vertex_z * normal_2d.x);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        // Indices
        for (uint32 x = 0; x < slices - 1; ++x)
        {
            pushTriangle(0, x + 1, x + 2);
        }
        pushTriangle(0, slices, 1);

        for (uint32 y = 1; y < 7; y += 2)
        {
            for (uint32 x = 0; x < slices - 1; ++x)
            {
                pushTriangle(y * slices + 1 + x, (y + 1) * slices + 2 + x, y * slices + 2 + x);
                pushTriangle(y * slices + 1 + x, (y + 1) * slices + 1 + x, (y + 1) * slices + 2 + x);
            }
            pushTriangle(y * slices + slices, (y + 1) * slices + 1, y * slices + 1);
            pushTriangle(y * slices + slices, (y + 1) * slices + slices, (y + 1) * slices + 1);
        }
    }

    void MeshBuilder::push_torus(const TorusMeshDesc& desc, bool flip_winding_order)
    {
        float torus_radius = desc.torus_radius;
        float tube_radius = desc.tube_radius;
        uint32 slices = desc.slices;
        uint32 segments = desc.segments;
        vec3 center = desc.center;
        quat rotation = desc.rotation * quat(vec3(1.f, 0.f, 0.f), deg2rad(90.f));

        ASSERT(slices > 2);
        ASSERT(segments > 2);

        float tube_delta_angle = 2.f * M_PI / slices;
        float torus_delta_angle = 2.f * M_PI / segments;

        auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(segments * slices, segments * slices * 2);

        vec2 uv(0.f, 0.f);

        for (uint32 s = 0; s < segments; ++s)
        {
            float segment_angle = s * torus_delta_angle;
            quat segment_rotation(vec3(0.f, 0.f, 1.f), segment_angle);

            vec3 segment_offset = segment_rotation * vec3(torus_radius, 0.f, 0.f);

            for (uint32 x = 0; x < slices; ++x)
            {
                float horz_angle = x * tube_delta_angle;
                float vertex_x = cosf(horz_angle);
                float vertex_z = sinf(horz_angle);
                vec3 pos = segment_rotation * vec3(vertex_x * tube_radius, 0.f, vertex_z * tube_radius) + segment_offset;
                vec3 nor = segment_rotation * vec3(vertex_x, 0.f, vertex_z);

                pushVertex(center + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
            }
        }

        for (uint32 y = 0; y < segments - 1u; ++y)
        {
            for (uint32 x = 0; x < slices - 1u; ++x)
            {
                pushTriangle(y * slices + x, (y + 1) * slices + 1 + x, y * slices + 1 + x);
                pushTriangle(y * slices + x, (y + 1) * slices + x, (y + 1) * slices + 1 + x);
            }
            pushTriangle(y * slices + slices - 1, (y + 1) * slices, y * slices);
            pushTriangle(y * slices + slices - 1, (y + 1) * slices + slices - 1, (y + 1) * slices);
        }

        uint32 y = segments - 1u;
        for (uint32 x = 0; x < slices - 1u; ++x)
        {
            pushTriangle(y * slices + x, 1 + x, y * slices + 1 + x);
            pushTriangle(y * slices + x, x, 1 + x);
        }
        pushTriangle(y * slices + slices - 1, 0, y * slices);
        pushTriangle(y * slices + slices - 1, slices - 1, 0);
    }

    void MeshBuilder::push_mace(const MaceMeshDesc& desc, bool flip_winding_order)
    {
        float shaft_length = desc.shaft_length;
        float shaft_radius = desc.shaft_radius;
        float head_length = desc.head_length;
        float head_radius = desc.head_radius;
        quat rotation = desc.rotation;
        vec3 base = desc.base;
        uint32 slices = desc.slices;

        ASSERT(slices > 2);

        float horz_delta_angle = 2.f * M_PI / slices;

        auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(8 * slices + 2, 8 * slices);

        vec2 uv(0.f, 0.f);
        pushVertex(base, uv, rotation * vec3(0.f, -1.f, 0.f), rotation * vec3(1.f, 0.f, 0.f), {}, 0);

        // Bottom row, normal down
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * shaft_radius, 0.f, vertex_z * shaft_radius);
            vec3 nor(0.f, -1.f, 0.f);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        // Bottom row, normal around
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * shaft_radius, 0.f, vertex_z * shaft_radius);
            vec3 nor(vertex_x, 0.f, vertex_z);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top row, normal around
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * shaft_radius, shaft_length, vertex_z * shaft_radius);
            vec3 nor(vertex_x, 0.f, vertex_z);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top row, normal down
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * shaft_radius, shaft_length, vertex_z * shaft_radius);
            vec3 nor(0.f, -1.f, 0.f);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        // Top outer row, normal down
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * head_radius, shaft_length, vertex_z * head_radius);
            vec3 nor(0.f, -1.f, 0.f);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top outer row, normal around
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * head_radius, shaft_length, vertex_z * head_radius);
            vec3 nor(vertex_x, 0.f, vertex_z);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top top outer row, normal around
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * head_radius, shaft_length + head_length, vertex_z * head_radius);
            vec3 nor(vertex_x, 0.f, vertex_z);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
        }

        // Top top outer row, normal up
        for (uint32 x = 0; x < slices; ++x)
        {
            float horz_angle = x * horz_delta_angle;
            float vertex_x = cosf(horz_angle);
            float vertex_z = sinf(horz_angle);
            vec3 pos(vertex_x * head_radius, shaft_length + head_length, vertex_z * head_radius);
            vec3 nor(0.f, 1.f, 0.f);

            pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
        }

        pushVertex(base + rotation * vec3(0.f, shaft_length + head_length, 0.f), uv, rotation * vec3(0.f, 1.f, 0.f), rotation * vec3(1.f, 0.f, 0.f), {}, 0);

        uint32 last_vertex = 8 * slices + 2;

        // Indices
        for (uint32 x = 0; x < slices - 1u; ++x)
        {
            pushTriangle(0, x + 1, x + 2);
        }
        pushTriangle(0, slices, 1);

        for (uint32 y = 1; y < 7; y += 2)
        {
            for (uint32 x = 0; x < slices - 1u; ++x)
            {
                pushTriangle(y * slices + 1 + x, (y + 1) * slices + 2 + x, y * slices + 2 + x);
                pushTriangle(y * slices + 1 + x, (y + 1) * slices + 1 + x, (y + 1) * slices + 2 + x);
            }
            pushTriangle(y * slices + slices, (y + 1) * slices + 1, y * slices + 1);
            pushTriangle(y * slices + slices, (y + 1) * slices + slices, (y + 1) * slices + 1);
        }

        for (uint32 x = 0; x < slices - 1u; ++x)
        {
            pushTriangle(last_vertex - 2 - x, last_vertex - 3 - x, last_vertex - 1);
        }
        pushTriangle(last_vertex - 1 - slices, last_vertex - 2, last_vertex - 1);
    }

    void MeshBuilder::push_mesh(const SubmeshAsset& mesh, float scale, BoundingBox* aabb)
    {
        uint32 num_vertices = (uint32)mesh.positions.size();
        uint32 num_faces = (uint32)mesh.triangles.size();

        if (index_type == MESH_INDEX_UINT32)
        {
            ASSERT(num_vertices - 1 <= UINT16_MAX);
        }

        auto [position_ptr, others_ptr, index_ptr, index_offset] = begin_primitive(num_vertices, num_faces);
        auto vertex_others = others_ptr;

        vec3 position(0.f, 0.f, 0.f);
        vec3 normal(0.f, 0.f, 0.f);
        vec3 tangent(0.f, 0.f, 0.f);
        vec2 uv(0.f, 0.f);
        uint32 vertex_color = 0;
        animation::SkinningWeights skin = {};

        bool has_positions = true;
        bool has_uvs = !mesh.uvs.empty();
        bool has_normals = !mesh.normals.empty();
        bool has_tangents = !mesh.tangents.empty();
        bool has_vertex_colors = !mesh.colors.empty();
        ;
        bool has_skin = !mesh.skin.empty();

        if (aabb)
        {
            *aabb = BoundingBox::negative_infinity();
        }

        for (uint32 i = 0; i < num_vertices; ++i)
        {
            position = mesh.positions[i] * scale;
            if (aabb)
            {
                aabb->grow(position);
            }

            if (has_uvs)
            {
                uv = mesh.uvs[i];
            }

            if (has_normals)
            {
                normal = mesh.normals[i];
            }

            if (has_tangents)
            {
                tangent = mesh.tangents[i];
            }

            if (has_vertex_colors)
            {
                vertex_color = mesh.colors[i];
            }

            if (has_skin)
            {
                skin = mesh.skin[i];
            }

            pushVertex(position, uv, normal, tangent, skin, vertex_color);
        }

        const bool flip_winding_order = false;
        for (uint32 i = 0; i < num_faces; ++i)
        {
            const IndexedTriangle32& tri = mesh.triangles[i];
            pushTriangle(tri.a, tri.b, tri.c);
        }
    }

    SubmeshInfo MeshBuilder::end_submesh()
    {
        uint32 first_vertex = total_num_vertices;
        uint32 first_triangle = total_num_triangles;

        SubmeshInfo result;
        result.first_index = first_triangle * 3;
        result.num_indices = num_triangles_in_current_submesh * 3;
        result.base_vertex = first_vertex;
        result.num_vertices = num_vertices_in_current_submesh;

        total_num_vertices += num_vertices_in_current_submesh;
        total_num_triangles += num_triangles_in_current_submesh;

        // Align the next index to a 16-byte boundary
        uint32 aligned_num_triangles = align_to(total_num_triangles, 8); // 8 triangles are 48 bytes, which is divisible by 16.
        uint32 missing = aligned_num_triangles - total_num_triangles;
        if (missing > 0)
        {
            index_arena.allocate(index_size * 3 * missing); // Push arena forward
        }
        total_num_triangles = aligned_num_triangles;

        // Reset next submesh
        num_vertices_in_current_submesh = 0;
        num_triangles_in_current_submesh = 0;

        ++num_submeshes;

        return result;
    }

    DXMesh MeshBuilder::create_dx_mesh()
    {
        if (num_vertices_in_current_submesh > 0)
        {
            end_submesh();
        }

        DXMesh result;
        result.vertex_buffer.positions = create_vertex_buffer(sizeof(vec3), total_num_vertices, position_arena.base());
        if (vertex_flags != MESH_CREATION_FLAGS_WITH_POSITIONS)
        {
            result.vertex_buffer.others = create_vertex_buffer(others_size, total_num_vertices, others_arena.base(), true);
        }
        result.index_buffer = create_index_buffer(index_size, total_num_triangles * 3, index_arena.base());
        return result;
    }

    std::tuple<vec3*, uint8*, uint8*, uint32> MeshBuilder::begin_primitive(uint32 num_vertices, uint32 num_triangles)
    {
        vec3* position_ptr = (vec3*)position_arena.allocate(sizeof(vec3) * num_vertices);
        uint8* others_ptr = (uint8*)others_arena.allocate(others_size * num_vertices);
        uint8* index_ptr = (uint8*)index_arena.allocate(index_size * 3 * num_triangles);

        uint32 index_offset = num_vertices_in_current_submesh;

        num_vertices_in_current_submesh += num_vertices;
        num_triangles_in_current_submesh += num_triangles;

        uint32 max_num_vertices = (index_type == MESH_INDEX_UINT32) ? UINT16_MAX : UINT32_MAX;
        ASSERT(num_vertices_in_current_submesh - 1 <= max_num_vertices);

        return {position_ptr, others_ptr, index_ptr, index_offset};
    }
}