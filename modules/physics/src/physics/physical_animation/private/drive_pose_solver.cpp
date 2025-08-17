#include "physics/physical_animation/drive_pose_solver.h"
#include "physics/scene_queries.h"
#include "physics/shape_component.h"
#include "physics/body_component.h"
#include "physics/physical_animation/physical_animation_component.h"
#include "physics/joint.h"

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

			// If we are too close to target_pose - we already arrived and there is no need to do sweep check to target pose.
			if (should_process_sweep(value_of(limb_data_component->type)))
			{
				process_by_sweep(physical_animation_component, limb_data_component, dt);
			}
			else
			{
				process_by_collision(limb_data_component, dt);
			}
		}
	}

	void DrivePoseSolver::force_solve_collided_limb(PhysicalAnimationLimbComponent* limb_component) const
	{
		solve_collided_limb(limb_component);
	}

	void DrivePoseSolver::process_by_sweep(const PhysicalAnimationComponent* physical_animation_component, PhysicalAnimationLimbComponent* limb_component, float dt) const
	{
        SceneQueryGeometry geometry;
        trs shape_local_transform;
        {
            const ShapeComponent* shape_component = nullptr;
            if (const BoxShapeComponent* box_shape_component = limb_component->get_entity().get_component_if_exists<BoxShapeComponent>())
            {
                shape_component = box_shape_component;
                geometry.type = SceneQueryGeometry::Type::BOX;
                geometry.box_half_extents = box_shape_component->half_extents;
            }
            else if (const CapsuleShapeComponent* capsule_shape_component = limb_component->get_entity().get_component_if_exists<CapsuleShapeComponent>())
            {
                shape_component = capsule_shape_component;
                geometry.type = SceneQueryGeometry::Type::CAPSULE;
                geometry.capsule_half_height = capsule_shape_component->half_height;
                geometry.capsule_radius = capsule_shape_component->radius;
            }
            else
            {
                ASSERT(false);
                return;
            }

            shape_local_transform = trs(shape_component->local_position, shape_component->local_rotation, vec3(1.0f));
        }
        trs geometry_transform = limb_component->physics_pose * shape_local_transform;
        geometry_transform.rotation = normalize(geometry_transform.rotation);

        trs desired_geometry_transform = limb_component->target_pose * shape_local_transform;
        desired_geometry_transform.rotation = normalize(desired_geometry_transform.rotation);

        const vec3 to_target = desired_geometry_transform.position - geometry_transform.position;
        const float distance = length(to_target);

        // We do sweep only for the future limb trajectory.
        if (distance > 0.01f)
        {
            World* world = physical_animation_component->get_world();

            const vec3 direction = normalize(to_target);

            SceneQueryFilter raycast_filter;
            // We can collide not only with players. But with other obstacles too. But now it is not used because it works only in physics contest for now.
            raycast_filter.collision_mask =
                SceneQueryFilter::construct_from_type(world, value_of(CollisionType::RAGDOLL)).collision_mask;
            set_flag(raycast_filter.collision_mask, CollisionType::TERRAIN, false);

            // We also want to skip ignore list in raycast. Assume that the animation doesn't go away on its own.
            for (const auto& limb_ptr : physical_animation_component->limbs)
            {
                raycast_filter.ignore_list.emplace_back(limb_ptr.get());
            }

            SweepQuery::Params params;
            params.geometry = geometry;
            params.geometry_transform = geometry_transform;
            params.direction = direction;
            params.distance = distance;
            params.filter_flags = SceneQueryFilterFlag::STATICS | SceneQueryFilterFlag::DYNAMICS | SceneQueryFilterFlag::PREFILTER;
            params.filter_callback = &raycast_filter;

            SceneQueryPositionedHit hit;
            if (SweepQuery::closest(world, params, &hit))
            {
                solve_blocked_limb(limb_component, dt);
            }
            else
            {
                solve_released_limb(limb_component, dt, true);
            }
        }
        else
        {
            solve_free_limb(limb_component);
        }
	}

	void DrivePoseSolver::process_by_collision(PhysicalAnimationLimbComponent* limb_component, float dt) const
	{
        if (limb_component->is_colliding)
        {
            solve_collided_limb(limb_component);
        }
        else
        {
            if (limb_component->was_in_collision || limb_component->collision_time > 0.0f)
            {
                solve_released_limb(limb_component, dt, false);
            }
            else
            {
                solve_free_limb(limb_component);
            }
        }
	}

	void DrivePoseSolver::solve_blocked_limb(PhysicalAnimationLimbComponent* limb_component, float dt) const
	{
        limb_component->is_blocked = true;

        // Smooth go from 0 to 1.
        limb_component->blocked_blend_factor = min(limb_component->blocked_blend_factor + 2.0f * dt, 1.0f);

        const float interpolation_factor = 1.0f - limb_component->blocked_blend_factor * 0.7f;
        limb_component->adjusted_pose.position = lerp(limb_component->target_pose.position,
            limb_component->physics_pose.position,
            interpolation_factor);

        limb_component->adjusted_pose.rotation = slerp(limb_component->target_pose.rotation,
            limb_component->physics_pose.rotation,
            interpolation_factor);
	}

	void DrivePoseSolver::solve_collided_limb(PhysicalAnimationLimbComponent* limb_component) const
	{
        limb_component->is_colliding = true;
        limb_component->is_blocked = true;
        limb_component->blocked_blend_factor = 1.0f;
        limb_component->adjusted_pose = limb_component->physics_pose;
	}

	void DrivePoseSolver::solve_free_limb(PhysicalAnimationLimbComponent* limb_component) const
	{
        const PhysicalAnimationComponent* physical_animation_component = dynamic_cast<const PhysicalAnimationComponent*>(physical_animation_component_ptr.get());

        limb_component->blocked_blend_factor = 0.0f;
        limb_component->is_blocked = false;

        limb_component->adjusted_pose.position = lerp(limb_component->target_pose.position,
            limb_component->physics_pose.position,
            physical_animation_component->target_position_blend_factor);

        limb_component->adjusted_pose.rotation = slerp(limb_component->target_pose.rotation,
            limb_component->physics_pose.rotation,
            physical_animation_component->target_rotation_blend_factor);
	}

	void DrivePoseSolver::solve_released_limb(PhysicalAnimationLimbComponent* limb_component, float dt, bool solve_as_blocking) const
	{
        const PhysicalAnimationComponent* physical_animation_component = dynamic_cast<const PhysicalAnimationComponent*>(physical_animation_component_ptr.get());

        // Smooth go from 1 to 0.
        limb_component->blocked_blend_factor = max(limb_component->blocked_blend_factor - 3.0f * dt, 0.0f);

        limb_component->is_blocked = limb_component->blocked_blend_factor > 0.0f;

        if (solve_as_blocking)
        {
            // Event if blend_factor is close to 0 we need to consider physics_pose.
            limb_component->adjusted_pose.position = lerp(limb_component->target_pose.position,
                limb_component->physics_pose.position,
                max(limb_component->blocked_blend_factor, physical_animation_component->target_position_blend_factor));

            limb_component->adjusted_pose.rotation = slerp(limb_component->target_pose.rotation,
                limb_component->physics_pose.rotation,
                max(limb_component->blocked_blend_factor, physical_animation_component->target_rotation_blend_factor));
        }
        else
        {
            const float interpolation_factor = map_value(limb_component->collision_time,
                0.0f,
                limb_component->max_collision_time,
                physical_animation_component->target_position_blend_factor,
                1.0f);
            // Event if blend_factor is close to 0 we need to consider physics_pose.
            limb_component->adjusted_pose.position = lerp(limb_component->target_pose.position,
                limb_component->physics_pose.position,
                interpolation_factor);

            limb_component->adjusted_pose.rotation = slerp(limb_component->target_pose.rotation,
                limb_component->physics_pose.rotation,
                interpolation_factor);
        }
	}

	bool DrivePoseSolver::should_process_sweep(uint8 limb_type) const
	{
        PhysicalAnimationLimbComponent::Type type = PhysicalAnimationLimbComponent::Type(limb_type);
        return type == PhysicalAnimationLimbComponent::Type::FOREARM ||
            type == PhysicalAnimationLimbComponent::Type::ARM ||
            type == PhysicalAnimationLimbComponent::Type::HAND;
    }
}