// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "geometry/mesh.h"

#include "rendering/pbr.h"

#include "core/hash.h"
#include "core/string.h"

#include "asset/file_registry.h"
#include "asset/model_asset.h"

namespace era_engine
{
	struct mesh_key
	{
		AssetHandle handle;
		uint32 flags;
	};
}

namespace std
{
	template<>
	struct hash<era_engine::mesh_key>
	{
		size_t operator()(const era_engine::mesh_key& x) const
		{
			size_t seed = 0;
			hash_combine(seed, x.handle);
			hash_combine(seed, x.flags);
			return seed;
		}
	};
}

namespace era_engine
{
	static void meshLoaderThread(ref<multi_mesh> result, const fs::path& sceneFilename, uint32 flags, mesh_load_callback cb,
		bool async, JobHandle parentJob)
	{
		using namespace animation;

		result->aabb = bounding_box::negativeInfinity();

		ModelAsset asset = load_3d_model_from_file(sceneFilename);
		mesh_builder builder(flags | mesh_creation_flags_with_skin);

		for (auto& mesh : asset.meshes)
		{
			for (auto& sub : mesh.submeshes)
			{
				PbrMaterialDesc& materialDesc = asset.materials[sub.material_index];

				{
					if (!materialDesc.albedo.empty() && *materialDesc.albedo.c_str() != 'F')
						materialDesc.albedo = get_asset_path(convert_path(materialDesc.albedo.string()));
					if (!materialDesc.normal.empty() && *materialDesc.normal.c_str() != 'F')
						materialDesc.normal = get_asset_path(convert_path(materialDesc.normal.string()));
					if (!materialDesc.roughness.empty() && *materialDesc.roughness.c_str() != 'F')
						materialDesc.roughness = get_asset_path(convert_path(materialDesc.roughness.string()));
					if (!materialDesc.metallic.empty() && *materialDesc.metallic.c_str() != 'F')
						materialDesc.metallic = get_asset_path(convert_path(materialDesc.metallic.string()));
				}

				ref<pbr_material> material;
				if (!async)
				{
					material = createPBRMaterial(materialDesc);
				}
				else
				{
					material = createPBRMaterialAsync(materialDesc, parentJob);
				}

				bounding_box aabb;
				builder.pushMesh(sub, 1.f, &aabb);
				result->submeshes.push_back({ builder.endSubmesh(), aabb, trs::identity, material, mesh.name });

				result->aabb.grow(aabb.minCorner);
				result->aabb.grow(aabb.maxCorner);
			}
		}

		Skeleton& skeleton = result->skeleton;

		AnimationSkeleton& animation_skeleton = result->animation_skeleton;
		animation_skeleton.skeleton = &result->skeleton;

		// Load skeleton
		if (!asset.skeletons.empty()/* && flags & mesh_creation_flags_with_skin*/)
		{
			SkeletonAsset& in = asset.skeletons.front();

			skeleton.joints = std::move(in.joints);
			skeleton.name_to_joint_id = std::move(in.name_to_joint_id);
			skeleton.analyze_joints(builder.getPositions(), (uint8*)builder.getOthers() + builder.getSkinOffset(), builder.getOthersSize(), builder.getNumVertices());
		}

		// Load animations
		for (auto& anim : asset.animations)
		{
			AnimationAsset& in = anim;

			AnimationClip& clip = animation_skeleton.clips.emplace_back();
			clip.name = std::move(in.name);
			clip.filename = sceneFilename;
			clip.length_in_seconds = in.duration;
			clip.joints.resize(skeleton.joints.size(), {});

			clip.position_keyframes = std::move(in.position_keyframes);
			clip.position_timestamps = std::move(in.position_timestamps);
			clip.rotation_keyframes = std::move(in.rotation_keyframes);
			clip.rotation_timestamps = std::move(in.rotation_timestamps);
			clip.scale_keyframes = std::move(in.scale_keyframes);
			clip.scale_timestamps = std::move(in.scale_timestamps);

			for (auto& [name, joint] : in.joints)
			{
				auto it = skeleton.name_to_joint_id.find(name);
				if (it != skeleton.name_to_joint_id.end())
				{
					AnimationJoint& j = clip.joints[it->second];
					j = joint;
				}
			}
		}

		if (cb)
		{
			cb(builder, result->submeshes, result->aabb);
		}

		result->mesh = builder.createDXMesh();

		result->loadState.store(AssetLoadState::LOADED, std::memory_order_release);
	}

