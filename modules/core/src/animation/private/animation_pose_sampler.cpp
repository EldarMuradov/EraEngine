#include "animation/animation_pose_sampler.h"

namespace era_engine::animation
{
	void AnimationPoseSampler::init(const Skeleton* _skeleton, std::shared_ptr<AnimationAssetClip> _animation)
	{
		ASSERT(_skeleton != nullptr);
		ASSERT(_animation != nullptr);

		skeleton = _skeleton;
		animation = _animation;

		if (skeleton == nullptr)
		{
			joint_mapping.clear();
		}
		else
		{
			const std::unordered_map<std::string, uint32>& mapping = animation->get_joint_mapping();

			joint_mapping.resize(mapping.size(), INVALID_JOINT);

			for (uint32 joint_index = 0; joint_index < skeleton->joints.size(); ++joint_index)
			{
				const std::string& joint_uuid = skeleton->joints[joint_index].name;

				auto iter = mapping.find(joint_uuid);
				if (iter != mapping.end())
				{
					joint_mapping[iter->second] = joint_index;
				}
			}
		}
	}

	void AnimationPoseSampler::reset()
	{
		skeleton = nullptr;
		animation.reset();
		joint_mapping.clear();
	}

	bool AnimationPoseSampler::sample_pose(float sample_time, SkeletonPose& target) const
	{
		return animation->sample_joints(sample_time, skeleton->get_default_pose(), target, joint_mapping);
	}

	void AnimationPoseSampler::sample_curves(float sample_time, std::vector<float>& target) const
	{
		target.resize(animation->get_curve_mapping().size());
		animation->sample_curves(sample_time, target);
	}

	void AnimationPoseSampler::set_joint_enabled(const std::string& joint_name, bool enabled)
	{
		ASSERT(skeleton != nullptr);
		ASSERT(animation != nullptr);
		const std::unordered_map<std::string, uint32>& clip_mapping = animation->get_joint_mapping();
		auto clip_iter = clip_mapping.find(joint_name);
		if (clip_iter != clip_mapping.end())
		{
			if (enabled)
			{
				joint_mapping[clip_iter->second] = skeleton->get_joint_index(joint_name);
			}
			else
			{
				joint_mapping[clip_iter->second] = INVALID_JOINT;
			}
		}
	}

	std::shared_ptr<AnimationAssetClip> AnimationPoseSampler::get_animation() const
	{
		return animation;
	}

	bool AnimationPoseSampler::is_valid() const
	{
		return animation != nullptr &&
			skeleton != nullptr;
	}

	float AnimationPoseSampler::get_duration() const
	{
		return animation->get_duration();
	}
}