#include "asset/model_asset.h"
#include "asset/asset.h"
#include "asset/io.h"

#include "animation/skeleton.h"
#include "animation/skinning.h"
#include "animation/animation_clip_utils.h"
#include "animation/animation.h"

#include "core/log.h"

#include <material.hlsli>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace era_engine
{
	static void load_materials(const aiScene* scene, ModelAsset& result)
	{
		result.materials.reserve(scene->mNumMaterials);

		for (uint32 i = 0; i < scene->mNumMaterials; ++i)
		{
			aiMaterial* material = scene->mMaterials[i];
			PbrMaterialDesc imported;

			aiColor4D albedo;
			if (material->Get(AI_MATKEY_COLOR_DIFFUSE, albedo) == AI_SUCCESS)
			{
				imported.albedo_tint = vec4(albedo.r, albedo.g, albedo.b, albedo.a);
			}

			float roughness = 0.5f;
			if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
			{
				imported.roughness_override = roughness;
			}
			else
			{
				float shininess = 0.0f;
				if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
				{
					imported.roughness_override = 1.0f - sqrt(shininess / 100.0f);
				}
			}

			float metallic = 0.0f;
			material->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
			imported.metallic_override = metallic;

			aiString texture_path;

			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path) == AI_SUCCESS)
			{
				imported.albedo = texture_path.C_Str();
			}

			if (material->GetTexture(aiTextureType_NORMALS, 0, &texture_path) == AI_SUCCESS)
			{
				imported.normal = texture_path.C_Str();
			}

			if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texture_path) == AI_SUCCESS)
			{
				imported.roughness = texture_path.C_Str();
			}

			if (material->GetTexture(aiTextureType_METALNESS, 0, &texture_path) == AI_SUCCESS)
			{
				imported.metallic = texture_path.C_Str();
			}

			result.materials.push_back(imported);
		}
	}

	static void add_bone_weight(animation::SkinningWeights& skinning, uint32 bone_index, float weight)
	{
		uint8 weight_byte = (uint8)clamp(weight * 255.0f, 0.0f, 255.0f);
		uint8 bone_id = (uint8)bone_index;

		for (int i = 0; i < 4; ++i)
		{
			if (skinning.skin_weights[i] == 0)
			{
				skinning.skin_indices[i] = bone_id;
				skinning.skin_weights[i] = weight_byte;
				return;
			}
		}

		int min_index = 0;
		uint8 min_weight = skinning.skin_weights[0];

		for (int i = 1; i < 4; ++i)
		{
			if (skinning.skin_weights[i] < min_weight)
			{
				min_index = i;
				min_weight = skinning.skin_weights[i];
			}
		}

		if (weight_byte > min_weight)
		{
			skinning.skin_indices[min_index] = bone_id;
			skinning.skin_weights[min_index] = weight_byte;
		}
	}

	static void import_skinning(aiMesh* ai_mesh, SubmeshAsset& submesh, uint32 flags)
	{
		submesh.skin.resize(ai_mesh->mNumVertices);

		for (auto& skin : submesh.skin)
		{
			skin.skin_indices.fill(0);
			skin.skin_weights.fill(0);
		}

		for (uint32 bone_index = 0; bone_index < ai_mesh->mNumBones; ++bone_index)
		{
			aiBone* bone = ai_mesh->mBones[bone_index];

			const uint32 joint_index = flags & MESH_CREATION_FLAGS_GENERATE_ROOT_MOTION ? bone_index + 1 : bone_index;

			for (uint32 weight_index = 0; weight_index < bone->mNumWeights; ++weight_index)
			{
				aiVertexWeight weight = bone->mWeights[weight_index];

				uint32 vertex_id = weight.mVertexId;
				float weight_value = weight.mWeight;

				if (vertex_id >= submesh.skin.size())
				{
					continue;
				}

				add_bone_weight(submesh.skin[vertex_id], joint_index, weight_value);
			}
		}

		for (auto& skin : submesh.skin)
		{
			uint32 weight_sum = skin.skin_weights[0] + skin.skin_weights[1] +
				skin.skin_weights[2] + skin.skin_weights[3];

			if (weight_sum > 0)
			{
				uint32 running_sum = 0;
				for (int i = 0; i < 3; ++i)
				{
					skin.skin_weights[i] = (uint8)((uint32)skin.skin_weights[i] * 255 / weight_sum);
					running_sum += skin.skin_weights[i];
				}
				skin.skin_weights[3] = (uint8)(255 - running_sum);
			}
			else
			{
				skin.skin_indices.fill(0);
				skin.skin_weights.fill(0);
				skin.skin_weights[0] = 255;
			}
		}
	}

	static void load_meshes(const aiScene* scene, float scale_factor, ModelAsset& result)
	{
		result.meshes.reserve(scene->mNumMeshes);

		for (uint32 mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
		{
			aiMesh* ai_mesh = scene->mMeshes[mesh_index];

			MeshAsset mesh;
			mesh.name = ai_mesh->mName.C_Str();
			mesh.skeleton_index = 0;

			SubmeshAsset submesh;
			submesh.material_index = ai_mesh->mMaterialIndex;

			submesh.positions.reserve(ai_mesh->mNumVertices);
			for (uint32 i = 0; i < ai_mesh->mNumVertices; ++i)
			{
				aiVector3D pos = ai_mesh->mVertices[i];

				submesh.positions.push_back(vec3(pos.x, pos.y, pos.z) * scale_factor);
			}

			if (ai_mesh->HasNormals())
			{
				submesh.normals.reserve(ai_mesh->mNumVertices);
				for (uint32 i = 0; i < ai_mesh->mNumVertices; ++i)
				{
					aiVector3D normal = ai_mesh->mNormals[i];
					submesh.normals.push_back(vec3(normal.x, normal.y, normal.z));
				}
			}

			if (ai_mesh->HasTextureCoords(0))
			{
				submesh.uvs.reserve(ai_mesh->mNumVertices);
				for (uint32 i = 0; i < ai_mesh->mNumVertices; ++i)
				{
					aiVector3D uv = ai_mesh->mTextureCoords[0][i];
					submesh.uvs.push_back(vec2(uv.x, uv.y));
				}
			}

			if (ai_mesh->HasTangentsAndBitangents())
			{
				submesh.tangents.reserve(ai_mesh->mNumVertices);
				for (uint32 i = 0; i < ai_mesh->mNumVertices; ++i)
				{
					aiVector3D tangent = ai_mesh->mTangents[i];
					submesh.tangents.push_back(vec3(tangent.x, tangent.y, tangent.z));
				}
			}

			if (ai_mesh->HasVertexColors(0))
			{
				submesh.colors.reserve(ai_mesh->mNumVertices);
				for (uint32 i = 0; i < ai_mesh->mNumVertices; ++i)
				{
					aiColor4D color = ai_mesh->mColors[0][i];
					uint32 packed = pack_color(color.r, color.g, color.b, color.a);
					submesh.colors.push_back(packed);
				}
			}

			if (ai_mesh->HasBones())
			{
				import_skinning(ai_mesh, submesh, result.flags);
			}

			submesh.triangles.reserve(ai_mesh->mNumFaces);

			for (uint32 face_index = 0; face_index < ai_mesh->mNumFaces; ++face_index)
			{
				aiFace face = ai_mesh->mFaces[face_index];

				if (face.mNumIndices == 3)
				{
					IndexedTriangle32 triangle;
					triangle.a = (uint32)face.mIndices[0];
					triangle.b = (uint32)face.mIndices[1];
					triangle.c = (uint32)face.mIndices[2];
					submesh.triangles.push_back(triangle);
				}
				else
				{
					printf("Warning: Face with %d indices (expected 3)\n", face.mNumIndices);
				}
			}

			mesh.submeshes.push_back(submesh);
			result.meshes.push_back(mesh);
		}
	}

	static std::string clean_bone_name(const std::string& name)
	{
		size_t pos = name.find("_$AssimpFbx$");
		if (pos != std::string::npos)
		{
			return name.substr(0, pos);
		}
		return name;
	}

	static void find_skeleton_roots_recursive(aiNode* node,
		const std::unordered_set<std::string>& bone_names,
		std::vector<aiNode*>& roots)
	{
		std::string node_name = clean_bone_name(node->mName.C_Str());

		bool is_bone = bone_names.find(node_name) != bone_names.end();

		if (is_bone)
		{
			bool parent_is_bone = false;

			if (node->mParent)
			{
				std::string parent_name = clean_bone_name(node->mParent->mName.C_Str());
				parent_is_bone = bone_names.find(parent_name) != bone_names.end();
			}

			if (!parent_is_bone)
			{
				roots.push_back(node);
				return;
			}
		}

		for (uint32 i = 0; i < node->mNumChildren; ++i)
		{
			find_skeleton_roots_recursive(node->mChildren[i], bone_names, roots);
		}
	}

	static std::vector<aiNode*> find_skeleton_roots(const aiScene* scene)
	{
		std::vector<aiNode*> roots;

		std::unordered_set<std::string> bone_names;

		for (uint32 mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
		{
			aiMesh* mesh = scene->mMeshes[mesh_index];

			for (uint32 bone_index = 0; bone_index < mesh->mNumBones; ++bone_index)
			{
				aiBone* bone = mesh->mBones[bone_index];
				std::string bone_name = clean_bone_name(bone->mName.C_Str());
				bone_names.insert(bone_name);
			}
		}

		find_skeleton_roots_recursive(scene->mRootNode, bone_names, roots);

		return roots;
	}

	static void collect_bone_names_recursive(aiNode* node, std::unordered_set<std::string>& bone_names)
	{
		std::string node_name = clean_bone_name(node->mName.C_Str());
		bone_names.insert(node_name);

		for (uint32 i = 0; i < node->mNumChildren; ++i)
		{
			collect_bone_names_recursive(node->mChildren[i], bone_names);
		}
	}

	static void collect_skeleton_bones(const aiScene* scene,
		aiNode* skeleton_root,
		std::unordered_map<std::string, aiBone*>& skeleton_bones)
	{
		std::unordered_set<std::string> bone_names;
		collect_bone_names_recursive(skeleton_root, bone_names);

		for (uint32 mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
		{
			aiMesh* mesh = scene->mMeshes[mesh_index];

			for (uint32 bone_index = 0; bone_index < mesh->mNumBones; ++bone_index)
			{
				aiBone* bone = mesh->mBones[bone_index];
				std::string bone_name = clean_bone_name(bone->mName.C_Str());

				if (bone_names.find(bone_name) != bone_names.end())
				{
					if (skeleton_bones.find(bone_name) == skeleton_bones.end())
					{
						skeleton_bones[bone_name] = bone;
					}
				}
			}
		}
	}

	static void convert_assimp_matrix(const aiMatrix4x4& ai_mat, mat4& out_mat)
	{
		out_mat.m[0] = ai_mat.a1;
		out_mat.m[1] = ai_mat.b1;
		out_mat.m[2] = ai_mat.c1;
		out_mat.m[3] = ai_mat.d1;

		out_mat.m[4] = ai_mat.a2;
		out_mat.m[5] = ai_mat.b2;
		out_mat.m[6] = ai_mat.c2;
		out_mat.m[7] = ai_mat.d2;

		out_mat.m[8] = ai_mat.a3;
		out_mat.m[9] = ai_mat.b3;
		out_mat.m[10] = ai_mat.c3;
		out_mat.m[11] = ai_mat.d3;

		out_mat.m[12] = ai_mat.a4;
		out_mat.m[13] = ai_mat.b4;
		out_mat.m[14] = ai_mat.c4;
		out_mat.m[15] = ai_mat.d4;
	}

	static void process_skeleton_node(aiNode* node,
		aiNode* parent_node,
		const std::unordered_map<std::string, aiBone*>& skeleton_bones,
		float scale_factor,
		animation::SkeletonAssetImportData& result)
	{
		using namespace animation;

		std::string node_name = clean_bone_name(node->mName.C_Str());

		auto bone_it = skeleton_bones.find(node_name);

		if (bone_it != skeleton_bones.end())
		{
			if (result.name_to_joint_id.find(node_name) != result.name_to_joint_id.end())
			{
				for (uint32 i = 0; i < node->mNumChildren; ++i)
				{
					process_skeleton_node(node->mChildren[i], node, skeleton_bones, scale_factor, result);
				}
				return;
			}

			aiBone* bone = bone_it->second;

			SkeletonJoint joint;
			joint.name = node_name;

			aiMatrix4x4 offset_matrix = bone->mOffsetMatrix;
			convert_assimp_matrix(offset_matrix, joint.inv_bind_transform);

			trs temp = mat4_to_trs(invert(joint.inv_bind_transform));
			temp.position *= scale_factor;

			joint.bind_transform = trs_to_mat4(temp);
			joint.inv_bind_transform = invert(joint.bind_transform);

			if (parent_node)
			{
				aiNode* current_parent = parent_node;
				while (current_parent)
				{
					std::string current_parent_name = clean_bone_name(current_parent->mName.C_Str());
					auto current_parent_it = result.name_to_joint_id.find(current_parent_name);

					if (current_parent_it != result.name_to_joint_id.end())
					{
						joint.parent_id = current_parent_it->second;
						break;
					}

					current_parent = current_parent->mParent;
				}
			}

			uint32 joint_id = (uint32)result.joints.size();
			result.joints.push_back(joint);
			result.name_to_joint_id[node_name] = joint_id;
		}

		for (uint32 i = 0; i < node->mNumChildren; ++i)
		{
			process_skeleton_node(node->mChildren[i], node, skeleton_bones, scale_factor, result);
		}
	}

	static std::vector<animation::SkeletonAssetImportData> load_skeletons(const aiScene* scene, float scale_factor, uint32 flags)
	{
		using namespace animation;

		std::vector<SkeletonAssetImportData> result;

		std::vector<aiNode*> skeleton_roots = find_skeleton_roots(scene);

		printf("Found %zu skeletons\n", skeleton_roots.size());

		for (size_t i = 0; i < skeleton_roots.size(); ++i)
		{
			SkeletonAssetImportData skeleton_data;

			std::unordered_map<std::string, aiBone*> skeleton_bones;
			collect_skeleton_bones(scene, skeleton_roots[i], skeleton_bones);

			printf("Skeleton %zu: %zu bones\n", i, skeleton_bones.size());

			process_skeleton_node(skeleton_roots[i], nullptr, skeleton_bones, scale_factor, skeleton_data);

			printf("Processed skeleton %zu: %zu joints\n", i, skeleton_data.joints.size());

			for (size_t j = 0; j < skeleton_data.joints.size(); ++j)
			{
				printf("  Joint %zu: %s (parent: %d)\n",
					j,
					skeleton_data.joints[j].name.c_str(),
					skeleton_data.joints[j].parent_id);
			}

			result.push_back(std::move(skeleton_data));
		}

		if (result.empty())
		{
			SkeletonAssetImportData single_skeleton;
			std::unordered_map<std::string, aiBone*> all_bones;

			for (uint32 mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
			{
				aiMesh* mesh = scene->mMeshes[mesh_index];

				for (uint32 bone_index = 0; bone_index < mesh->mNumBones; ++bone_index)
				{
					aiBone* bone = mesh->mBones[bone_index];
					std::string bone_name = clean_bone_name(bone->mName.C_Str());

					if (all_bones.find(bone_name) == all_bones.end())
					{
						all_bones[bone_name] = bone;
					}
				}
			}

			if (!all_bones.empty())
			{
				printf("No skeleton roots found, using all %zu bones\n", all_bones.size());
				process_skeleton_node(scene->mRootNode, nullptr, all_bones, scale_factor, single_skeleton);

				printf("Processed skeleton: %zu joints\n", single_skeleton.joints.size());

				for (size_t j = 0; j < single_skeleton.joints.size(); ++j)
				{
					printf("  Joint %zu: %s (parent: %d)\n",
						j,
						single_skeleton.joints[j].name.c_str(),
						single_skeleton.joints[j].parent_id);
				}

				result.push_back(std::move(single_skeleton));
			}
		}

		return result;
	}

	static std::vector<aiNodeAnim*> find_all_node_anims_for_bone(
		const aiAnimation* animation,
		const std::string& bone_name)
	{
		std::vector<aiNodeAnim*> result;
		std::string clean_bone = clean_bone_name(bone_name);

		for (uint32 i = 0; i < animation->mNumChannels; ++i)
		{
			aiNodeAnim* node_anim = animation->mChannels[i];
			std::string anim_name = node_anim->mNodeName.C_Str();
			std::string clean_anim_name = clean_bone_name(anim_name);

			if (clean_anim_name == clean_bone || anim_name == bone_name)
			{
				result.push_back(node_anim);
			}
		}

		return result;
	}

	static trs sample_node_animation_component(
		const aiNodeAnim* node_anim,
		float time,
		float scale_factor,
		float ticks_per_second)
	{
		trs result = trs::identity;

		if (!node_anim)
			return result;

		if (node_anim->mNumPositionKeys > 0)
		{
			aiVector3D position;

			float first_time = (float)node_anim->mPositionKeys[0].mTime / ticks_per_second;
			float last_time = (float)node_anim->mPositionKeys[node_anim->mNumPositionKeys - 1].mTime / ticks_per_second;

			if (time <= first_time)
			{
				position = node_anim->mPositionKeys[0].mValue;
			}
			else if (time >= last_time)
			{
				position = node_anim->mPositionKeys[node_anim->mNumPositionKeys - 1].mValue;
			}
			else
			{
				for (uint32 i = 0; i < node_anim->mNumPositionKeys - 1; ++i)
				{
					float t0 = (float)node_anim->mPositionKeys[i].mTime / ticks_per_second;
					float t1 = (float)node_anim->mPositionKeys[i + 1].mTime / ticks_per_second;

					if (time >= t0 && time <= t1)
					{
						float alpha = (t1 > t0) ? (time - t0) / (t1 - t0) : 0.0f;

						aiVector3D p0 = node_anim->mPositionKeys[i].mValue;
						aiVector3D p1 = node_anim->mPositionKeys[i + 1].mValue;

						position = p0 + (p1 - p0) * alpha;
						break;
					}
				}
			}

			result.position = vec3(position.x, position.y, position.z) * scale_factor;
		}

		if (node_anim->mNumRotationKeys > 0)
		{
			aiQuaternion rotation;

			float first_time = (float)node_anim->mRotationKeys[0].mTime / ticks_per_second;
			float last_time = (float)node_anim->mRotationKeys[node_anim->mNumRotationKeys - 1].mTime / ticks_per_second;

			if (time <= first_time)
			{
				rotation = node_anim->mRotationKeys[0].mValue;
			}
			else if (time >= last_time)
			{
				rotation = node_anim->mRotationKeys[node_anim->mNumRotationKeys - 1].mValue;
			}
			else
			{
				for (uint32 i = 0; i < node_anim->mNumRotationKeys - 1; ++i)
				{
					float t0 = (float)node_anim->mRotationKeys[i].mTime / ticks_per_second;
					float t1 = (float)node_anim->mRotationKeys[i + 1].mTime / ticks_per_second;

					if (time >= t0 && time <= t1)
					{
						float alpha = (t1 > t0) ? (time - t0) / (t1 - t0) : 0.0f;

						aiQuaternion q0 = node_anim->mRotationKeys[i].mValue;
						aiQuaternion q1 = node_anim->mRotationKeys[i + 1].mValue;

						aiQuaternion::Interpolate(rotation, q0, q1, alpha);
						break;
					}
				}
			}

			result.rotation = quat(rotation.x, rotation.y, rotation.z, rotation.w);
		}

		if (node_anim->mNumScalingKeys > 0)
		{
			aiVector3D scaling;

			float first_time = (float)node_anim->mScalingKeys[0].mTime / ticks_per_second;
			float last_time = (float)node_anim->mScalingKeys[node_anim->mNumScalingKeys - 1].mTime / ticks_per_second;

			if (time <= first_time)
			{
				scaling = node_anim->mScalingKeys[0].mValue;
			}
			else if (time >= last_time)
			{
				scaling = node_anim->mScalingKeys[node_anim->mNumScalingKeys - 1].mValue;
			}
			else
			{
				for (uint32 i = 0; i < node_anim->mNumScalingKeys - 1; ++i)
				{
					float t0 = (float)node_anim->mScalingKeys[i].mTime / ticks_per_second;
					float t1 = (float)node_anim->mScalingKeys[i + 1].mTime / ticks_per_second;

					if (time >= t0 && time <= t1)
					{
						float alpha = (t1 > t0) ? (time - t0) / (t1 - t0) : 0.0f;

						aiVector3D s0 = node_anim->mScalingKeys[i].mValue;
						aiVector3D s1 = node_anim->mScalingKeys[i + 1].mValue;

						scaling = s0 + (s1 - s0) * alpha;
						break;
					}
				}
			}

			result.scale = vec3(scaling.x, scaling.y, scaling.z);
		}

		return result;
	}

	static trs get_local_bind_pose(
		const animation::Skeleton* skeleton,
		uint32 joint_id)
	{
		const auto& joint = skeleton->joints[joint_id];

		trs global_bind = mat4_to_trs(joint.bind_transform);

		if (joint.parent_id == INVALID_JOINT)
		{
			return global_bind;
		}

		const auto& parent_joint = skeleton->joints[joint.parent_id];
		trs parent_global_bind = mat4_to_trs(parent_joint.bind_transform);

		trs parent_inverse = invert(parent_global_bind);

		trs local_bind = parent_inverse * global_bind;

		return local_bind;
	}

	static std::vector<animation::AnimationClipAssetImportData> load_animations(
		const aiScene* scene,
		const animation::Skeleton* skeleton,
		float scale_factor,
		uint32 flags)
	{
		using namespace animation;

		std::vector<AnimationClipAssetImportData> result;

		if (!scene->mNumAnimations)
		{
			return result;
		}

		for (uint32 anim_index = 0; anim_index < scene->mNumAnimations; ++anim_index)
		{
			aiAnimation* animation = scene->mAnimations[anim_index];

			AnimationClipAssetImportData anim_data;
			anim_data.name = animation->mName.C_Str();
			anim_data.duration = (float)animation->mDuration / (float)animation->mTicksPerSecond;

			printf("Processing animation: %s (duration: %f seconds, tps: %f)\n",
				anim_data.name.c_str(), anim_data.duration, animation->mTicksPerSecond);

			const float sample_rate = 30.0f;
			uint32 num_samples = (uint32)std::ceil(anim_data.duration * sample_rate) + 1;

			for (uint32 sample = 0; sample < num_samples; ++sample)
			{
				float time = (float)sample / sample_rate;
				if (time > anim_data.duration)
					time = anim_data.duration;
				anim_data.timestamps.push_back(time);
			}

			for (uint32 sample = 0; sample < num_samples; ++sample)
			{
				float time = anim_data.timestamps[sample];
				SkeletonPose pose((uint32)skeleton->joints.size());

				for (uint32 joint_id = 0; joint_id < skeleton->joints.size(); ++joint_id)
				{
					const auto& joint = skeleton->joints[joint_id];

					std::vector<aiNodeAnim*> node_anims = find_all_node_anims_for_bone(animation, joint.name);

					trs final_transform;

					if (!node_anims.empty())
					{
						trs local_bind_pose = get_local_bind_pose(skeleton, joint_id);

						final_transform = local_bind_pose;

						bool has_position_anim = false;
						bool has_rotation_anim = false;
						bool has_scale_anim = false;

						for (aiNodeAnim* node_anim : node_anims)
						{
							trs component_transform = sample_node_animation_component(
								node_anim,
								time,
								scale_factor,
								(float)animation->mTicksPerSecond
							);

							if (node_anim->mNumPositionKeys > 1)
							{
								final_transform.position = component_transform.position;
								has_position_anim = true;
							}
							if (node_anim->mNumRotationKeys > 1)
							{
								final_transform.rotation = component_transform.rotation;
								has_rotation_anim = true;
							}
							if (node_anim->mNumScalingKeys > 1)
							{
								final_transform.scale = component_transform.scale;
								has_scale_anim = true;
							}
						}

						if (!has_position_anim)
						{
							final_transform.position = local_bind_pose.position;
						}
						if (!has_rotation_anim)
						{
							final_transform.rotation = local_bind_pose.rotation;
						}
						if (!has_scale_anim)
						{
							final_transform.scale = local_bind_pose.scale;
						}
					}
					else
					{
						final_transform = get_local_bind_pose(skeleton, joint_id);
					}

					JointTransform joint_transform;
					joint_transform.set_transform(final_transform);
					pose.set_joint_transform(joint_transform, joint_id);
				}

				anim_data.poses.push_back(pose);
			}

			printf("Animation '%s': %u samples processed\n",
				anim_data.name.c_str(), num_samples);

			result.push_back(std::move(anim_data));
		}

		printf("Loaded %zu animations\n", result.size());
		return result;
	}

	static void swap_lr_in_string(aiString& str)
	{
		std::string name = str.C_Str();

		// Маркеры: помечаем все Left-варианты
		auto mark = [&](const std::string& from, const std::string& marker)
			{
				size_t pos = 0;
				while ((pos = name.find(from, pos)) != std::string::npos)
				{
					name.replace(pos, from.size(), marker);
					pos += marker.size();
				}
			};

		mark("_Left_", "\x01");
		mark("_left_", "\x02");
		mark("_L_", "\x03");
		mark("_l_", "\x04");
		mark("Left", "\x05");
		mark("left", "\x06");

		// Right -> Left
		auto swap_right = [&](const std::string& from, const std::string& to)
			{
				size_t pos = 0;
				while ((pos = name.find(from, pos)) != std::string::npos)
				{
					name.replace(pos, from.size(), to);
					pos += to.size();
				}
			};

		swap_right("_Right_", "_Left_");
		swap_right("_right_", "_left_");
		swap_right("_R_", "_L_");
		swap_right("_r_", "_l_");
		swap_right("Right", "Left");
		swap_right("right", "left");

		// Маркеры -> Right
		auto unmark = [&](const std::string& marker, const std::string& to)
			{
				size_t pos = 0;
				while ((pos = name.find(marker, pos)) != std::string::npos)
				{
					name.replace(pos, marker.size(), to);
					pos += to.size();
				}
			};

		unmark("\x01", "_Right_");
		unmark("\x02", "_right_");
		unmark("\x03", "_R_");
		unmark("\x04", "_r_");
		unmark("\x05", "Right");
		unmark("\x06", "right");

		str.Set(name.c_str());
	}

	static void mirror_matrix_x(aiMatrix4x4& m)
	{
		// m' = M * m * M, M = diag(-1, 1, 1, 1)
		m.b1 = -m.b1;
		m.c1 = -m.c1;
		m.d1 = -m.d1;

		m.a2 = -m.a2;
		m.a3 = -m.a3;
		m.a4 = -m.a4;
	}

	static void mirror_node_recursive(aiNode* node)
	{
		if (node == nullptr)
		{
			return;
		}

		aiVector3D pos, scale;
		aiQuaternion rot;

		node->mTransformation.Decompose(scale, rot, pos);

		pos.x = -pos.x;
		rot.y = -rot.y;
		rot.z = -rot.z;

		node->mTransformation = aiMatrix4x4(scale, rot, pos);

		swap_lr_in_string(node->mName);

		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			mirror_node_recursive(node->mChildren[i]);
		}
	}

	static void mirror_scene_x(aiScene* scene)
	{
		if (scene == nullptr)
		{
			return;
		}

		for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
		{
			aiMesh* mesh = scene->mMeshes[m];

			for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
			{
				mesh->mVertices[i].x = -mesh->mVertices[i].x;

				if (mesh->mNormals)
					mesh->mNormals[i].x = -mesh->mNormals[i].x;

				if (mesh->mTangents)
					mesh->mTangents[i].x = -mesh->mTangents[i].x;

				if (mesh->mBitangents)
					mesh->mBitangents[i].x = -mesh->mBitangents[i].x;
			}

			for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
			{
				aiFace& face = mesh->mFaces[f];

				if (face.mNumIndices == 3)
				{
					std::swap(face.mIndices[1], face.mIndices[2]);
				}
			}

			for (unsigned int b = 0; b < mesh->mNumBones; ++b)
			{
				aiBone* bone = mesh->mBones[b];

				mirror_matrix_x(bone->mOffsetMatrix);
				swap_lr_in_string(bone->mName);
			}
		}

		mirror_node_recursive(scene->mRootNode);

		for (unsigned int a = 0; a < scene->mNumAnimations; ++a)
		{
			aiAnimation* anim = scene->mAnimations[a];

			swap_lr_in_string(anim->mName);

			for (unsigned int c = 0; c < anim->mNumChannels; ++c)
			{
				aiNodeAnim* ch = anim->mChannels[c];

				swap_lr_in_string(ch->mNodeName);

				for (unsigned int i = 0; i < ch->mNumPositionKeys; ++i)
				{
					ch->mPositionKeys[i].mValue.x = -ch->mPositionKeys[i].mValue.x;
				}

				for (unsigned int i = 0; i < ch->mNumRotationKeys; ++i)
				{
					ch->mRotationKeys[i].mValue.y = -ch->mRotationKeys[i].mValue.y;
					ch->mRotationKeys[i].mValue.z = -ch->mRotationKeys[i].mValue.z;
				}
			}
		}
	}

	void import_animations(const fs::path& path, const animation::Skeleton* skeleton, uint32 flags)
	{
		if (!fs::exists(path))
		{
			LOG_WARNING("Could not find file '%ws'", path.c_str());
			return;
		}

		Assimp::Importer importer;

		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(path.string(),
			aiProcess_Triangulate |
			aiProcess_ImproveCacheLocality |
			aiProcess_SortByPType);

		if (scene == nullptr ||
			scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
			scene->mRootNode == nullptr)
		{
			LOG_ERROR(importer.GetErrorString());
			return;
		}

		aiScene* mutable_scene = const_cast<aiScene*>(scene);
		if (flags & MESH_CREATION_FLAGS_FLIP_X)
		{
			mirror_scene_x(mutable_scene);
		}

		float scale_factor = 1.0f;
		if (flags & MESH_CREATION_FLAGS_SM_TO_M)
		{
			scale_factor = 0.01f;
		}
		else if (flags & MESH_CREATION_FLAGS_M_TO_SM)
		{
			scale_factor = 100.0f;
		}

		std::vector<animation::AnimationClipAssetImportData> animations = load_animations(mutable_scene, skeleton, scale_factor, flags);
		animation::AnimationAssetClipUtils::import_animations(animations, skeleton, path, flags);
	}

	void import_skeletons(const fs::path& path, uint32 flags)
	{
		if (!fs::exists(path))
		{
			LOG_WARNING("Could not find file '%ws'", path.c_str());
			return;
		}

		Assimp::Importer importer;

		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(path.string(),
			aiProcess_Triangulate |
			aiProcess_GenNormals |
			aiProcess_FixInfacingNormals |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality |
			aiProcess_GenUVCoords |
			aiProcess_SortByPType);

		if (scene == nullptr ||
			scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
			scene->mRootNode == nullptr)
		{
			LOG_ERROR(importer.GetErrorString());
			return;
		}

		aiScene* mutable_scene = const_cast<aiScene*>(scene);
		if (flags & MESH_CREATION_FLAGS_FLIP_X)
		{
			mirror_scene_x(mutable_scene);
		}

		float scale_factor = 1.0f;
		if (flags & MESH_CREATION_FLAGS_SM_TO_M)
		{
			scale_factor = 0.01f;
		}
		else if (flags & MESH_CREATION_FLAGS_M_TO_SM)
		{
			scale_factor = 100.0f;
		}

		std::vector<animation::SkeletonAssetImportData> skeletons = load_skeletons(mutable_scene, scale_factor, flags);
		std::vector<ref<animation::Skeleton>> imported_skeletons = animation::SkeletonImportUtils::import_skeletons(skeletons);

		animation::SkeletonImportUtils::post_process_skeletons(imported_skeletons, path, flags);
	}

	void import_animations_and_skeletons(const fs::path& path, uint32 flags)
	{
		if (!fs::exists(path))
		{
			LOG_WARNING("Could not find file '%ws'", path.c_str());
			return;
		}

		Assimp::Importer importer;

		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(path.string(),
			aiProcess_Triangulate |
			aiProcess_GenNormals |
			aiProcess_FixInfacingNormals |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality |
			aiProcess_GenUVCoords |
			aiProcess_SortByPType);

		if (scene == nullptr ||
			scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
			scene->mRootNode == nullptr)
		{
			LOG_ERROR(importer.GetErrorString());
			return;
		}

		aiScene* mutable_scene = const_cast<aiScene*>(scene);
		if (flags & MESH_CREATION_FLAGS_FLIP_X)
		{
			mirror_scene_x(mutable_scene);
		}

		float scale_factor = 1.0f;
		if (flags & MESH_CREATION_FLAGS_SM_TO_M)
		{
			scale_factor = 0.01f;
		}
		else if (flags & MESH_CREATION_FLAGS_M_TO_SM)
		{
			scale_factor = 100.0f;
		}

		std::vector<animation::SkeletonAssetImportData> skeletons = load_skeletons(mutable_scene, scale_factor, flags);

		if (!skeletons.empty())
		{
			std::vector<ref<animation::Skeleton>> imported_skeletons = animation::SkeletonImportUtils::import_skeletons(skeletons);

			std::vector<animation::AnimationClipAssetImportData> animations = load_animations(mutable_scene, imported_skeletons[0].get(), scale_factor, flags);

			animation::SkeletonImportUtils::post_process_skeletons(imported_skeletons, path, flags);

			animation::AnimationAssetClipUtils::import_animations(animations, imported_skeletons[0].get(), path, flags);
		}
	}

	ModelAsset import_3d_model_from_file(const fs::path& path, uint32 flags)
	{
		if (!fs::exists(path))
		{
			LOG_WARNING("Could not find file '%ws'", path.c_str());
			std::cerr << "Could not find file '" << path << "'.\n";
			return {};
		}

		ModelAsset result;
		result.flags = flags;

		Assimp::Importer importer;

		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(path.string(),
			aiProcess_Triangulate |
			aiProcess_GenNormals |
			aiProcess_FixInfacingNormals |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality |
			aiProcess_GenUVCoords |
			aiProcess_SortByPType);

		if (scene == nullptr ||
			scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
			scene->mRootNode == nullptr)
		{
			LOG_ERROR(importer.GetErrorString());
			return result;
		}

		aiScene* mutable_scene = const_cast<aiScene*>(scene);
		mirror_scene_x(mutable_scene);

		load_materials(mutable_scene, result);

		float scale_factor = 1.0f;
		if (flags & MESH_CREATION_FLAGS_SM_TO_M)
		{
			scale_factor = 0.01f;
		}
		else if (flags & MESH_CREATION_FLAGS_M_TO_SM)
		{
			scale_factor = 100.0f;
		}

		load_meshes(mutable_scene, scale_factor, result);

		std::vector<animation::SkeletonAssetImportData> skeletons = load_skeletons(mutable_scene, scale_factor, flags);

		if (!skeletons.empty())
		{
			std::vector<ref<animation::Skeleton>> imported_skeletons = animation::SkeletonImportUtils::import_skeletons(skeletons);

			std::vector<animation::AnimationClipAssetImportData> animations = load_animations(mutable_scene, imported_skeletons[0].get(), scale_factor, flags);

			animation::SkeletonImportUtils::post_process_skeletons(imported_skeletons, path, flags);

			animation::AnimationAssetClipUtils::import_animations(animations, imported_skeletons[0].get(), path, flags);
		}

		return result;
	}
}