	static ref<multi_mesh> loadMeshFromFileInternal(const fs::path& sceneFilename, AssetHandle handle, uint32 flags, mesh_load_callback cb,
		bool async, JobHandle parentJob)
	{
		ref<multi_mesh> result = make_ref<multi_mesh>();
		result->handle = handle;
		result->flags = flags;
		result->loadState = AssetLoadState::LOADING;

		if (!async)
		{
			meshLoaderThread(result, sceneFilename, flags, cb, false, {});
			result->loadJob = {};
			return result;
		}
		else
		{
			struct mesh_loading_data
			{
				mesh_load_callback cb;
				fs::path path;
				ref<multi_mesh> mesh;
				uint32 flags;
			};

			constexpr int a = sizeof(mesh_loading_data);

			mesh_loading_data data = { cb, sceneFilename, result, flags };

			JobHandle job = low_priority_job_queue.createJob<mesh_loading_data>([](mesh_loading_data& data, JobHandle job)
				{
					meshLoaderThread(data.mesh, data.path, data.flags, data.cb, true, job);
				}, data, parentJob);
			job.submit_now();

			result->loadJob = job;

			return result;
		}
	}

	static bool operator==(const mesh_key& a, const mesh_key& b)
	{
		return a.handle == b.handle && a.flags == b.flags;
	}

	static std::unordered_map<mesh_key, weakref<multi_mesh>> meshCache;
	static std::mutex mutex;

	static ref<multi_mesh> loadMeshFromFileAndHandle(const fs::path& filename, AssetHandle handle, uint32 flags, mesh_load_callback cb,
		bool async = false, JobHandle parentJob = {})
	{
		if (!fs::exists(filename))
			return 0;

		mesh_key key = { handle, flags };

		mutex.lock();

		ref<multi_mesh> result = { meshCache[key].lock(), {} };
		if (!result)
		{
			result = loadMeshFromFileInternal(filename, handle, flags, cb, async, parentJob);
			meshCache[key] = result;
		}

		mutex.unlock();
		return result;
	}

	NODISCARD ref<multi_mesh> loadMeshFromFile(const fs::path& filename, uint32 flags, mesh_load_callback cb)
	{
		fs::path path = filename.lexically_normal().make_preferred();

		AssetHandle handle = getAssetHandleFromPath(path);
		return loadMeshFromFileAndHandle(path, handle, flags, cb);
	}

	NODISCARD ref<multi_mesh> loadMeshFromHandle(AssetHandle handle, uint32 flags, mesh_load_callback cb)
	{
		fs::path sceneFilename = getPathFromAssetHandle(handle);
		return loadMeshFromFileAndHandle(sceneFilename, handle, flags, cb);
	}

	NODISCARD ref<multi_mesh> loadMeshFromFileAsync(const fs::path& filename, uint32 flags, JobHandle parentJob, mesh_load_callback cb)
	{
		fs::path path = filename.lexically_normal().make_preferred();

		AssetHandle handle = getAssetHandleFromPath(path);
		return loadMeshFromFileAndHandle(path, handle, flags, cb, true, parentJob);
	}

	NODISCARD ref<multi_mesh> loadMeshFromHandleAsync(AssetHandle handle, uint32 flags, JobHandle parentJob, mesh_load_callback cb)
	{
		fs::path sceneFilename = getPathFromAssetHandle(handle);
		return loadMeshFromFileAndHandle(sceneFilename, handle, flags, cb, true, parentJob);
	}
}