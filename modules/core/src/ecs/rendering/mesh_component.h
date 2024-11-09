#pragma once

#include "ecs/component.h"

#include "geometry/mesh.h"

namespace era_engine
{
	class MeshComponent : public Component
	{
	public:
		MeshComponent(ref<Entity::EcsData> _data, ref<multi_mesh> _mesh, bool _is_hidden = false);
		virtual ~MeshComponent();

		ERA_VIRTUAL_REFLECT(Component)

	public:
		ref<multi_mesh> mesh;
		bool is_hidden = false;
	};

	class MeshUtils final
	{
		MeshUtils() = delete;
	public:
		static Entity load_entity_mesh_from_file(ref<World> world, const fs::path& filename, uint32 flags = mesh_creation_flags_default, mesh_load_callback cb = nullptr);
		static Entity load_entity_mesh_from_handle(ref<World> world, asset_handle handle, uint32 flags = mesh_creation_flags_default, mesh_load_callback cb = nullptr);
		
		static Entity load_entity_mesh_from_file_async(ref<World> world, const fs::path& filename, uint32 flags = mesh_creation_flags_default, mesh_load_callback cb = nullptr, job_handle parent_job = {});
		static Entity load_entity_mesh_from_handle_async(ref<World> world, asset_handle handle, uint32 flags = mesh_creation_flags_default, mesh_load_callback cb = nullptr, job_handle parent_job = {});
	};
}