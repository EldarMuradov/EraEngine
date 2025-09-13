#include "physics/destructions/destruction_utils.h"

#include <NvBlastGlobals.h>

namespace era_engine::physics
{
	ref<DestructibleAsset> DestructionUtils::create_box_asset(const vec3& size, uint32 chunks_per_axis, float health)
	{
		ref<DestructibleAsset> blast_asset = make_ref<DestructibleAsset>();

        const vec3 chunk_size = size / static_cast<float>(chunks_per_axis);

        for (uint32 z = 0; z < chunks_per_axis; ++z)
        {
            for (uint32 y = 0; y < chunks_per_axis; ++y)
            {
                for (uint32 x = 0; x < chunks_per_axis; ++x)
                {
                    NvBlastChunkDesc chunk{};
                    chunk.volume = chunk_size.x * chunk_size.y * chunk_size.z;
                    chunk.parentChunkDescIndex = UINT32_MAX;
                    chunk.flags = 0;

                    vec3 position = {
                        x * chunk_size.x - size.x / 2.0f,
                        y * chunk_size.y - size.y / 2.0f,
                        z * chunk_size.z - size.z / 2.0f
                    };

                    memcpy(chunk.centroid, &position, sizeof(vec3));
                    blast_asset->chunks.push_back(chunk);
                }
            }
        }

        blast_asset->chunks[0].flags = NvBlastChunkDesc::SupportFlag;

        create_bonds(blast_asset, chunks_per_axis, chunk_size, health);

        const uint32 chunks_count = static_cast<uint32>(blast_asset->chunks.size());
        const uint32 bonds_count = static_cast<uint32>(blast_asset->bonds.size());

        NvBlastAssetDesc desc{};
        desc.chunkCount = chunks_count;
        desc.chunkDescs = blast_asset->chunks.data();
        desc.bondCount = bonds_count;
        desc.bondDescs = blast_asset->bonds.data();

        std::vector<char> scratch(chunks_count * sizeof(NvBlastChunkDesc));

        bool result = NvBlastEnsureAssetExactSupportCoverage(blast_asset->chunks.data(), chunks_count, scratch.data(), nullptr);
        ASSERT(result);

        std::vector<uint32> map(chunks_count);
        result = NvBlastReorderAssetDescChunks(blast_asset->chunks.data(), chunks_count, blast_asset->bonds.data(), bonds_count, map.data(), true, scratch.data(), nullptr);
        ASSERT(result);

        scratch.resize(NvBlastGetRequiredScratchForCreateAsset(&desc, nullptr));
        void* mem = malloc(NvBlastGetAssetMemorySize(&desc, nullptr));
        blast_asset->blast_asset = NvBlastCreateAsset(mem, &desc, scratch.data(), nullptr);

		return blast_asset;
	}

    void DestructionUtils::create_bonds(
        ref<DestructibleAsset> asset,
        uint32 chunks_per_axis,
        const vec3& chunk_size,
        float health)
    {
        for (uint32 z = 0; z < chunks_per_axis; ++z) 
        {
            for (uint32 y = 0; y < chunks_per_axis; ++y) 
            {
                for (uint32 x = 0; x < chunks_per_axis; ++x) 
                {
                    uint32 currentIndex = get_index(x, y, z, chunks_per_axis);

                    if (x < chunks_per_axis - 1) 
                    {
                        uint32 right_index = get_index(x + 1, y, z, chunks_per_axis);
                        create_bond(asset, currentIndex, right_index, chunk_size, health);
                    }

                    if (y < chunks_per_axis - 1) 
                    {
                        uint32 up_index = get_index(x, y + 1, z, chunks_per_axis);
                        create_bond(asset, currentIndex, up_index, chunk_size, health);
                    }

                    if (z < chunks_per_axis - 1) 
                    {
                        uint32 back_index = get_index(x, y, z + 1, chunks_per_axis);
                        create_bond(asset, currentIndex, back_index, chunk_size, health);
                    }
                }
            }
        }
    }

    void DestructionUtils::create_bond(
        ref<DestructibleAsset> asset,
        uint32 chunk_index_a,
        uint32 chunk_index_b,
        const vec3& chunk_size,
        float health) 
    {
        NvBlastBondDesc bond{};
        bond.chunkIndices[0] = chunk_index_a;
        bond.chunkIndices[1] = chunk_index_b;

        bond.bond.area = chunk_size.y * chunk_size.z;

        vec3 center = (get_chunk_center(asset->chunks[chunk_index_a]) +
            get_chunk_center(asset->chunks[chunk_index_b])) * 0.5f;
        memcpy(bond.bond.centroid, &center, sizeof(vec3));

        vec3 normal = normalize(
            get_chunk_center(asset->chunks[chunk_index_b]) -
            get_chunk_center(asset->chunks[chunk_index_a])
        );
        memcpy(bond.bond.normal, &normal, sizeof(vec3));

        asset->bonds.push_back(bond);
    }

    uint32 DestructionUtils::get_index(uint32 x, uint32 y, uint32 z, uint32 chunks_per_axis)
    {
        return x + y * chunks_per_axis + z * chunks_per_axis * chunks_per_axis;
    }

    vec3 DestructionUtils::get_chunk_center(const NvBlastChunkDesc& chunk)
    {
        vec3 center;
        memcpy(&center, chunk.centroid, sizeof(vec3));
        return center;
    }
}