#pragma once

#include "physics_api.h"

#include <core/math.h>
#include <ecs/entity.h>

namespace era_engine::animation
{
    class SkeletonComponent;
    class SkeletonPose;
}

namespace era_engine::physics
{
    class PhysicalAnimationComponent;

	class ERA_PHYSICS_API DrivePoseSampler
	{
	public:
        DrivePoseSampler();

        void sample_pose(PhysicalAnimationComponent* physical_animation_component,
            animation::SkeletonComponent* skeleton_to_update,
            animation::SkeletonPose& pose) const;

	private:
		void blend_with_prev_physics_pose(const trs& prev_joint_transform,
			float blend_value,
			trs& out_transform) const;

		void blend_with_animation_pose(const trs& animation_limb_local_transform,
			float blend_value,
			trs& out_transform) const;

		trs sample_limb(Entity limb,
			uint32 limb_id,
			PhysicalAnimationComponent* physical_animation_component,
			const trs& limb_animation_transform,
			const trs& inverse_ragdoll_transform,
			const trs& inverse_parent_local_transform,
			const trs& parent_local_transform) const;
	};
}