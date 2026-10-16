#pragma once

#include "core_api.h"

#include "core/math.h"
#include "core/serialization/binary_serializer.h"

#define INVALID_JOINT 0xFFFFFFFF

namespace era_engine::animation
{
	enum LimbType
	{
		LIMB_TYPE_NONE,

		LIMB_TYPE_TORSO,
		LIMB_TYPE_HEAD,

		LIMB_TYPE_UPPER_ARM_RIGHT,
		LIMB_TYPE_LOWER_ARM_RIGHT,
		LIMB_TYPE_HAND_RIGHT,

		LIMB_TYPE_UPPER_ARM_LEFT,
		LIMB_TYPE_LOWER_ARM_LEFT,
		LIMB_TYPE_HAND_LEFT,

		LIMB_TYPE_UPPER_LEG_RIGHT,
		LIMB_TYPE_LOWER_LEG_RIGHT,
		LIMB_TYPE_FOOT_RIGHT,

		LIMB_TYPE_UPPER_LEG_LEFT,
		LIMB_TYPE_LOWER_LEG_LEFT,
		LIMB_TYPE_FOOT_LEFT,

		LIMB_TYPE_COUNT,
	};

	struct ERA_CORE_API SkeletonJoint final
	{
		std::string name;
		LimbType limb_type;
		bool ik = false;

		mat4 inv_bind_transform; // Transforms from model space to joint space.
		mat4 bind_transform; // Position of joint relative to model space.

		uint32 parent_id = INVALID_JOINT;

		ERA_BINARY_SERIALIZE(name, limb_type, ik, inv_bind_transform, bind_transform, parent_id)
	};

	struct ERA_CORE_API SkinningWeights
	{
		SkinningWeights() = default;

		std::array<uint8, 4> skin_indices;
		std::array<uint8, 4> skin_weights;

		ERA_BINARY_SERIALIZE(skin_indices, skin_weights)
	};

	struct ERA_CORE_API SkeletonAssetImportData
	{
		std::vector<animation::SkeletonJoint> joints;
		std::unordered_map<std::string, uint32> name_to_joint_id;
	};
}