#pragma once

#include "core_api.h"
#include "ecs/component.h"

#include "geometry/mesh.h"

namespace era_engine
{
    class ERA_CORE_API MeshComponent : public Component
    {
    public:
        MeshComponent(ref<Entity::EcsData> _data, ref<MultiMesh> _mesh = nullptr, bool _is_hidden = false);
        ~MeshComponent() override;

        // Returns the transform rendered in the previous frame.
        mat4 update_frame_transform(const mat4& transform, uint64 frame_id);

        ERA_VIRTUAL_REFLECT(Component)

    public:
        ref<MultiMesh> mesh;
        bool is_hidden = false;

    private:
        mat4 frame_transform = mat4::identity;
        mat4 prev_frame_transform = mat4::identity;
        uint64 frame_transform_id = UINT64_MAX;
    };

    class MeshUtils final
    {
        MeshUtils() = delete;

    public:
        static Entity load_entity_mesh_from_file(ref<World> world, const fs::path& filename, uint32 flags = MESH_CREATION_FLAGS_DEFAULT, MeshLoadCallback cb = nullptr);
        static Entity load_entity_mesh_from_handle(ref<World> world, AssetHandle handle, uint32 flags = MESH_CREATION_FLAGS_DEFAULT, MeshLoadCallback cb = nullptr);

        static Entity load_entity_mesh_from_file_async(ref<World> world, const fs::path& filename, uint32 flags = MESH_CREATION_FLAGS_DEFAULT, MeshLoadCallback cb = nullptr, JobHandle parent_job = {});
        static Entity load_entity_mesh_from_handle_async(ref<World> world, AssetHandle handle, uint32 flags = MESH_CREATION_FLAGS_DEFAULT, MeshLoadCallback cb = nullptr, JobHandle parent_job = {});
    };
}