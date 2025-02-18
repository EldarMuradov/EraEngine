// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "asset/bin.h"

#include "asset/model_asset.h"
#include "asset/io.h"

#include "core/cpu_profiling.h"

#include "rendering/pbr_material.h"

//#define PROFILE(name) CPU_PRINT_PROFILE_BLOCK(name)
#define PROFILE(name) 

namespace era_engine
{
	static const uint32 BIN_HEADER = 'BIN ';

	struct bin_header
	{
		uint32 header = BIN_HEADER;
		uint32 version = 1;
		uint32 flags;
		uint32 numMeshes;
		uint32 numMaterials;
		uint32 numSkeletons;
		uint32 numAnimations;
	};

	struct bin_mesh_header
	{
		uint32 numSubmeshes;
		int32 skeletonIndex;
		uint32 nameLength;
	};

	enum bin_submesh_flag
	{
		bin_submesh_flag_positions = (1 << 0),
		bin_submesh_flag_uvs = (1 << 1),
		bin_submesh_flag_normals = (1 << 2),
		bin_submesh_flag_tangents = (1 << 3),
		bin_submesh_flag_colors = (1 << 4),
		bin_submesh_flag_skin = (1 << 5),
	};

	struct bin_submesh_header
	{
		uint32 numVertices;
		uint32 numTriangles;
		int32 materialIndex;
		uint32 flags;
	};

	struct bin_material_header
	{
		uint32 albedoPathLength;
		uint32 normalPathLength;
		uint32 roughnessPathLength;
		uint32 metallicPathLength;
	};

	struct bin_skeleton_header
	{
		uint32 numJoints;
	};

	struct bin_animation_header
	{
		float duration;
		uint32 numJoints;
		uint32 numPositionKeyframes;
		uint32 numRotationKeyframes;
		uint32 numScaleKeyframes;

		uint32 nameLength;
	};

	template <typename T>
	static void writeArray(const std::vector<T>& in, FILE* file)
	{
		fwrite(in.data(), sizeof(T), in.size(), file);
	}

	static void writeMesh(const MeshAsset& mesh, FILE* file)
	{
		bin_mesh_header header;
		header.skeletonIndex = mesh.skeleton_index;
		header.numSubmeshes = (uint32)mesh.submeshes.size();
		header.nameLength = (uint32)mesh.name.length();

		fwrite(&header, sizeof(header), 1, file);
		fwrite(mesh.name.c_str(), sizeof(char), header.nameLength, file);

		for (uint32 i = 0; i < header.numSubmeshes; ++i)
		{
			const SubmeshAsset& in = mesh.submeshes[i];

			bin_submesh_header subHeader;
			subHeader.materialIndex = in.material_index;
			subHeader.numVertices = (uint32)in.positions.size();
			subHeader.numTriangles = (uint32)in.triangles.size();

			subHeader.flags = 0;
			if (!in.positions.empty()) { subHeader.flags |= bin_submesh_flag_positions; }
			if (!in.uvs.empty()) { subHeader.flags |= bin_submesh_flag_uvs; }
			if (!in.normals.empty()) { subHeader.flags |= bin_submesh_flag_normals; }
			if (!in.tangents.empty()) { subHeader.flags |= bin_submesh_flag_tangents; }
			if (!in.colors.empty()) { subHeader.flags |= bin_submesh_flag_colors; }
			if (!in.skin.empty()) { subHeader.flags |= bin_submesh_flag_skin; }

			fwrite(&subHeader, sizeof(bin_submesh_header), 1, file);

			if (!in.positions.empty()) { writeArray(in.positions, file); }
			if (!in.uvs.empty()) { writeArray(in.uvs, file); }
			if (!in.normals.empty()) { writeArray(in.normals, file); }
			if (!in.tangents.empty()) { writeArray(in.tangents, file); }
			if (!in.colors.empty()) { writeArray(in.colors, file); }
			if (!in.skin.empty()) { writeArray(in.skin, file); }
			writeArray(in.triangles, file);
		}
	}

	static void writeMaterial(const PbrMaterialDesc& material, FILE* file)
	{
		std::string albedo = material.albedo.string();
		std::string normal = material.normal.string();
		std::string roughness = material.roughness.string();
		std::string metallic = material.metallic.string();

		bin_material_header header;
		header.albedoPathLength = (uint32)albedo.length();
		header.normalPathLength = (uint32)normal.length();
		header.roughnessPathLength = (uint32)roughness.length();
		header.metallicPathLength = (uint32)metallic.length();

		fwrite(&header, sizeof(header), 1, file);
		fwrite(albedo.c_str(), sizeof(char), header.albedoPathLength, file);
		fwrite(normal.c_str(), sizeof(char), header.normalPathLength, file);
		fwrite(roughness.c_str(), sizeof(char), header.roughnessPathLength, file);
		fwrite(metallic.c_str(), sizeof(char), header.metallicPathLength, file);

		fwrite(&material.albedo_flags, sizeof(uint32), 1, file);
		fwrite(&material.normal_flags, sizeof(uint32), 1, file);
		fwrite(&material.roughness_flags, sizeof(uint32), 1, file);
		fwrite(&material.metallic_flags, sizeof(uint32), 1, file);

		fwrite(&material.emission, sizeof(vec4), 1, file);
		fwrite(&material.albedo_tint, sizeof(vec4), 1, file);
		fwrite(&material.roughness_override, sizeof(float), 1, file);
		fwrite(&material.metallic_override, sizeof(float), 1, file);
		fwrite(&material.shader, sizeof(PbrMaterialShader), 1, file);
		fwrite(&material.uv_scale, sizeof(float), 1, file);
		fwrite(&material.translucency, sizeof(float), 1, file);
	}

