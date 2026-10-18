#pragma once

#include <ecs/system.h>
#include <ecs/entity.h>

#include <geometry/mesh.h>
#include <rendering/pbr_material.h>

namespace era_engine
{
    class EditorInitSystem final : public System
    {
    public:
        EditorInitSystem(World* _world);
        ~EditorInitSystem();

        void init() override;
        void update(float dt) override;

        ERA_VIRTUAL_REFLECT(System)

    private:
        Entity camera_entity;

        ref<MultiMesh> sphere_mesh;
        ref<PBRMaterial> sphere_render_material;
    };
}