#include "physics/physical_animation/ragdoll_profile.h"

namespace era_engine::physics
{
	const PhysicalLimbDetails& RagdollProfile::get_limb_details_by_type(RagdollLimbType limb_type) const
	{
		switch (limb_type)
		{
		case RagdollLimbType::BODY_LOWER:
			return body_lower_limb_details;

		case RagdollLimbType::BODY_MIDDLE:
			return body_middle_limb_details;

		case RagdollLimbType::BODY_UPPER:
			return body_upper_limb_details;

		case RagdollLimbType::HEAD:
			return head_limb_details;

		case RagdollLimbType::ARM:
			return arm_limb_details;

		case RagdollLimbType::FOREARM:
			return forearm_limb_details;

		case RagdollLimbType::HAND:
			return hand_limb_details;

		case RagdollLimbType::LEG:
			return leg_limb_details;

		case RagdollLimbType::CALF:
			return calf_limb_details;

		case RagdollLimbType::FOOT:
			return foot_limb_details;
		}
	}
}