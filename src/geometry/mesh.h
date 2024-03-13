#pragma once

#include "asset/asset.h"
#include "physics/bounding_volumes.h"
#include "dx/dx_buffer.h"
#include "animation/animation.h"
#include "mesh_builder.h"
#include "core/job_system.h"

struct pbr_material;

struct submesh
{
	submesh_info info;
	bounding_box aabb; // local space.
	trs transform;

	ref<pbr_material> material;
	std::string name;
};

struct multi_mesh
{
	std::vector<submesh> submeshes;
	animation_skeleton skeleton;
	dx_mesh mesh;
	bounding_box aabb = { vec3(0.f), vec3(0.f) };

	asset_handle handle;
	uint32 flags;

	std::atomic<asset_load_state> loadState = asset_loaded;
	job_handle loadJob;
};

using mesh_load_callback = std::function<void(mesh_builder& builder, std::vector<submesh>& submeshes, const bounding_box& boundingBox)>;

NODISCARD ref<multi_mesh> loadMeshFromFile(const fs::path& filename, uint32 flags = mesh_creation_flags_default, mesh_load_callback cb = nullptr);
NODISCARD ref<multi_mesh> loadMeshFromHandle(asset_handle handle, uint32 flags = mesh_creation_flags_default, mesh_load_callback cb = nullptr);
NODISCARD ref<multi_mesh> loadMeshFromFileAsync(const fs::path& filename, uint32 flags = mesh_creation_flags_default, job_handle parentJob = {}, mesh_load_callback cb = nullptr);
NODISCARD ref<multi_mesh> loadMeshFromHandleAsync(asset_handle handle, uint32 flags = mesh_creation_flags_default, job_handle parentJob = {}, mesh_load_callback cb = nullptr);

struct eentity;
struct escene;

NODISCARD eentity loadEntityMeshFromFile(escene scene, const fs::path& filename, uint32 flags = mesh_creation_flags_default, mesh_load_callback cb = nullptr);

// Same functions but with different default flags (includes skin).
inline NODISCARD ref<multi_mesh> loadAnimatedMeshFromFile(const fs::path& filename, uint32 flags = mesh_creation_flags_animated, mesh_load_callback cb = nullptr)
{
	return loadMeshFromFile(filename, flags, cb);
}

inline NODISCARD ref<multi_mesh> loadAnimatedMeshFromHandle(asset_handle handle, uint32 flags = mesh_creation_flags_animated, mesh_load_callback cb = nullptr)
{
	return loadMeshFromHandle(handle, flags, cb);
}

inline NODISCARD ref<multi_mesh> loadAnimatedMeshFromFileAsync(const fs::path& filename, uint32 flags = mesh_creation_flags_animated, job_handle parentJob = {}, mesh_load_callback cb = nullptr)
{
	return loadMeshFromFileAsync(filename, flags, parentJob, cb);
}

inline NODISCARD ref<multi_mesh> loadAnimatedMeshFromHandleAsync(asset_handle handle, uint32 flags = mesh_creation_flags_animated, job_handle parentJob = {}, mesh_load_callback cb = nullptr)
{
	return loadMeshFromHandleAsync(handle, flags, parentJob, cb);
}

struct mesh_component
{
	ref<multi_mesh> mesh;
};