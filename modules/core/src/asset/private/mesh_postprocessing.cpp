// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "asset/mesh_postprocessing.h"

#include "core/cpu_profiling.h"

namespace era_engine
{
	void generate_normals_and_tangents(std::vector<SubmeshAsset>& submeshes, uint32 flags)
	{
		if (flags & MESH_FLAG_GEN_TANGENTS)
		{
			flags |= MESH_FLAG_GEN_NORNALS;
		}

		for (SubmeshAsset& sub : submeshes)
		{
			if (sub.normals.empty() && flags & MESH_FLAG_GEN_NORNALS)
			{
				printf("Generating normals\n");
				CPU_PRINT_PROFILE_BLOCK("Generating normals");

				sub.normals.resize(sub.positions.size(), vec3(0.f));
				for (IndexedTriangle32 tri : sub.triangles)
				{
					vec3 a = sub.positions[tri.a];
					vec3 b = sub.positions[tri.b];
					vec3 c = sub.positions[tri.c];

					vec3 n = cross(b - a, c - a);
					sub.normals[tri.a] += n;
					sub.normals[tri.b] += n;
					sub.normals[tri.c] += n;
				}
				for (vec3& n : sub.normals)
				{
					n = normalize(n);
				}
			}

			if (sub.tangents.empty() && flags & MESH_FLAG_GEN_TANGENTS)
			{
				printf("Generating tangents\n");
				CPU_PRINT_PROFILE_BLOCK("Generating tangents");

				sub.tangents.resize(sub.positions.size(), vec3(0.f));
				if (!sub.uvs.empty())
				{
					for (IndexedTriangle32 tri : sub.triangles)
					{
						vec3 a = sub.positions[tri.a];
						vec3 b = sub.positions[tri.b];
						vec3 c = sub.positions[tri.c];

						vec2 h = sub.uvs[tri.a];
						vec2 k = sub.uvs[tri.b];
						vec2 l = sub.uvs[tri.c];

						vec3 d = b - a;
						vec3 e = c - a;

						vec2 f = k - h;
						vec2 g = l - h;

						float inv_det = 1.f / (f.x * g.y - f.y * g.x);

						vec3 t;
						t.x = g.y * d.x - f.y * e.x;
						t.y = g.y * d.y - f.y * e.y;
						t.z = g.y * d.z - f.y * e.z;
						t *= inv_det;
						sub.tangents[tri.a] += t;
						sub.tangents[tri.b] += t;
						sub.tangents[tri.c] += t;
					}
					for (uint32 i = 0; i < (uint32)sub.positions.size(); ++i)
					{
						vec3 t = sub.tangents[i];
						vec3 n = sub.normals[i];

						vec3 b = cross(t, n);
						t = cross(n, b);

						sub.tangents[i] = normalize(t);
					}
				}
				else
				{
					printf("Mesh has no UVs. Generating suboptimal tangents.\n");
					for (uint32 i = 0; i < (uint32)sub.positions.size(); ++i)
					{
						sub.tangents[i] = get_tangent(sub.normals[i]);
					}
				}
			}
		}
	}

	void generate_normals_and_tangents(ref<SubmeshAsset> submesh, uint32 flags)
	{
		if (flags & MESH_FLAG_GEN_TANGENTS)
		{
			flags |= MESH_FLAG_GEN_NORNALS;
		}

		if (submesh->normals.empty() && flags & MESH_FLAG_GEN_NORNALS)
		{
			printf("Generating normals\n");
			CPU_PRINT_PROFILE_BLOCK("Generating normals");

			submesh->normals.resize(submesh->positions.size(), vec3(0.f));
			for (IndexedTriangle32 tri : submesh->triangles)
			{
				vec3 a = submesh->positions[tri.a];
				vec3 b = submesh->positions[tri.b];
				vec3 c = submesh->positions[tri.c];

				vec3 n = cross(b - a, c - a);
				submesh->normals[tri.a] += n;
				submesh->normals[tri.b] += n;
				submesh->normals[tri.c] += n;
			}
			for (vec3& n : submesh->normals)
			{
				n = normalize(n);
			}
		}

		if (submesh->tangents.empty() && flags & MESH_FLAG_GEN_TANGENTS)
		{
			printf("Generating tangents\n");
			CPU_PRINT_PROFILE_BLOCK("Generating tangents");

			submesh->tangents.resize(submesh->positions.size(), vec3(0.f));
			if (!submesh->uvs.empty())
			{
				for (IndexedTriangle32 tri : submesh->triangles)
				{
					vec3 a = submesh->positions[tri.a];
					vec3 b = submesh->positions[tri.b];
					vec3 c = submesh->positions[tri.c];

					vec2 h = submesh->uvs[tri.a];
					vec2 k = submesh->uvs[tri.b];
					vec2 l = submesh->uvs[tri.c];

					vec3 d = b - a;
					vec3 e = c - a;

					vec2 f = k - h;
					vec2 g = l - h;

					float inv_det = 1.f / (f.x * g.y - f.y * g.x);

					vec3 t;
					t.x = g.y * d.x - f.y * e.x;
					t.y = g.y * d.y - f.y * e.y;
					t.z = g.y * d.z - f.y * e.z;
					t *= inv_det;
					submesh->tangents[tri.a] += t;
					submesh->tangents[tri.b] += t;
					submesh->tangents[tri.c] += t;
				}

				for (uint32 i = 0; i < (uint32)submesh->positions.size(); ++i)
				{
					vec3 t = submesh->tangents[i];
					vec3 n = submesh->normals[i];

					vec3 b = cross(t, n);
					t = cross(n, b);

					submesh->tangents[i] = normalize(t);
				}
			}
			else
			{
				printf("Mesh has no UVs. Generating suboptimal tangents.\n");
				for (uint32 i = 0; i < (uint32)submesh->positions.size(); ++i)
				{
					submesh->tangents[i] = get_tangent(submesh->normals[i]);
				}
			}
		}
	}

