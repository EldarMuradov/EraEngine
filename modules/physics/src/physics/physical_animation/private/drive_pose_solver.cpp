#include "physics/physical_animation/drive_pose_solver.h"
#include "physics/scene_queries.h"
#include "physics/shape_component.h"
#include "physics/body_component.h"
#include "physics/physical_animation/physical_animation_component.h"
#include "physics/joint.h"
#include "physics/core/physics_utils.h"

#include <core/math.h>
#include <core/traits.h>
#include <ecs/base_components/transform_component.h>

namespace era_engine::physics
{
	DrivePoseSolver::DrivePoseSolver(ComponentPtr _physical_animation_component_ptr)
		: physical_animation_component_ptr(_physical_animation_component_ptr)
	{
		ASSERT(physical_animation_component_ptr.get() != nullptr);
	}

	void DrivePoseSolver::solve_pose(float dt) const
	{
		const PhysicalAnimationComponent* physical_animation_component = dynamic_cast<const PhysicalAnimationComponent*>(physical_animation_component_ptr.get());

		for (const auto& limb_ptr : physical_animation_component->limbs)
		{
			PhysicalAnimationLimbComponent* limb_data_component = limb_ptr.get().get_component<PhysicalAnimationLimbComponent>();

            if (limb_data_component->is_colliding)
            {
                solve_collided_limb(limb_data_component);
            }
            else
            {
                if (limb_data_component->was_in_collision ||
                    limb_data_component->collision_time > 0.0f)
                {
                    solve_released_limb(limb_data_component, dt);
                }
                else
                {
                    solve_free_limb(limb_data_component);
                }
            }

            if (limb_data_component->drive_joint_component.get() != nullptr)
            {
                D6JointComponent* drive_joint = dynamic_cast<D6JointComponent*>(limb_data_component->drive_joint_component.get_for_write());
                PhysicsUtils::manual_set_physics_transform(drive_joint->get_first_entity_ptr().get(), limb_data_component->adjusted_pose, true);
            }
		}
	}

	void DrivePoseSolver::force_solve_collided_limb(PhysicalAnimationLimbComponent* limb_component) const
	{
		solve_collided_limb(limb_component);
	}

	void DrivePoseSolver::solve_collided_limb(PhysicalAnimationLimbComponent* limb_component) const
	{
        limb_component->adjusted_pose = limb_component->physics_pose;
        limb_component->is_colliding = true;
	}

	void DrivePoseSolver::solve_free_limb(PhysicalAnimationLimbComponent* limb_component) const
	{
        // Don't change other state data.
        limb_component->adjusted_pose = limb_component->target_pose;
	}

	void DrivePoseSolver::solve_released_limb(PhysicalAnimationLimbComponent* limb_component, float dt, bool solve_as_blocking) const
	{
        const PhysicalAnimationComponent* physical_animation_component = dynamic_cast<const PhysicalAnimationComponent*>(physical_animation_component_ptr.get());

        const float used_collision_time = limb_component->type == PhysicalAnimationLimbComponent::Type::HAND ?
            PhysicalAnimationLimbComponent::MAX_FREQUENT_COLLISION_TIME :
            PhysicalAnimationLimbComponent::MAX_COLLISION_TIME;

        const float interpolation_factor = map_value(limb_component->collision_time,
            0.0f,
            used_collision_time,
            physical_animation_component->target_position_blend_factor,
            1.0f);

        // Event if blend_factor is close to 0 we need to consider physics_pose.
        limb_component->adjusted_pose.position = lerp(limb_component->physics_pose.position,
            limb_component->target_pose.position,
            interpolation_factor);

        limb_component->adjusted_pose.rotation = slerp(limb_component->physics_pose.rotation,
            limb_component->target_pose.rotation,
            interpolation_factor);
	}
}