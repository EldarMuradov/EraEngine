// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "terrain/terrain.h"

#include "ecs/component.h"

namespace era_engine
{
    struct ERA_CORE_API ProcPlacementLayerDesc
    {
        const char* name;
        float footprint;
        ref<MultiMesh> meshes[4] = {};
    };

    class ERA_CORE_API ProcPlacementComponent : public Component
    {
    public:
        ProcPlacementComponent() = default;
        ProcPlacementComponent(ref<Entity::EcsData> _data, const std::vector<ProcPlacementLayerDesc>& _layers);
        ~ProcPlacementComponent() override;

        void generate(const RenderCamera& camera, const TerrainComponent& terrain, const vec3& position_offset);
        void render(struct LDRRenderPass* render_pass);

        struct PlacementLayer
        {
            const char* name;

            float footprint;

            uint32 global_mesh_offset;
            uint32 num_meshes;

            ref<MultiMesh> meshes[4];
            float densities[4] = {0.25f, 0.25f, 0.25f, 0.25f};
        };

        ERA_VIRTUAL_REFLECT(Component)

    public:
        std::vector<PlacementLayer> layers;

    private:
        bool has_valid_meshes = false;

        std::vector<uint32> submesh_to_mesh;

        ref<DXBuffer> placement_point_buffer;
        ref<DXBuffer> transform_buffer;
        ref<DXBuffer> mesh_count_buffer;
        ref<DXBuffer> mesh_offset_buffer;
        ref<DXBuffer> submesh_to_mesh_buffer;
        ref<DXBuffer> draw_indirect_buffer;
    };

    void initialize_procedural_placement_pipelines();
}