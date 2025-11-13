// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"

#include "animation/animation.h"

#include "geometry/mesh.h"

namespace era_engine
{
	struct ERA_CORE_API SkeletonAsset
	{
		std::vector<animation::SkeletonJoint> joints;
		std::unordered_map<std::string, uint32> name_to_joint_id;
	};

	struct ERA_CORE_API AnimationAsset
	{
		std::string name;
		float duration = 0.0f;

		std::unordered_map<std::string, animation::AnimationJoint> joints;

		std::vector<float> position_timestamps;
		std::vector<float> rotation_timestamps;
		std::vector<float> scale_timestamps;

		std::vector<vec3> position_keyframes;
		std::vector<quat> rotation_keyframes;
		std::vector<vec3> scale_keyframes;
	};

	struct ERA_CORE_API SubmeshAsset
	{
		int32 material_index = 0;

		std::vector<vec3> positions;
		std::vector<vec2> uvs;
		std::vector<vec3> normals;
		std::vector<vec3> tangents;
		std::vector<uint32> colors;
		std::vector<animation::SkinningWeights> skin;

		std::vector<indexed_triangle16> triangles;
	};

	struct ERA_CORE_API MeshAsset
	{
		std::string name;
		std::vector<SubmeshAsset> submeshes;
		int32 skeleton_index = 0;
	};

	struct PbrMaterialDesc;

	struct ERA_CORE_API ModelAsset
	{
		uint32 flags = 0;
		std::vector<MeshAsset> meshes;
		std::vector<PbrMaterialDesc> materials;
		std::vector<SkeletonAsset> skeletons;
		std::vector<AnimationAsset> animations;
	};

	enum MeshFlags
	{
		mesh_flag_load_uvs = (1 << 0),
		mesh_flag_flip_uvs_vertically = (1 << 1),
		mesh_flag_load_normals = (1 << 2),
		mesh_flag_load_tangents = (1 << 3),
		mesh_flag_gen_normals = (1 << 4), // Only if mesh has no normals.
		mesh_flag_gen_tangents = (1 << 5), // Only if mesh has no tangents.
		mesh_flag_load_colors = (1 << 6), // Only if mesh has no tangents.
		mesh_flag_load_skin = (1 << 7),

		mesh_flag_default = mesh_flag_load_uvs | mesh_flag_flip_uvs_vertically |
		mesh_flag_load_normals | mesh_flag_gen_normals |
		mesh_flag_load_tangents | mesh_flag_gen_tangents |
		mesh_flag_load_colors | mesh_flag_load_skin,
	};

	ERA_CORE_API ModelAsset load_3d_model_from_file(const fs::path& path, uint32 mesh_flags = mesh_flag_default);

	inline bool is_mesh_extension(const fs::path& extension)
	{
		return extension == ".fbx" || extension == ".obj" || extension == ".bin";
	}

	inline bool is_mesh_extension(const std::string& extension)
	{
		return extension == ".fbx" || extension == ".obj" || extension == ".bin";
	}
}