#pragma once

#define PX_PHYSX_CHARACTER_STATIC_LIB

#include "physics_api.h"

#include "physics/body_component.h"

#include "ecs/component.h"

namespace era_engine::physics
{
    class ERA_PHYSICS_API CharacterControllerComponent final : public Component
    {
    public:
        CharacterControllerComponent() = default;
        CharacterControllerComponent(ref<Entity::EcsData> _data);
        ~CharacterControllerComponent() override;

        float height = 1.2f;
        float radius = 0.3f;

        float step_offset = 0.5f;
        float contact_offset = 0.1f;

        float slope_limit = deg2rad(30.0f);

        ObservableMember<vec3> velocity;

        ERA_VIRTUAL_REFLECT(Component)

    private:
        physx::PxController* controller = nullptr;
    };
}