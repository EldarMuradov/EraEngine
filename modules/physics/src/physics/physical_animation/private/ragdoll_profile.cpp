#include "physics/physical_animation/ragdoll_profile.h"

#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::enumeration<RagdollProfileType>("RagdollProfileType")
		(
			value("IDLE", RagdollProfileType::IDLE),
			value("RUNNING", RagdollProfileType::RUNNING),
			value("SPRINT", RagdollProfileType::SPRINT),
			value("CLIMBING", RagdollProfileType::CLIMBING),
			value("RAGDOLL", RagdollProfileType::RAGDOLL)
		);

		registration::enumeration<RagdollProfileStrengthType>("RagdollProfileStrengthType")
		(
			value("DEFAULT", RagdollProfileStrengthType::DEFAULT),
			value("SOFT", RagdollProfileStrengthType::SOFT),
			value("HARD", RagdollProfileStrengthType::HARD)
		);

		registration::enumeration<PhysicalLimbBlendType>("PhysicalLimbBlendType")
		(
			value("NONE", PhysicalLimbBlendType::NONE),
			value("BLEND_WITH_PREV_POSE", PhysicalLimbBlendType::BLEND_WITH_PREV_POSE),
			value("BLEND_WITH_ANIMATION_POSE", PhysicalLimbBlendType::BLEND_WITH_ANIMATION_POSE),
			value("PURE_PHYSICS", PhysicalLimbBlendType::PURE_PHYSICS),
			value("PURE_ANIMATION", PhysicalLimbBlendType::PURE_ANIMATION)
		);
	}

	const PhysicalLimbDetails& RagdollProfile::get_limb_details_by_type(RagdollLimbType limb_type) const
	{
		return const_cast<RagdollProfile*>(this)->get_limb_details_by_type(limb_type);
	}

	PhysicalLimbDetails& RagdollProfile::get_limb_details_by_type(RagdollLimbType limb_type)
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

		case RagdollLimbType::NECK:
			return neck_limb_details;

		case RagdollLimbType::CLAVICLE:
			return clavicle_limb_details;

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
		return body_lower_limb_details;
	}

	float RagdollProfile::get_strength(RagdollLimbType limb_type, RagdollProfileStrengthType strength_type) const
	{
		const LimbStrengthDetails& strength_details = get_limb_details_by_type(limb_type).strength_details;

		if (strength_type == RagdollProfileStrengthType::SOFT)
		{
			return strength_details.soft_strength_coeff;
		}
		else if (strength_type == RagdollProfileStrengthType::HARD)
		{
			return strength_details.hard_strength_coeff;
		}

		return strength_details.default_strength_coeff;
	}
}