	void PerMaterial::add_triangles(const std::vector<vec3>& positions, const std::vector<vec2>& uvs, const std::vector<vec3>& normals, const std::vector<vec3>& tangents, const std::vector<uint32>& colors, const std::vector<era_engine::animation::SkinningWeights>& skins, int32 first_index, int32 face_size, std::vector<SubmeshAsset>& out_submeshes)
	{
		if (face_size < 3)
			return;

		int32 a_index = first_index++;
		int32 b_index = first_index++;
		AddVertexResult a = add_vertex(positions, uvs, normals, tangents, colors, skins, a_index);
		AddVertexResult b = add_vertex(positions, uvs, normals, tangents, colors, skins, b_index);
		for (int32 i = 2; i < face_size; ++i)
		{
			int32 c_index = first_index++;
			AddVertexResult c = add_vertex(positions, uvs, normals, tangents, colors, skins, c_index);

			if (!(a.success && b.success && c.success))
			{
				flush(out_submeshes);
				a = add_vertex(positions, uvs, normals, tangents, colors, skins, a_index);
				b = add_vertex(positions, uvs, normals, tangents, colors, skins, b_index);
				c = add_vertex(positions, uvs, normals, tangents, colors, skins, c_index);
				printf("Too many vertices for 16-bit indices. Splitting mesh!\n");
			}

			sub.triangles.push_back(IndexedTriangle32{ a.index, b.index, c.index });

			b = c;
			b_index = c_index;
		}
	}

	void PerMaterial::flush(std::vector<SubmeshAsset>& out_submeshes)
	{
		if (vertex_to_index.size() > 0)
		{
			out_submeshes.push_back(std::move(sub));
			vertex_to_index.clear();
		}
	}

	PerMaterial::AddVertexResult PerMaterial::add_vertex(const std::vector<vec3>& positions, const std::vector<vec2>& uvs, const std::vector<vec3>& normals, const std::vector<vec3>& tangents, const std::vector<uint32>& colors, const std::vector<era_engine::animation::SkinningWeights>& skins, int32 index)
	{
		vec3 position = positions[index];
		vec2 uv = !uvs.empty() ? uvs[index] : vec2(0.f, 0.f);
		vec3 normal = !normals.empty() ? normals[index] : vec3(0.f, 0.f, 0.f);
		vec3 tangent = !tangents.empty() ? tangents[index] : vec3(0.f, 0.f, 0.f);
		uint32 color = !colors.empty() ? colors[index] : 0;
		animation::SkinningWeights skin = !skins.empty() ? skins[index] : animation::SkinningWeights{};

		FullVertex vertex = { position, uv, normal, tangent, color, skin, };
		auto it = vertex_to_index.find(vertex);
		if (it == vertex_to_index.end())
		{
			uint32 vertex_index = (uint32)sub.positions.size();
			if (vertex_index > UINT16_MAX)
			{
				return { 0, false };
			}

			vertex_to_index.insert({ vertex, (uint32)vertex_index });

			sub.positions.push_back(position);
			if (!uvs.empty()) { sub.uvs.push_back(uv); }
			if (!normals.empty()) { sub.normals.push_back(normal); }
			if (!tangents.empty()) { sub.tangents.push_back(tangent); }
			if (!colors.empty()) { sub.colors.push_back(color); }
			if (!skins.empty()) { sub.skin.push_back(skin); }

			return { (uint32)vertex_index, true };
		}
		else
			return { it->second, true };
	}
}