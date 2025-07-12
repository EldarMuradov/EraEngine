#pragma once

#define PX_PHYSX_CHARACTER_STATIC_LIB

#include "physics_api.h"

#include "physics/body_component.h"

#include "ecs/component.h"

namespace era_engine::physics
{
    class ERA_PHYSICS_API CCTBaseComponent : public BodyComponent
    {
    public:
        CCTBaseComponent() = default;
        CCTBaseComponent(ref<Entity::EcsData> _data, float _mass = 1.0f);
        ~CCTBaseComponent() override;

        float mass = 1.0f;

        ERA_VIRTUAL_REFLECT(BodyComponent)

    protected:
        virtual void create_character_controller();

        void register_cct();

    protected:
        physx::PxControllerManager* manager = nullptr;
        physx::PxController* controller = nullptr;
    };

    class ERA_PHYSICS_API BoxCCTComponent : public CCTBaseComponent
    {
    public:
        BoxCCTComponent() = default;
        BoxCCTComponent(ref<Entity::EcsData> _data, float _half_height, float _half_side_extent, float _mass = 1.0f);
        ~BoxCCTComponent() override;

        float half_height = 1.0f;
        float half_side_extent = 0.5f;

        ERA_VIRTUAL_REFLECT(CCTBaseComponent)

    protected:
        void create_character_controller() override;
    };

    class ERA_PHYSICS_API CapsuleCCTComponent : public CCTBaseComponent
    {
    public:
        CapsuleCCTComponent() = default;
        CapsuleCCTComponent(ref<Entity::EcsData> _data, float _height, float _radius, float _mass = 1.0f);
        ~CapsuleCCTComponent() override;

        float height = 2.0f;
        float radius = 0.5f;

        ERA_VIRTUAL_REFLECT(CCTBaseComponent)

    protected:
        void create_character_controller() override;
    };
}