	static void writeSkeleton(const SkeletonAsset& skeleton, FILE* file)
	{
		bin_skeleton_header header;
		header.numJoints = (uint32)skeleton.joints.size();

		fwrite(&header, sizeof(header), 1, file);

		for (uint32 i = 0; i < header.numJoints; ++i)
		{
			uint32 nameLength = (uint32)skeleton.joints[i].name.length();
			fwrite(&nameLength, sizeof(uint32), 1, file);
			fwrite(skeleton.joints[i].name.c_str(), sizeof(char), nameLength, file);
			fwrite(&skeleton.joints[i].limb_type, sizeof(era_engine::animation::LimbType), 1, file);
			fwrite(&skeleton.joints[i].ik, sizeof(bool), 1, file);
			fwrite(&skeleton.joints[i].inv_bind_transform, sizeof(mat4), 1, file);
			fwrite(&skeleton.joints[i].bind_transform, sizeof(mat4), 1, file);
			fwrite(&skeleton.joints[i].parent_id, sizeof(uint32), 1, file);
		}
	}

	static void writeAnimation(const AnimationAsset& animation, FILE* file)
	{
		bin_animation_header header;
		header.duration = animation.duration;
		header.numJoints = (uint32)animation.joints.size();
		header.numPositionKeyframes = (uint32)animation.position_keyframes.size();
		header.numRotationKeyframes = (uint32)animation.rotation_keyframes.size();
		header.numScaleKeyframes = (uint32)animation.scale_keyframes.size();
		header.nameLength = (uint32)animation.name.length();

		fwrite(&header, sizeof(header), 1, file);
		fwrite(animation.name.c_str(), sizeof(char), header.nameLength, file);

		for (auto& [name, joint] : animation.joints)
		{
			uint32 nameLength = (uint32)name.length();
			fwrite(&nameLength, sizeof(uint32), 1, file);
			fwrite(name.c_str(), sizeof(char), nameLength, file);

			fwrite(&joint, sizeof(animation::AnimationJoint), 1, file);
		}

		writeArray(animation.position_timestamps, file);
		writeArray(animation.rotation_timestamps, file);
		writeArray(animation.scale_timestamps, file);
		writeArray(animation.position_keyframes, file);
		writeArray(animation.rotation_keyframes, file);
		writeArray(animation.scale_keyframes, file);
	}

	void writeBIN(const ModelAsset& asset, const fs::path& path)
	{
		FILE* file = fopen(path.string().c_str(), "wb");

		bin_header header;
		header.flags = asset.flags;
		header.numMeshes = (uint32)asset.meshes.size();
		header.numMaterials = (uint32)asset.materials.size();
		header.numSkeletons = (uint32)asset.skeletons.size();
		header.numAnimations = (uint32)asset.animations.size();

		fwrite(&header, sizeof(header), 1, file);

		for (uint32 i = 0; i < header.numMeshes; ++i)
		{
			writeMesh(asset.meshes[i], file);
		}
		for (uint32 i = 0; i < header.numMaterials; ++i)
		{
			writeMaterial(asset.materials[i], file);
		}
		for (uint32 i = 0; i < header.numSkeletons; ++i)
		{
			writeSkeleton(asset.skeletons[i], file);
		}
		for (uint32 i = 0; i < header.numAnimations; ++i)
		{
			writeAnimation(asset.animations[i], file);
		}

		fclose(file);
	}

	template <typename T>
	static void readArray(EntireFile& file, std::vector<T>& out, uint32 count)
	{
		out.resize(count);
		T* ptr = file.consume<T>(count);
		memcpy(out.data(), ptr, sizeof(T) * count);
	}

	static MeshAsset readMesh(EntireFile& file)
	{
		bin_mesh_header* header = file.consume<bin_mesh_header>();
		char* name = file.consume<char>(header->nameLength);

		MeshAsset result;
		result.name = std::string(name, header->nameLength);
		result.submeshes.resize(header->numSubmeshes);
		result.skeleton_index = header->skeletonIndex;

		for (uint32 i = 0; i < header->numSubmeshes; ++i)
		{
			bin_submesh_header* subHeader = file.consume<bin_submesh_header>();

			SubmeshAsset& sub = result.submeshes[i];
			sub.material_index = subHeader->materialIndex;
			if (subHeader->flags & bin_submesh_flag_positions) { readArray(file, sub.positions, subHeader->numVertices); }
			if (subHeader->flags & bin_submesh_flag_uvs) { readArray(file, sub.uvs, subHeader->numVertices); }
			if (subHeader->flags & bin_submesh_flag_normals) { readArray(file, sub.normals, subHeader->numVertices); }
			if (subHeader->flags & bin_submesh_flag_tangents) { readArray(file, sub.tangents, subHeader->numVertices); }
			if (subHeader->flags & bin_submesh_flag_colors) { readArray(file, sub.colors, subHeader->numVertices); }
			if (subHeader->flags & bin_submesh_flag_skin) { readArray(file, sub.skin, subHeader->numVertices); }
			readArray(file, sub.triangles, subHeader->numTriangles);
		}

		return result;
	}

