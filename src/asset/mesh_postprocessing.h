// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"
#include "core/hash.h"
#include "geometry/mesh.h"
#include "asset/model_asset.h"

namespace era_engine
{
	struct full_vertex
	{
		vec3 position;
		vec2 uv;
		vec3 normal;
		vec3 tangent;
		uint32 color;
		animation::skinning_weights skin;
	};

	static bool operator==(const full_vertex& a, const full_vertex& b)
	{
		return memcmp(&a, &b, sizeof(full_vertex)) == 0;
	}

}

namespace std
{
	template<>
	struct hash<era_engine::full_vertex>
	{
		size_t operator()(const era_engine::full_vertex& x) const
		{
			size_t seed = 0;

			hash_combine(seed, x.position);
			hash_combine(seed, x.uv);
			hash_combine(seed, x.normal);
			hash_combine(seed, x.tangent);
			hash_combine(seed, x.color);
			hash_combine(seed, *(uint32*)x.skin.skinIndices);
			hash_combine(seed, *(uint32*)x.skin.skinWeights);

			return seed;
		}
	};
}

namespace era_engine
{
	struct per_material
	{
		void addTriangles(const std::vector<vec3>& positions, const std::vector<vec2>& uvs, const std::vector<vec3>& normals,
			const std::vector<vec3>& tangents, const std::vector<uint32>& colors, const std::vector<animation::skinning_weights>& skins,
			int32 firstIndex, int32 faceSize, std::vector<submesh_asset>& outSubmeshes);

		void flush(std::vector<submesh_asset>& outSubmeshes);

		std::unordered_map<full_vertex, uint16> vertexToIndex;
		submesh_asset sub;

	private:
		struct add_vertex_result
		{
			uint16 index;
			bool success;
		};

		add_vertex_result addVertex(const std::vector<vec3>& positions, const std::vector<vec2>& uvs, const std::vector<vec3>& normals,
			const std::vector<vec3>& tangents, const std::vector<uint32>& colors, const std::vector<animation::skinning_weights>& skins,
			int32 index);
	};

	void generateNormalsAndTangents(std::vector<submesh_asset>& submeshes, uint32 flags);
	void generateNormalsAndTangents(ref<submesh_asset> submesh, uint32 flags);
}