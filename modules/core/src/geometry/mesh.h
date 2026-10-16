#pragma once

#include "core/job_system.h"
#include "core/bounding_volumes.h"

#include "asset/asset.h"
#include "asset/pbr_material_desc.h"
#include "asset/game_asset.h"
#include "asset/model_asset.h"

#include "geometry/mesh_builder.h"

namespace era_engine
{
	struct PBRMaterial;

	struct ERA_CORE_API Submesh
	{
		SubmeshInfo info;
		BoundingBox aabb; // local space.
		trs transform;

		ref<PBRMaterial> material;
		std::string name;

		bool is_visible = true;
	};

	class ERA_CORE_API MultiMesh : public GameAsset
	{
	public:
		~MultiMesh() override;

		bool serialize(std::ostream& os) const override;
		bool deserialize(std::istream& is) override;

		static std::string get_asset_type_impl();

		void update_mesh(const ModelAsset& new_model_asset);

		bool has_visible_submeshes() const;

		std::optional<ModelAsset> model_asset;
		
		std::vector<Submesh> submeshes;
		DXMesh mesh;
		BoundingBox aabb = { vec3(0.f), vec3(0.f) };

		std::mutex mutex;
	};

	using MeshLoadCallback = std::function<void(MeshBuilder& builder, std::vector<Submesh>& submeshes, const BoundingBox& bounding_box)>;

	ERA_CORE_API ref<MultiMesh> import_mesh_from_file(const fs::path& filename, uint32 flags = MESH_CREATION_FLAGS_DEFAULT, const MeshLoadCallback& cb = nullptr);
	ERA_CORE_API ref<MultiMesh> import_mesh_from_handle(AssetHandle handle, uint32 flags = MESH_CREATION_FLAGS_DEFAULT, const MeshLoadCallback& cb = nullptr);
	ERA_CORE_API ref<MultiMesh> import_mesh_from_file_async(const fs::path& filename, uint32 flags = MESH_CREATION_FLAGS_DEFAULT, JobHandle parent_job = {}, const MeshLoadCallback& cb = nullptr);
	ERA_CORE_API ref<MultiMesh> import_mesh_from_handle_async(AssetHandle handle, uint32 flags = MESH_CREATION_FLAGS_DEFAULT, JobHandle parent_job = {}, const MeshLoadCallback& cb = nullptr);

	// Same functions but with different default flags (includes skin).
	inline ref<MultiMesh> import_animated_mesh_from_file(const fs::path& filename, uint32 flags = MESH_CREATION_FLAGS_ANIMATED, const MeshLoadCallback& cb = nullptr)
	{
		return import_mesh_from_file(filename, flags, cb);
	}

	inline ref<MultiMesh> import_animated_mesh_from_handle(AssetHandle handle, uint32 flags = MESH_CREATION_FLAGS_ANIMATED, const MeshLoadCallback& cb = nullptr)
	{
		return import_mesh_from_handle(handle, flags, cb);
	}

	inline ref<MultiMesh> import_animated_mesh_from_file_async(const fs::path& filename, uint32 flags = MESH_CREATION_FLAGS_ANIMATED, JobHandle parent_job = {}, const MeshLoadCallback& cb = nullptr)
	{
		return import_mesh_from_file_async(filename, flags, parent_job, cb);
	}

	inline ref<MultiMesh> import_animated_mesh_from_handle_async(AssetHandle handle, uint32 flags = MESH_CREATION_FLAGS_ANIMATED, JobHandle parent_job = {}, const MeshLoadCallback& cb = nullptr)
	{
		return import_mesh_from_handle_async(handle, flags, parent_job, cb);
	}

}