	static PbrMaterialDesc readMaterial(EntireFile& file)
	{
		bin_material_header* header = file.consume<bin_material_header>();

		char* albedo = file.consume<char>(header->albedoPathLength);
		char* normal = file.consume<char>(header->normalPathLength);
		char* roughness = file.consume<char>(header->roughnessPathLength);
		char* metallic = file.consume<char>(header->metallicPathLength);

		PbrMaterialDesc result;
		result.albedo = std::string(albedo, header->albedoPathLength);
		result.normal = std::string(normal, header->normalPathLength);
		result.roughness = std::string(roughness, header->roughnessPathLength);
		result.metallic = std::string(metallic, header->metallicPathLength);

		result.albedo_flags = *file.consume<uint32>();
		result.normal_flags = *file.consume<uint32>();
		result.roughness_flags = *file.consume<uint32>();
		result.metallic_flags = *file.consume<uint32>();

		result.emission = *file.consume<vec4>();
		result.albedo_tint = *file.consume<vec4>();
		result.roughness_override = *file.consume<float>();
		result.metallic_override = *file.consume<float>();
		result.shader = *file.consume<PbrMaterialShader>();
		result.uv_scale = *file.consume<float>();
		result.translucency = *file.consume<float>();

		return result;
	}

	static SkeletonAsset readSkeleton(EntireFile& file)
	{
		bin_skeleton_header* header = file.consume<bin_skeleton_header>();

		SkeletonAsset result;
		result.joints.resize(header->numJoints);
		result.name_to_joint_id.reserve(header->numJoints);

		for (uint32 i = 0; i < header->numJoints; ++i)
		{
			uint32 nameLength = *file.consume<uint32>();
			char* name = file.consume<char>(nameLength);

			result.joints[i].name = std::string(name, nameLength);
			result.joints[i].limb_type = *file.consume<animation::LimbType>();
			result.joints[i].ik = *file.consume<bool>();
			result.joints[i].inv_bind_transform = *file.consume<mat4>();
			result.joints[i].bind_transform = *file.consume<mat4>();
			result.joints[i].parent_id = *file.consume<uint32>();

			result.name_to_joint_id[result.joints[i].name] = i;
		}

		return result;
	}

	static AnimationAsset readAnimation(EntireFile& file)
	{
		bin_animation_header* header = file.consume<bin_animation_header>();

		AnimationAsset result;
		result.duration = header->duration;

		char* name = file.consume<char>(header->nameLength);
		result.name = std::string(name, header->nameLength);

		result.joints.reserve(header->numJoints);

		for (uint32 i = 0; i < header->numJoints; ++i)
		{
			uint32 nameLength = *file.consume<uint32>();
			char* name = file.consume<char>(nameLength);

			animation::AnimationJoint joint = *file.consume<animation::AnimationJoint>();
			result.joints[std::string(name, nameLength)] = joint;
		}

		readArray(file, result.position_timestamps, header->numPositionKeyframes);
		readArray(file, result.rotation_timestamps, header->numRotationKeyframes);
		readArray(file, result.scale_timestamps, header->numScaleKeyframes);
		readArray(file, result.position_keyframes, header->numPositionKeyframes);
		readArray(file, result.rotation_keyframes, header->numRotationKeyframes);
		readArray(file, result.scale_keyframes, header->numScaleKeyframes);

		return result;
	}

	ModelAsset loadBIN(const fs::path& path)
	{
		PROFILE("Loading BIN");

		EntireFile file = load_file(path);

		bin_header* header = file.consume<bin_header>();
		if (header->header != BIN_HEADER)
		{
			free_file(file);
			return {};
		}

		ModelAsset result;
		result.meshes.resize(header->numMeshes);
		result.materials.resize(header->numMaterials);
		result.skeletons.resize(header->numSkeletons);
		result.animations.resize(header->numAnimations);

		for (uint32 i = 0; i < header->numMeshes; ++i)
		{
			result.meshes[i] = readMesh(file);
		}
		for (uint32 i = 0; i < header->numMaterials; ++i)
		{
			result.materials[i] = readMaterial(file);
		}
		for (uint32 i = 0; i < header->numSkeletons; ++i)
		{
			result.skeletons[i] = readSkeleton(file);
		}
		for (uint32 i = 0; i < header->numAnimations; ++i)
		{
			result.animations[i] = readAnimation(file);
		}

		free_file(file);

		return result;
	}
}