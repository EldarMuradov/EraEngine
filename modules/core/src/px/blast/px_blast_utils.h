#pragma once

#include "px/core/px_physics_engine.h"

#if PX_BLAST_ENABLE

#include "px/core/px_extensions.h"

#include "scene/scene.h"

namespace era_engine::physics
{
    struct chunkPair
    {
        chunkPair(uint32 c1, uint32 c2) : chunk1((entity_handle)c1), chunk2((entity_handle)c2) {}
        chunkPair(entity_handle c1, entity_handle c2) : chunk1(c1), chunk2(c2) {}

        entity_handle chunk1{};
        entity_handle chunk2{};
    };

    inline bool operator==(const chunkPair& lhs, const chunkPair& rhs)
    {
        return (rhs.chunk1 == lhs.chunk1 || rhs.chunk1 == lhs.chunk2) && (rhs.chunk2 == lhs.chunk2 || rhs.chunk2 == lhs.chunk1);
    }

    struct int_pair_hash
    {
        template <class T1, class T2>
        std::size_t operator () (const std::pair<T1, T2>& p) const
        {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ h2;
        }
    };
}

namespace std
{
    template<>
    struct hash<era_engine::physics::chunkPair>
    {
        size_t operator()(const era_engine::physics::chunkPair& pair) const
        {
            size_t seed = 0;

            hash_combine(seed, (uint32)pair.chunk1);
            hash_combine(seed, (uint32)pair.chunk2);

            return seed;
        }
    };
}

namespace era_engine
{
    struct submesh_asset;
}

namespace Nv::Blast
{
    struct Triangle;
}

namespace era_engine::physics
{
    using namespace physx;

    struct nvmesh;

    bounds getCompositeMeshBounds(eentity& entt);

    ref<submesh_asset> createRenderMesh(const nvmesh& simpleMesh);

    void solveOpenEdges(std::vector<uint32>& indices);

    std::vector<uint32> generateIndices(Nv::Blast::Triangle* triangles, size_t nbTriangles);

    enum class anchor : uint8
    {
        anchor_none = 0,
        anchor_left = 1,
        anchor_right = 2,
        anchor_bottom = 4,
        anchor_top = 8,
        anchor_front = 16,
        anchor_back = 32
    };

    inline anchor operator|(anchor lhs, anchor rhs)
    {
        return static_cast<anchor>(static_cast<char>(lhs) | static_cast<char>(rhs));
    }

    inline bool operator&(anchor lhs, anchor rhs)
    {
        return (static_cast<char>(lhs) & static_cast<char>(rhs));
    }
}

#endif