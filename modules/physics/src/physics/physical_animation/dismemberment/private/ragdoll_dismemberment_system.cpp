#include "physics/physical_animation/dismemberment/ragdoll_dismemberment_system.h"
#include "physics/core/physics.h"
#include "physics/core/physics_utils.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_utils.h"
#include "physics/collisions_holder_root_component.h"
#include "physics/shape_component.h"
#include "physics/physical_animation/ragdoll_profile.h"

#include <core/cpu_profiling.h>
#include <core/debug/debug_var.h>
#include <core/log.h>
#include <core/traits.h>

#include <rendering/debug_visualization.h>
#include <rendering/ecs/renderer_holder_root_component.h>

#include <ecs/update_groups.h>
#include <ecs/rendering/mesh_component.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<RagdollDismembermentSystem>("RagdollDismembermentSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))

			.method("update_ragdolls", &RagdollDismembermentSystem::update_ragdolls)
			(metadata("update_group", update_types::PHYSICS),
			 metadata("After", std::vector<std::string>{"PhysicalAnimationSystem::update_after_physics"}));
	}

	static void seal_submesh_holes(
			SubmeshAsset& submesh,
			const std::vector<uint32>& group_bones)
	{
		if (submesh.triangles.empty())
		{
			return;
		}

		struct EdgeKey
		{
			uint32 a, b;

			bool operator==(const EdgeKey& o) const
			{
				return (a == o.a && b == o.b) || (a == o.b && b == o.a);
			}
		};

		struct EdgeHash
		{
			size_t operator()(const EdgeKey& k) const
			{
				uint32 lo = k.a < k.b ? k.a : k.b;
				uint32 hi = k.a < k.b ? k.b : k.a;
				return std::hash<uint64>()(((uint64)lo << 32) | hi);
			}
		};

		std::unordered_map<EdgeKey, uint32, EdgeHash> edge_count;

		for (const auto& tri : submesh.triangles)
		{
			edge_count[{tri.a, tri.b}]++;
			edge_count[{tri.b, tri.c}]++;
			edge_count[{tri.c, tri.a}]++;
		}

		std::vector<std::pair<uint32, uint32>> boundary;

		for (const auto& [edge, count] : edge_count)
		{
			if (count == 1)
			{
				boundary.push_back({ edge.a, edge.b });
			}
		}

		if (boundary.empty())
		{
			return;
		}

		std::unordered_map<uint32, std::vector<uint32>> graph;

		for (const auto& e : boundary)
		{
			graph[e.first].push_back(e.second);
			graph[e.second].push_back(e.first);
		}

		while (!graph.empty())
		{
			uint32 start = graph.begin()->first;
			uint32 current = start;

			std::vector<uint32> loop;
			loop.push_back(current);

			std::unordered_set<uint64> visited_edges;

			while (true)
			{
				bool found = false;

				auto it = graph.find(current);

				if (it == graph.end())
				{
					break;
				}

				for (uint32 next : it->second)
				{
					uint32 lo = current < next ? current : next;
					uint32 hi = current < next ? next : current;
					uint64 key = ((uint64)lo << 32) | hi;

					if (visited_edges.count(key))
					{
						continue;
					}

					visited_edges.insert(key);
					loop.push_back(next);
					current = next;
					found = true;
					break;
				}

				if (!found || current == start)
				{
					break;
				}
			}

			if (!loop.empty() && loop.back() == start)
			{
				loop.pop_back();
			}

			for (size_t i = 0; i < loop.size(); ++i)
			{
				uint32 a = loop[i];
				uint32 b = loop[(i + 1) % loop.size()];

				auto itA = graph.find(a);

				if (itA != graph.end())
				{
					auto& vec = itA->second;

					for (auto vit = vec.begin(); vit != vec.end(); )
					{
						if (*vit == b)
						{
							vit = vec.erase(vit);
						}
						else
						{
							++vit;
						}
					}

					if (vec.empty())
					{
						graph.erase(itA);
					}
				}
			}

			if (loop.size() < 3)
			{
				continue;
			}

			vec3 center(0);

			for (uint32 v : loop)
			{
				center += submesh.positions[v];
			}

			center /= (float)loop.size();

			uint32 center_idx = (uint32)submesh.positions.size();

			submesh.positions.push_back(center);

			if (!submesh.normals.empty())
			{
				submesh.normals.push_back(vec3(0));
			}

			if (!submesh.uvs.empty())
			{
				vec2 uv_center(0);

				for (uint32 v : loop)
				{
					uv_center += submesh.uvs[v];
				}

				uv_center /= (float)loop.size();

				submesh.uvs.push_back(uv_center);
			}

			if (!submesh.tangents.empty())
			{
				submesh.tangents.push_back(vec3(0));
			}

			if (!submesh.colors.empty())
			{
				submesh.colors.push_back(submesh.colors[loop[0]]);
			}

			if (!submesh.skin.empty())
			{
				std::unordered_map<uint32, uint32> bone_weights;

				for (uint32 v : loop)
				{
					const auto& skin = submesh.skin[v];

					for (int i = 0; i < 4; ++i)
					{
						if (skin.skin_weights[i] == 0)
						{
							continue;
						}

						if (!group_bones.empty())
						{
							bool in_group = false;

							for (uint32 b : group_bones)
							{
								if (b == skin.skin_indices[i])
								{
									in_group = true;
									break;
								}
							}

							if (!in_group)
							{
								continue;
							}
						}

						bone_weights[skin.skin_indices[i]] += skin.skin_weights[i];
					}
				}

				animation::SkinningWeights center_skin{};

				std::vector<std::pair<uint32, uint32>> sorted(std::make_move_iterator(bone_weights.begin()), std::make_move_iterator(bone_weights.end()));
				std::sort(sorted.begin(), sorted.end(),
					[](const auto& a, const auto& b) { return a.second > b.second; });

				uint32 total = 0;

				for (uint32 i = 0; i < 4 && i < sorted.size(); ++i)
				{
					center_skin.skin_indices[i] = (uint8)sorted[i].first;
					total += sorted[i].second;
				}

				if (total > 0)
				{
					for (uint32 i = 0; i < 4 && i < sorted.size(); ++i)
					{
						center_skin.skin_weights[i] = (uint8)((uint64)sorted[i].second * 255 / total);
					}
				}

				submesh.skin.push_back(center_skin);
			}

			for (size_t i = 0; i < loop.size(); ++i)
			{
				size_t j = (i + 1) % loop.size();

				IndexedTriangle32 tri;
				tri.a = center_idx;
				tri.b = loop[i];
				tri.c = loop[j];

				submesh.triangles.push_back(tri);
			}

			vec3 cap_normal(0);

			for (size_t i = 0; i < loop.size(); ++i)
			{
				size_t j = (i + 1) % loop.size();

				vec3 e0 = submesh.positions[loop[i]] - center;
				vec3 e1 = submesh.positions[loop[j]] - center;

				cap_normal += cross(e0, e1);
			}

			if (length(cap_normal) > 0.0001f)
			{
				cap_normal = normalize(cap_normal);
			}

			if (!submesh.normals.empty())
			{
				submesh.normals[center_idx] = cap_normal;
			}
		}
	}

	static uint32 find_dominant_bone(const animation::SkinningWeights& skin)
	{
		uint32 best_bone = 0;
		uint8 best_weight = 0;

		for (int i = 0; i < 4; ++i)
		{
			if (skin.skin_weights[i] > best_weight)
			{
				best_weight = skin.skin_weights[i];
				best_bone = skin.skin_indices[i];
			}
		}

		return best_bone;
	}

	static void build_part_submesh_filtered(
		const SubmeshAsset& source,
		const std::vector<IndexedTriangle32>& triangles,
		const std::vector<uint32>& group_bones,
		SubmeshAsset& out_mesh)
	{
		out_mesh.material_index = source.material_index;

		std::unordered_map<uint32, uint32> vertex_remap;
		std::vector<uint32> unique_vertices;
		unique_vertices.reserve(triangles.size() * 3);

		for (const auto& tri : triangles)
		{
			for (uint32 v : {tri.a, tri.b, tri.c})
			{
				if (vertex_remap.find(v) == vertex_remap.end())
				{
					vertex_remap[v] = (uint32)unique_vertices.size();
					unique_vertices.push_back(v);
				}
			}
		}

		out_mesh.positions.reserve(unique_vertices.size());

		if (!source.normals.empty())
		{
			out_mesh.normals.reserve(unique_vertices.size());
		}
		if (!source.uvs.empty())
		{
			out_mesh.uvs.reserve(unique_vertices.size());
		}
		if (!source.tangents.empty())
		{
			out_mesh.tangents.reserve(unique_vertices.size());
		}
		if (!source.colors.empty())
		{
			out_mesh.colors.reserve(unique_vertices.size());
		}
		if (!source.skin.empty())
		{
			out_mesh.skin.reserve(unique_vertices.size());
		}

		for (uint32 old_idx : unique_vertices)
		{
			out_mesh.positions.push_back(source.positions[old_idx]);

			if (!source.normals.empty())
			{
				out_mesh.normals.push_back(source.normals[old_idx]);
			}
			if (!source.uvs.empty())
			{
				out_mesh.uvs.push_back(source.uvs[old_idx]);
			}
			if (!source.tangents.empty())
			{
				out_mesh.tangents.push_back(source.tangents[old_idx]);
			}
			if (!source.colors.empty())
			{
				out_mesh.colors.push_back(source.colors[old_idx]);
			}

			if (!source.skin.empty())
			{
				animation::SkinningWeights skin = source.skin[old_idx];

				for (int i = 0; i < 4; ++i)
				{
					if (skin.skin_weights[i] == 0)
					{
						continue;
					}

					bool in_group = false;

					for (uint32 b : group_bones)
					{
						if (b == skin.skin_indices[i])
						{
							in_group = true;
							break;
						}
					}

					if (!in_group)
					{
						skin.skin_indices[i] = 0;
						skin.skin_weights[i] = 0;
					}
				}

				bool any_weight = false;

				for (int i = 0; i < 4; ++i)
				{
					if (skin.skin_weights[i] > 0)
					{
						any_weight = true;
						break;
					}
				}

				if (!any_weight && !group_bones.empty())
				{
					skin.skin_indices[0] = (uint8)group_bones[0];
					skin.skin_weights[0] = 255;
					skin.skin_indices[1] = 0;
					skin.skin_weights[1] = 0;
					skin.skin_indices[2] = 0;
					skin.skin_weights[2] = 0;
					skin.skin_indices[3] = 0;
					skin.skin_weights[3] = 0;
				}

				out_mesh.skin.push_back(skin);
			}
		}

		out_mesh.triangles.reserve(triangles.size());

		for (const auto& tri : triangles)
		{
			IndexedTriangle32 new_tri;
			new_tri.a = vertex_remap[tri.a];
			new_tri.b = vertex_remap[tri.b];
			new_tri.c = vertex_remap[tri.c];
			out_mesh.triangles.push_back(new_tri);
		}
	}

	static SubmeshAsset build_merged_live_submesh(const DismembermentState& state)
	{
		SubmeshAsset merged;
		merged.material_index = 0;

		std::unordered_set<uint32> detached_bones;

		for (uint32 g = 0; g < state.parts.size(); ++g)
		{
			if (!state.detached[g])
			{
				continue;
			}
			for (uint32 b : state.parts[g].bone_ids)
			{
				detached_bones.insert(b);
			}
		}

		std::vector<uint32> live_bones;

		for (uint32 g = 0; g < state.parts.size(); ++g)
		{
			if (state.detached[g])
			{
				continue;
			}
			for (uint32 b : state.parts[g].bone_ids)
			{
				live_bones.push_back(b);
			}
		}

		for (uint32 si = 0; si < state.source_submeshes.size(); ++si)
		{
			const SubmeshAsset& src = state.source_submeshes[si];

			std::vector<bool> vertex_detached(src.positions.size(), false);

			if (!src.skin.empty())
			{
				for (uint32 v = 0; v < src.skin.size(); ++v)
				{
					const auto& skin = src.skin[v];

					for (int i = 0; i < 4; ++i)
					{
						if (skin.skin_weights[i] == 0)
						{
							continue;
						}

						if (detached_bones.count(skin.skin_indices[i]))
						{
							vertex_detached[v] = true;
							break;
						}
					}
				}
			}

			std::vector<bool> vertex_near_detached = vertex_detached;

			for (const auto& tri : src.triangles)
			{
				if (vertex_detached[tri.a] || vertex_detached[tri.b] || vertex_detached[tri.c])
				{
					vertex_near_detached[tri.a] = true;
					vertex_near_detached[tri.b] = true;
					vertex_near_detached[tri.c] = true;
				}
			}

			std::vector<IndexedTriangle32> live_tris;
			live_tris.reserve(src.triangles.size());

			for (const auto& tri : src.triangles)
			{
				uint32 near_count = 0;
				if (vertex_near_detached[tri.a]) near_count++;
				if (vertex_near_detached[tri.b]) near_count++;
				if (vertex_near_detached[tri.c]) near_count++;

				if (near_count >= 2)
				{
					continue;
				}

				live_tris.push_back(tri);
			}

			if (live_tris.empty())
			{
				continue;
			}

			SubmeshAsset piece;
			build_part_submesh_filtered(src, live_tris, live_bones, piece);

			uint32 base = (uint32)merged.positions.size();

			merged.positions.reserve(merged.positions.size() + base);
			merged.positions.insert(merged.positions.end(), std::make_move_iterator(piece.positions.begin()), std::make_move_iterator(piece.positions.end()));

			merged.normals.reserve(merged.normals.size() + piece.normals.size());
			merged.normals.insert(merged.normals.end(), std::make_move_iterator(piece.normals.begin()), std::make_move_iterator(piece.normals.end()));

			merged.uvs.reserve(merged.uvs.size() + piece.uvs.size());
			merged.uvs.insert(merged.uvs.end(), std::make_move_iterator(piece.uvs.begin()), std::make_move_iterator(piece.uvs.end()));

			merged.tangents.reserve(merged.tangents.size() + piece.tangents.size());
			merged.tangents.insert(merged.tangents.end(), std::make_move_iterator(piece.tangents.begin()), std::make_move_iterator(piece.tangents.end()));

			merged.colors.reserve(merged.colors.size() + piece.colors.size());
			merged.colors.insert(merged.colors.end(), std::make_move_iterator(piece.colors.begin()), std::make_move_iterator(piece.colors.end()));

			merged.skin.reserve(merged.skin.size() + piece.skin.size());
			merged.skin.insert(merged.skin.end(), std::make_move_iterator(piece.skin.begin()), std::make_move_iterator(piece.skin.end()));

			merged.triangles.reserve(merged.triangles.size() + piece.triangles.size());
			for (auto& tri : piece.triangles)
			{
				merged.triangles.push_back({ tri.a + base, tri.b + base, tri.c + base });
			}
		}

		return merged;
	}

	static SubmeshAsset build_detached_submesh(
		const DismembermentState& state,
		uint32 part_index)
	{
		SubmeshAsset result;
		result.material_index = 0;

		const std::vector<uint32>& group_bones = state.parts[part_index].bone_ids;

		for (const auto& split : state.splits)
		{
			const auto& tris = split.group_triangles[part_index];

			if (tris.empty())
			{
				continue;
			}

			const SubmeshAsset& src = state.source_submeshes[split.source_submesh_index];

			SubmeshAsset piece;
			build_part_submesh_filtered(src, tris, group_bones, piece);

			uint32 base = (uint32)result.positions.size();

			result.positions.reserve(result.positions.size() + base);
			result.positions.insert(result.positions.end(), std::make_move_iterator(piece.positions.begin()), std::make_move_iterator(piece.positions.end()));

			result.normals.reserve(result.normals.size() + piece.normals.size());
			result.normals.insert(result.normals.end(), std::make_move_iterator(piece.normals.begin()), std::make_move_iterator(piece.normals.end()));

			result.uvs.reserve(result.uvs.size() + piece.uvs.size());
			result.uvs.insert(result.uvs.end(), std::make_move_iterator(piece.uvs.begin()), std::make_move_iterator(piece.uvs.end()));

			result.tangents.reserve(result.tangents.size() + piece.tangents.size());
			result.tangents.insert(result.tangents.end(), std::make_move_iterator(piece.tangents.begin()), std::make_move_iterator(piece.tangents.end()));

			result.colors.reserve(result.colors.size() + piece.colors.size());
			result.colors.insert(result.colors.end(), std::make_move_iterator(piece.colors.begin()), std::make_move_iterator(piece.colors.end()));

			result.skin.reserve(result.skin.size() + piece.skin.size());
			result.skin.insert(result.skin.end(), std::make_move_iterator(piece.skin.begin()), std::make_move_iterator(piece.skin.end()));

			result.triangles.reserve(result.triangles.size() + piece.triangles.size());
			for (auto& tri : piece.triangles)
			{
				result.triangles.push_back({ tri.a + base, tri.b + base, tri.c + base });
			}
		}

		return result;
	}

	static void split_multimesh_into_parts(
		const ModelAsset& source_model,
		const MultiMesh& source_multimesh,
		const std::vector<BoneGroup>& groups,
		DismembermentState& out_state)
	{
		out_state.parts.clear();
		out_state.parts.resize(groups.size());
		out_state.splits.clear();
		out_state.detached.clear();
		out_state.detached.resize(groups.size(), false);
		out_state.source_submeshes.clear();

		for (uint32 g = 0; g < groups.size(); ++g)
		{
			out_state.parts[g].name = groups[g].name;
			out_state.parts[g].bone_ids = groups[g].bone_indices;
		}

		std::unordered_map<uint32, uint32> bone_to_group;

		for (uint32 g = 0; g < groups.size(); ++g)
		{
			for (uint32 bone_id : groups[g].bone_indices)
			{
				bone_to_group[bone_id] = g;
			}
		}

		uint32 global_submesh_index = 0;

		for (uint32 mesh_idx = 0; mesh_idx < source_model.meshes.size(); ++mesh_idx)
		{
			const auto& mesh = source_model.meshes[mesh_idx];

			for (uint32 submesh_idx = 0; submesh_idx < mesh.submeshes.size(); ++submesh_idx)
			{
				const auto& src_submesh = mesh.submeshes[submesh_idx];

				out_state.source_submeshes.push_back(src_submesh);

				SubmeshPartSplit split;
				split.source_submesh_index = global_submesh_index;
				split.group_triangles.resize(groups.size());

				std::vector<uint32> vertex_group(src_submesh.positions.size(), UINT32_MAX);

				if (!src_submesh.skin.empty())
				{
					for (uint32 v = 0; v < src_submesh.skin.size(); ++v)
					{
						uint32 dominant = find_dominant_bone(src_submesh.skin[v]);

						auto it = bone_to_group.find(dominant);

						if (it != bone_to_group.end())
						{
							vertex_group[v] = it->second;
						}
					}
				}

				for (const auto& tri : src_submesh.triangles)
				{
					uint32 g0 = vertex_group[tri.a];
					uint32 g1 = vertex_group[tri.b];
					uint32 g2 = vertex_group[tri.c];

					std::vector<uint32> tri_groups;

					auto add_group = [&](uint32 g)
						{
							if (g == UINT32_MAX)
							{
								return;
							}

							for (uint32 existing : tri_groups)
							{
								if (existing == g)
								{
									return;
								}
							}

							tri_groups.push_back(g);
						};

					add_group(g0);
					add_group(g1);
					add_group(g2);

					for (uint32 g : tri_groups)
					{
						split.group_triangles[g].push_back(tri);
					}
				}

				out_state.splits.push_back(std::move(split));

				global_submesh_index++;
			}
		}

		out_state.initialized = true;
	}

	static uint32 find_part_index_by_limb(
		Entity limb,
		const DismembermentState& state)
	{
		PhysicalAnimationLimbComponent* p_limb = limb.get_component<PhysicalAnimationLimbComponent>();

		if (p_limb == nullptr)
		{
			return UINT32_MAX;
		}

		for (uint32 g = 0; g < state.parts.size(); ++g)
		{
			for (uint32 bone_id : state.parts[g].bone_ids)
			{
				if (bone_id == p_limb->joint_id)
				{
					return g;
				}
			}
		}

		return UINT32_MAX;
	}

	RagdollDismembermentSystem::RagdollDismembermentSystem(World* _world)
		: System(_world)
	{
	}

	void RagdollDismembermentSystem::init()
	{
		using namespace animation;
		entt::registry& registry = world->get_registry();
		registry.on_construct<RagdollDismembermentComponent>().connect<&RagdollDismembermentSystem::on_rdc_created>(this);

		ragdolls_group = world->group(components_group<TransformComponent, PhysicalAnimationComponent, RagdollDismembermentComponent, SkeletonComponent>);

		collisions_holder_rc = world->add_root_component<CollisionsHolderRootComponent>();
		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();

		{
			dismemberment_profile = make_ref<RagdollDismembermentProfile>();

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::HEAD;
				details.type = DismemberableLimbType::ROOT;
				details.max_health = 1.0f;
			}

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::NECK;
				details.type = DismemberableLimbType::DISMEMBERABLE;
				details.max_health = 2.0f;
			}

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::BODY_UPPER;
				details.type = DismemberableLimbType::ROOT;
				details.max_health = 1.0f;
			}

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::BODY_MIDDLE;
				details.type = DismemberableLimbType::ROOT;
				details.max_health = 1.0f;
			}

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::BODY_LOWER;
				details.type = DismemberableLimbType::ROOT;
				details.max_health = 1.0f;
			}

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::CLAVICLE;
				details.type = DismemberableLimbType::ROOT;
				details.max_health = 1.0f;
			}

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::ARM;
				details.type = DismemberableLimbType::DISMEMBERABLE;
				details.max_health = 3.0f;
			}

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::FOREARM;
				details.type = DismemberableLimbType::DISMEMBERABLE;
				details.max_health = 2.0f;
			}

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::HAND;
				details.type = DismemberableLimbType::ROOT;
				details.max_health = 1.0f;
			}

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::LEG;
				details.type = DismemberableLimbType::DISMEMBERABLE;
				details.max_health = 3.0f;
			}

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::CALF;
				details.type = DismemberableLimbType::DISMEMBERABLE;
				details.max_health = 2.0f;
			}

			{
				DismemberableLimbDetails& details = dismemberment_profile->limbs_details.emplace_back();
				details.limb_type = RagdollLimbType::FOOT;
				details.type = DismemberableLimbType::ROOT;
				details.max_health = 1.0f;
			}
		}
	}

	void RagdollDismembermentSystem::apply_dismemberment_result(
		Entity entity,
		PhysicalAnimationComponent& pac,
		RagdollDismembermentComponent& rdc)
	{
		DismembermentTaskData* data = rdc.pending_task;

		if (data == nullptr)
		{
			return;
		}

		MeshComponent* mesh_component = entity.get_component_if_exists<MeshComponent>();

		if (mesh_component == nullptr || mesh_component->mesh == nullptr)
		{
			return;
		}

		MultiMesh& multimesh = *mesh_component->mesh;

		ModelAsset rebuilt;
		MeshAsset mesh;

		if (!data->result.live.positions.empty())
		{
			data->result.live.material_index = 0;
			mesh.submeshes.push_back(std::move(data->result.live));
		}

		for (auto& part : data->result.detached)
		{
			if (part.positions.empty())
			{
				continue;
			}

			part.material_index = 0;
			mesh.submeshes.push_back(std::move(part));
		}

		if (!mesh.submeshes.empty())
		{
			rebuilt.meshes.push_back(std::move(mesh));

			if (multimesh.model_asset.has_value())
			{
				rebuilt.materials = multimesh.model_asset->materials;
			}

			multimesh.update_mesh(rebuilt);
			multimesh.load_state.store(AssetLoadState::LOADED);
		}

		for (EntityPtr limb_ptr : pac.limbs)
		{
			Entity limb = limb_ptr.get();

			RagdollDismembermentLimbComponent* d_limb = limb.get_component<RagdollDismembermentLimbComponent>();
			PhysicalAnimationLimbComponent* p_limb = limb.get_component<PhysicalAnimationLimbComponent>();

			if (d_limb == nullptr || p_limb == nullptr)
			{
				continue;
			}

			if (d_limb->state != DismemberState::PENDING)
			{
				continue;
			}

			d_limb->state = DismemberState::DISMEMBERED;
			p_limb->force_switch_state(PhysicalLimbStateType::DISMEMBERED);

			if (rdc.on_dismember != nullptr)
			{
				rdc.on_dismember(d_limb);
			}

			const PhysicsLimbChain* chain = pac.get_chain_by_joint_id(p_limb->joint_id);

			if (chain == nullptr)
			{
				continue;
			}

			bool found = false;

			for (EntityPtr chain_limb_ptr : chain->connected_limbs)
			{
				Entity chain_limb = chain_limb_ptr.get();

				if (chain_limb == limb)
				{
					found = true;
					continue;
				}

				if (!found)
				{
					continue;
				}

				RagdollDismembermentLimbComponent* next_d = chain_limb.get_component<RagdollDismembermentLimbComponent>();

				if (next_d == nullptr || next_d->state == DismemberState::DISMEMBERED || next_d->state == DismemberState::PENDING)
				{
					continue;
				}

				next_d->state = DismemberState::PARTIAL_DISMEMBERED;

				PhysicalAnimationLimbComponent* next_p = chain_limb.get_component<PhysicalAnimationLimbComponent>();

				if (next_p != nullptr)
				{
					next_p->force_switch_state(PhysicalLimbStateType::PARTIAL_DISMEMBERED);
				}
			}
		}
	}

	void RagdollDismembermentSystem::start_dismemberment_task(
		Entity entity,
		RagdollDismembermentComponent& rdc)
	{
		DismembermentState& state = rdc.dismemberment_state;

		if (!state.initialized)
		{
			return;
		}

		DismembermentTaskData* data = new DismembermentTaskData();

		data->source_submeshes = state.source_submeshes;
		data->splits = state.splits;
		data->parts = state.parts;
		data->detached = state.detached;

		DefaultJob job_object;

		job_object.data.user_data = data;
		job_object.data.data_size = sizeof(DismembermentTaskData);
		job_object.data.destroy_after_execution = false;

		job_object.job_func = [](DefaultJob::Data& default_data)
			{
				DismembermentTaskData* data = reinterpret_cast<DismembermentTaskData*>(default_data.user_data);

				std::vector<uint32> live_bones;

				for (uint32 g = 0; g < data->parts.size(); ++g)
				{
					if (data->detached[g])
					{
						continue;
					}

					for (uint32 b : data->parts[g].bone_ids)
					{
						live_bones.push_back(b);
					}
				}

				DismembermentState temp_state;
				temp_state.source_submeshes = data->source_submeshes;
				temp_state.splits = data->splits;
				temp_state.parts = data->parts;
				temp_state.detached = data->detached;

				SubmeshAsset live = build_merged_live_submesh(temp_state);

				if (!live.positions.empty())
				{
					seal_submesh_holes(live, live_bones);
				}

				data->result.live = std::move(live);

				for (uint32 g = 0; g < data->parts.size(); ++g)
				{
					if (!data->detached[g])
					{
						continue;
					}

					SubmeshAsset part = build_detached_submesh(temp_state, g);

					if (!part.positions.empty())
					{
						seal_submesh_holes(part, data->parts[g].bone_ids);
					}

					data->result.detached.push_back(std::move(part));
					data->result.detached_indices.push_back(g);
				}

				data->completed.store(true, std::memory_order_release);
			};

		JobHandle job = schedule_low_priority_job(job_object);
		job.submit_now();

		rdc.pending_job = job;
		rdc.pending_task = data;

		rdc.pending_detached_parts.clear();
	}

	void RagdollDismembermentSystem::update_ragdolls(float dt)
	{
		ZoneScopedN("RagdollDismembermentSystem::update_ragdolls");

		process_added_rdcs();

		for (auto&& [entity_handle, transform_component, physical_animation_component, dismemberment_component, skeleton_component] : ragdolls_group.each())
		{
			if (!physical_animation_component.loaded || !dismemberment_component.loaded)
			{
				continue;
			}

			Entity entity = world->get_entity(entity_handle);

			if (dismemberment_component.pending_task != nullptr &&
				dismemberment_component.pending_task->completed.load(std::memory_order_acquire))
			{
				dismemberment_component.pending_job.wait_for_completion();

				apply_dismemberment_result(
					entity,
					physical_animation_component,
					dismemberment_component
				);

				delete dismemberment_component.pending_task;
				dismemberment_component.pending_task = nullptr;
			}

			for (EntityPtr limb_ptr : physical_animation_component.limbs)
			{
				Entity limb = limb_ptr.get();

				RagdollDismembermentLimbComponent* d_limb = limb.get_component<RagdollDismembermentLimbComponent>();
				PhysicalAnimationLimbComponent* p_limb = limb.get_component<PhysicalAnimationLimbComponent>();

				if (d_limb == nullptr || p_limb == nullptr)
				{
					continue;
				}

				if (dismemberment_component.affected_by_collisions && p_limb->collision.is_colliding)
				{
					d_limb->apply_damage(1.0f);
				}

				if (d_limb->is_pending())
				{
					uint32 part_index = find_part_index_by_limb(limb, dismemberment_component.dismemberment_state);

					if (part_index != UINT32_MAX &&
						!dismemberment_component.dismemberment_state.detached[part_index])
					{
						dismemberment_component.dismemberment_state.detached[part_index] = true;
						dismemberment_component.pending_detached_parts.push_back(part_index);

						d_limb->state = DismemberState::PENDING;
					}
				}
			}

			if (!dismemberment_component.pending_detached_parts.empty() &&
				dismemberment_component.pending_task == nullptr)
			{
				start_dismemberment_task(entity, dismemberment_component);
			}
		}
	}

	void RagdollDismembermentSystem::rebuild_multimesh(
		MultiMesh& multimesh,
		const DismembermentState& state)
	{
		ModelAsset rebuilt;
		MeshAsset mesh;

		std::vector<uint32> live_bones;

		for (uint32 g = 0; g < state.parts.size(); ++g)
		{
			if (state.detached[g])
			{
				continue;
			}
			for (uint32 b : state.parts[g].bone_ids)
			{
				live_bones.push_back(b);
			}
		}

		SubmeshAsset live = build_merged_live_submesh(state);

		if (!live.positions.empty())
		{
			seal_submesh_holes(live, live_bones);
			live.material_index = 0;
			mesh.submeshes.push_back(std::move(live));
		}

		for (uint32 g = 0; g < state.parts.size(); ++g)
		{
			if (!state.detached[g])
			{
				continue;
			}

			SubmeshAsset part = build_detached_submesh(state, g);

			if (!part.positions.empty())
			{
				seal_submesh_holes(part, state.parts[g].bone_ids);
				part.material_index = 0;
				mesh.submeshes.push_back(std::move(part));
			}
		}

		if (mesh.submeshes.empty())
		{
			return;
		}

		rebuilt.meshes.push_back(std::move(mesh));

		if (multimesh.model_asset.has_value())
		{
			rebuilt.materials = multimesh.model_asset->materials;
		}

		multimesh.update_mesh(rebuilt);
	}

	void RagdollDismembermentSystem::process_dismemberment(
		Entity entity,
		RagdollDismembermentComponent& rdc,
		const std::vector<Entity>& dismembered_limbs)
	{
		MeshComponent* mesh_comp = entity.get_component_if_exists<MeshComponent>();

		if (mesh_comp == nullptr || mesh_comp->mesh == nullptr)
		{
			return;
		}

		DismembermentState& state = rdc.dismemberment_state;

		if (!state.initialized)
		{
			return;
		}

		bool any = false;
		for (Entity limb : dismembered_limbs)
		{
			uint32 part_index = find_part_index_by_limb(limb, state);

			if (part_index == UINT32_MAX || state.detached[part_index])
			{
				continue;
			}

			state.detached[part_index] = true;
			any = true;
		}

		if (any)
		{
			rebuild_multimesh(*mesh_comp->mesh, state);
		}
	}

	void RagdollDismembermentSystem::process_added_rdcs()
	{
		using namespace animation;

		ScopedSpinLock _lock{ sync };

		for (auto iter = rdcs_to_init.begin(); iter != rdcs_to_init.end();)
		{
			Entity::Handle entity_handle = *iter;
			Entity entity = world->get_entity(entity_handle);

			RagdollDismembermentComponent* dismemberment_component = entity.get_component_if_exists<RagdollDismembermentComponent>();
			if (dismemberment_component == nullptr)
			{
				iter = rdcs_to_init.erase(iter);
				continue;
			}

			if (dismemberment_component->dismemberment_profile == nullptr)
			{
				dismemberment_component->dismemberment_profile = dismemberment_profile;
			}

			if (dismemberment_component->loaded)
			{
				iter = rdcs_to_init.erase(iter);
				continue;
			}

			const PhysicalAnimationComponent* physical_animation_component = entity.get_component_if_exists<PhysicalAnimationComponent>();
			if (physical_animation_component == nullptr || !physical_animation_component->loaded)
			{
				++iter;
				continue;
			}

			const SkeletonComponent* skeleton_component = entity.get_component_if_exists<SkeletonComponent>();
			if (skeleton_component == nullptr || skeleton_component->skeleton == nullptr)
			{
				++iter;
				continue;
			}

			const MeshComponent* mesh_component = entity.get_component_if_exists<MeshComponent>();
			if (mesh_component == nullptr || mesh_component->mesh == nullptr)
			{
				++iter;
				continue;
			}

			const ModelAsset& source_model = mesh_component->mesh->model_asset.value();
			if (source_model.meshes.empty())
			{
				++iter;
				continue;
			}

			const ref<RagdollDismembermentProfile>& profile = dismemberment_component->dismemberment_profile;
			const Skeleton& skeleton = *skeleton_component->skeleton;

			bool is_loaded = true;

			for (EntityPtr limb_ptr : physical_animation_component->limbs)
			{
				Entity limb = limb_ptr.get();

				PhysicalAnimationLimbComponent* limb_component = limb.get_component_if_exists<PhysicalAnimationLimbComponent>();
				if (limb_component == nullptr)
				{
					continue;
				}

				const DismemberableLimbDetails* limb_details = profile->get_details_by_limb_type(limb_component->type);
				if (limb_details == nullptr)
				{
					is_loaded = false;
					ASSERT(limb_details != nullptr);
					break;
				}

				if (limb.has_component<RagdollDismembermentLimbComponent>())
				{
					continue;
				}

				RagdollDismembermentLimbComponent* d_limb = limb.add_component<RagdollDismembermentLimbComponent>();
				d_limb->ragdoll_ptr = entity;
				d_limb->can_be_dismembered = limb_details->type == DismemberableLimbType::DISMEMBERABLE;
				d_limb->current_health = limb_details->max_health;
			}

			if (!is_loaded)
			{
				++iter;
				continue;
			}

			std::vector<BoneGroup> groups;

			auto process_chain = [&](const PhysicsLimbChain* chain, const std::string& prefix, bool exclude_assigned)
				{
					if (chain == nullptr)
					{
						return;
					}

					std::vector<uint32> limb_joints;

					for (EntityPtr limb_ptr : chain->connected_limbs)
					{
						Entity limb = limb_ptr.get();

						const PhysicalAnimationLimbComponent* limb_component = limb.get_component_if_exists<PhysicalAnimationLimbComponent>();
						if (limb_component == nullptr)
						{
							continue;
						}

						limb_joints.push_back(limb_component->joint_id);
					}

					for (size_t i = 0; i < limb_joints.size(); ++i)
					{
						uint32 start = limb_joints[i];

						BoneGroup group;
						group.name = prefix + "_" + std::to_string(i);

						std::vector<uint32> subtree;
						subtree.push_back(start);

						for (uint32 k = 0; k < subtree.size(); ++k)
						{
							uint32 current = subtree[k];

							for (uint32 j = 0; j < skeleton.joints.size(); ++j)
							{
								if (skeleton.joints[j].parent_id == current)
								{
									subtree.push_back(j);
								}
							}
						}

						for (uint32 bone : subtree)
						{
							bool belongs_to_later_limb = false;

							for (size_t j = i + 1; j < limb_joints.size(); ++j)
							{
								uint32 later_root = limb_joints[j];

								if (later_root == bone)
								{
									belongs_to_later_limb = true;
									break;
								}

								uint32 walk = bone;
								while (skeleton.joints[walk].parent_id != INVALID_JOINT)
								{
									walk = skeleton.joints[walk].parent_id;
									if (walk == later_root)
									{
										belongs_to_later_limb = true;
										break;
									}
								}

								if (belongs_to_later_limb)
								{
									break;
								}
							}

							if (belongs_to_later_limb)
							{
								continue;
							}

							if (exclude_assigned)
							{
								bool already_assigned = false;

								for (const auto& other : groups)
								{
									for (uint32 assigned : other.bone_indices)
									{
										if (assigned == bone)
										{
											already_assigned = true;
											break;
										}
									}

									if (already_assigned)
									{
										break;
									}
								}

								if (already_assigned)
								{
									continue;
								}
							}

							group.bone_indices.push_back(bone);
						}

						if (!group.bone_indices.empty())
						{
							groups.push_back(std::move(group));
						}
					}
				};

			process_chain(physical_animation_component->neck_chain.get(), "neck", false);
			process_chain(physical_animation_component->left_arm_chain.get(), "left_arm", false);
			process_chain(physical_animation_component->right_arm_chain.get(), "right_arm", false);
			process_chain(physical_animation_component->left_leg_chain.get(), "left_leg", false);
			process_chain(physical_animation_component->right_leg_chain.get(), "right_leg", false);
			process_chain(physical_animation_component->body_chain.get(), "body", true);

			{
				std::unordered_set<uint32> assigned;

				for (const auto& g : groups)
				{
					for (uint32 b : g.bone_indices)
					{
						assigned.insert(b);
					}
				}

				BoneGroup* body_group = nullptr;

				for (auto& g : groups)
				{
					if (g.name == "body")
					{
						body_group = &g;
						break;
					}
				}

				if (body_group == nullptr)
				{
					groups.push_back(BoneGroup{ "body", {} });
					body_group = &groups.back();
				}

				for (uint32 i = 0; i < skeleton.joints.size(); ++i)
				{
					if (!assigned.count(i))
					{
						body_group->bone_indices.push_back(i);
					}
				}
			}

			split_multimesh_into_parts(source_model, *mesh_component->mesh, groups, dismemberment_component->dismemberment_state);

			dismemberment_component->loaded = true;
			iter = rdcs_to_init.erase(iter);
		}
	}

	void RagdollDismembermentSystem::on_rdc_created(entt::registry& registry, entt::entity entity_handle)
	{
		ScopedSpinLock _lock{ sync };

		rdcs_to_init.push_back(static_cast<Entity::Handle>(entity_handle));
	}
}
