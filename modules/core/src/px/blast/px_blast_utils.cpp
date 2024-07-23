
#include "px/blast/px_blast_utils.h"
#include "px/blast/px_nvmesh.h"

#include "asset/mesh_postprocessing.h"

#include <NvBlastExtAuthoringTypes.h>

namespace era_engine::physics
{
    bounds getCompositeMeshBounds(eentity& entt)
    {
        auto childs = entt.getChilds();

        std::vector<bounds> meshBounds;
        meshBounds.reserve(childs.size());

        if (auto mesh = entt.getComponentIfExists<mesh_component>())
        {
            auto& aabb = mesh->mesh->aabb;

            meshBounds.push_back({ (aabb.maxCorner + aabb.minCorner) / 2.0f, (aabb.maxCorner - aabb.minCorner) / 2.0f });
        }

        for (auto& c : childs)
        {
            if (auto mesh = c.getComponentIfExists<mesh_component>())
            {
                auto& aabb = mesh->mesh->aabb;

                meshBounds.push_back({ (aabb.maxCorner + aabb.minCorner) / 2.0f, (aabb.maxCorner - aabb.minCorner) / 2.0f });
            }
        }

        if (meshBounds.size() == 0)
            return {};

        if (meshBounds.size() == 1)
            return meshBounds[0];

        auto& compositeBounds = meshBounds[0];

        for (size_t i = 1; i < meshBounds.size(); i++)
        {
            compositeBounds.encapsulate(meshBounds[i]);
        }

        return compositeBounds;
    }

    ref<submesh_asset> physics::createRenderMesh(const physics::nvmesh& simpleMesh)
    {
        ref<submesh_asset> asset = make_ref<submesh_asset>();

        auto defaultmat = createPBRMaterialAsync({ "", "" });

        std::vector<vec3> positions; positions.reserve(1 << 16);
        std::vector<vec2> uvs; uvs.reserve(1 << 16);
        std::vector<vec3> normals; normals.reserve(1 << 16);

        std::vector<vec3> positionCache; positionCache.reserve(16);
        std::vector<vec2> uvCache; uvCache.reserve(16);
        std::vector<vec3> normalCache; normalCache.reserve(16);

        size_t nbTriangles = simpleMesh.indices.size() / 3;

        for (auto& vert : simpleMesh.vertices)
        {
            asset->positions.push_back(physx::createVec3(vert));
        }

        for (auto& uv : simpleMesh.uvs)
        {
            asset->uvs.push_back(physx::createVec2(uv));
        }

        for (auto& norm : simpleMesh.normals)
        {
            asset->normals.push_back(physx::createVec3(norm));
        }

        for (uint32_t i = 0; i < simpleMesh.indices.size(); i += 3)
        {
            asset->triangles.push_back(indexed_triangle16{
                (uint16)simpleMesh.indices[i + 0],
                (uint16)simpleMesh.indices[i + 1],
                (uint16)simpleMesh.indices[i + 2] });
        }

        generateNormalsAndTangents(asset, mesh_flag_default);

        return asset;
    }

    void physics::solveOpenEdges(std::vector<uint32_t>& indices)
    {
        std::unordered_map<std::pair<uint32_t, uint32_t>, int, int_pair_hash> edgeCounts;
        std::unordered_map<uint32_t, std::vector<uint32_t>> openEdges;

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            uint32_t v1 = indices[i];
            uint32_t v2 = indices[i + 1];
            uint32_t v3 = indices[i + 2];

            edgeCounts[{v1, v2}]++;
            edgeCounts[{v2, v3}]++;
            edgeCounts[{v3, v1}]++;
        }

        for (const auto& pair : edgeCounts)
        {
            if (pair.second == 1)
            {
                openEdges[pair.first.first].push_back(pair.first.second);
                openEdges[pair.first.second].push_back(pair.first.first);
            }
        }

        for (const auto& pair : openEdges)
        {
            if (pair.second.size() == 2)
            {
                indices.push_back(pair.first);
                indices.push_back(pair.second[0]);
                indices.push_back(pair.second[1]);
            }
        }
    }

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

    std::vector<uint32_t> physics::generateIndices(Nv::Blast::Triangle* triangles, size_t nbTriangles)
    {
        std::vector<vertex> vertices;
        std::vector<uint32_t> indices;

        for (size_t i = 0; i < nbTriangles; i++)
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
                    // Vertex already exists in the vertices vector, so we just need to add the index
                    indices.push_back(std::distance(vertices.begin(), it));
                }
                else
                {
                    // New vertex, so we need to add it to the vertices vector and add the index
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
                    // Vertex already exists in the vertices vector, so we just need to add the index
                    indices.push_back(std::distance(vertices.begin(), it));
                }
                else
                {
                    // New vertex, so we need to add it to the vertices vector and add the index
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
                    // Vertex already exists in the vertices vector, so we just need to add the index
                    indices.push_back(std::distance(vertices.begin(), it));
                }
                else
                {
                    // New vertex, so we need to add it to the vertices vector and add the index
                    vertices.push_back(vertex);
                    indices.push_back(vertices.size() - 1);
                }
            }
        }

        return indices;
    }
}