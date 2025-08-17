#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include <ecs/entity.h>
#include <ecs/component.h>

namespace era_engine::physics
{
    class PhysicalAnimationLimbComponent;
    class PhysicalAnimationComponent;

	class ERA_PHYSICS_API DrivePoseSolver
	{
    public:
        DrivePoseSolver(ComponentPtr _physical_animation_component_ptr);

        void solve_pose(float dt) const;

        void force_solve_collided_limb(PhysicalAnimationLimbComponent* limb_component) const;

    private:
        void process_by_sweep(const PhysicalAnimationComponent* physical_animation_component,
            PhysicalAnimationLimbComponent* limb_component,
            float dt) const;

        void process_by_collision(PhysicalAnimationLimbComponent* limb_component,
            float dt) const;

        void solve_blocked_limb(PhysicalAnimationLimbComponent* limb_component, float dt) const;
        void solve_collided_limb(PhysicalAnimationLimbComponent* limb_component) const;

        void solve_free_limb(PhysicalAnimationLimbComponent* limb_component) const;

        void solve_released_limb(PhysicalAnimationLimbComponent* limb_component, float dt, bool solve_as_blocking = true) const;

        bool should_process_sweep(uint8 limb_type) const;

    private:
        ComponentPtr physical_animation_component_ptr = nullptr;
	};


}