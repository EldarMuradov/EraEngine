#include "physics/destructions/destruction_utils.h"

#include <NvBlastGlobals.h>
#include <extensions/authoringCommon/NvBlastExtAuthoringMesh.h>
#include <extensions/authoringCommon/NvBlastExtAuthoringTypes.h>

namespace era_engine::physics
{
    std::vector<uint32> DestructionUtils::generate_indices(Nv::Blast::Triangle* triangles, size_t nb_triangles)
    {
        struct vertex
        {
            vec3 position;
            vec3 normal;
            vec2 uv;

            bool operator==(const vertex& other) const
            {
                return position.x == other.position.x && position.y == other.position.y && position.z == other.position.z
                    && normal.x == other.normal.x && normal.y == other.normal.y && normal.z == other.normal.z
                    && uv.x == other.uv.x && uv.y == other.uv.y;
            }
        };

        std::vector<vertex> vertices;
        std::vector<uint32> indices;

        for (size_t i = 0; i < nb_triangles; i++)
        {
            auto& triangle = triangles[i];

            {
                vertex vertex = {
                    vec3(triangle.a.p.x, triangle.a.p.y, triangle.a.p.z),
                    vec3(triangle.a.n.x, triangle.a.n.y, triangle.a.n.z),
                    vec2(triangle.a.uv->x, triangle.a.uv->y) };

                auto it = std::find(vertices.begin(), vertices.end(), vertex);
                if (it != vertices.end())
                {
                    // Vertex already exists in the vertices vector, so we just need to add the index.
                    indices.push_back(std::distance(vertices.begin(), it));
                }
                else
                {
                    // New vertex, so we need to add it to the vertices vector and add the index.
                    vertices.push_back(vertex);
                    indices.push_back(vertices.size() - 1);
                }
            }

            {
                vertex vertex = {
                    vec3(triangle.b.p.x, triangle.b.p.y, triangle.b.p.z),
                    vec3(triangle.b.n.x, triangle.b.n.y, triangle.b.n.z),
                    vec2(triangle.b.uv->x, triangle.b.uv->y) };

                auto it = std::find(vertices.begin(), vertices.end(), vertex);
                if (it != vertices.end())
                {
                    // Vertex already exists in the vertices vector, so we just need to add the index.
                    indices.push_back(std::distance(vertices.begin(), it));
                }
                else
                {
                    // New vertex, so we need to add it to the vertices vector and add the index.
                    vertices.push_back(vertex);
                    indices.push_back(vertices.size() - 1);
                }
            }

            {
                vertex vertex = {
                    vec3(triangle.c.p.x, triangle.c.p.y, triangle.c.p.z),
                    vec3(triangle.c.n.x, triangle.c.n.y, triangle.c.n.z),
                    vec2(triangle.c.uv->x, triangle.c.uv->y) };

                auto it = std::find(vertices.begin(), vertices.end(), vertex);
                if (it != vertices.end())
                {
                    // Vertex already exists in the vertices vector, so we just need to add the index.
                    indices.push_back(std::distance(vertices.begin(), it));
                }
                else
                {
                    // New vertex, so we need to add it to the vertices vector and add the index.
                    vertices.push_back(vertex);
                    indices.push_back(vertices.size() - 1);
                }
            }
        }

        return indices;
    }

    NvBlastID DestructionUtils::generate_id_from_string(const char* str)
    {
        uint32 h[4] = { 5381, 5381, 5381, 5381 };

        int i = 0;

        for (const char* ptr = str; *ptr; i = ((i + 1) & 3), ++ptr)
        {
            h[i] = ((h[i] << 5) + h[i]) ^ static_cast<uint32_t>(*ptr);
        }

        return *reinterpret_cast<NvBlastID*>(h);
    }

}