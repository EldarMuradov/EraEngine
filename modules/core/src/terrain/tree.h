// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"
#include "core/job_system.h"

#include "geometry/mesh.h"

#include "rendering/material.h"
#include "rendering/pbr.h"

#include "ecs/component.h"

namespace era_engine
{
    struct ERA_CORE_API TreeSettings
    {
        float bend_strength;
    };

    class ERA_CORE_API TreeComponent : public Component
    {
    public:
        TreeComponent() = default;
        TreeComponent(ref<Entity::EcsData> _data, const TreeSettings& _settings);
        ~TreeComponent() override;

        ERA_VIRTUAL_REFLECT(Component)

    public:
        TreeSettings settings;
    };

    void render_tree(struct OpaqueRenderPass* render_pass, D3D12_GPU_VIRTUAL_ADDRESS transforms, uint32 num_instances, const MultiMesh* mesh, float dt);

    void initialize_tree_pipelines();

    ref<MultiMesh> load_tree_mesh_from_file(const fs::path& scene_filename);
    ref<MultiMesh> load_tree_mesh_from_handle(AssetHandle handle);

    ref<MultiMesh> load_tree_mesh_from_file_async(const fs::path& scene_filename, JobHandle parent_job = {});
    ref<MultiMesh> load_tree_mesh_from_handle_async(AssetHandle handle, JobHandle parent_job = {});
}