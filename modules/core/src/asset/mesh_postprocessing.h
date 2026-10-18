// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"
#include "core/hash.h"

#include "geometry/mesh.h"

#include "asset/model_asset.h"

namespace era_engine
{
    struct ERA_CORE_API FullVertex
    {
        vec3 position;
        vec2 uv;
        vec3 normal;
        vec3 tangent;
        uint32 color;
        animation::SkinningWeights skin;
    };

    static bool operator==(const FullVertex& a, const FullVertex& b)
    {
        return memcmp(&a, &b, sizeof(FullVertex)) == 0;
    }
}

namespace std
{
    template <>
    struct hash<era_engine::FullVertex>
    {
        size_t operator()(const era_engine::FullVertex& x) const
        {
            size_t seed = 0;

            hash_combine(seed, x.position);
            hash_combine(seed, x.uv);
            hash_combine(seed, x.normal);
            hash_combine(seed, x.tangent);
            hash_combine(seed, x.color);
            hash_combine(seed, *(uint32*)x.skin.skin_indices.data());
            hash_combine(seed, *(uint32*)x.skin.skin_weights.data());

            return seed;
        }
    };
}

namespace era_engine
{
    struct ERA_CORE_API PerMaterial
    {
        void add_triangles(const std::vector<vec3>& positions, const std::vector<vec2>& uvs, const std::vector<vec3>& normals,
            const std::vector<vec3>& tangents, const std::vector<uint32>& colors, const std::vector<animation::SkinningWeights>& skins,
            int32 first_index, int32 face_size, std::vector<SubmeshAsset>& out_submeshes);

        void flush(std::vector<SubmeshAsset>& out_submeshes);

        std::unordered_map<FullVertex, uint16> vertex_to_index;
        SubmeshAsset sub;

    private:
        struct AddVertexResult
        {
            uint32 index;
            bool success;
        };

        AddVertexResult add_vertex(const std::vector<vec3>& positions, const std::vector<vec2>& uvs, const std::vector<vec3>& normals,
            const std::vector<vec3>& tangents, const std::vector<uint32>& colors, const std::vector<animation::SkinningWeights>& skins,
            int32 index);
    };

    ERA_CORE_API void generate_normals_and_tangents(std::vector<SubmeshAsset>& submeshes, uint32 flags);
    ERA_CORE_API void generate_normals_and_tangents(ref<SubmeshAsset> submesh, uint32 flags);
}