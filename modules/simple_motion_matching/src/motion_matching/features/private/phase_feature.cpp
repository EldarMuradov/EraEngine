#include "motion_matching/features/phase_feature.h"

#include <animation/animation_pose_sampler.h>
#include <animation/skeleton_component.h>
#include <animation/animation.h>

#include <asset/file_registry.h>

#include <core/log.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<PhaseFeatureDesc>("PhaseFeatureDesc")
			.constructor<>()(policy::ctor::as_raw_ptr);

		registration::class_<PhaseFeature>("PhaseFeature")
			.constructor<>()(policy::ctor::as_raw_ptr);
	}

	namespace
	{
		constexpr float MIN_STEP_CYCLE_DURATION = 0.2f;
		constexpr float MAX_STEP_CYCLE_DURATION = 2.0f;
		constexpr float DEFAULT_STEP_CYCLE_DURATION = 1.0f;

		constexpr float AMPLITUDE_WINDOW_HALF_SIZE = 0.5f;

		constexpr uint32 MIN_CONTACT_RUN_SAMPLES = 2;

		struct PhaseCurves
		{
			std::vector<float> x;
			std::vector<float> y;

			uint32 touchdowns_count = 0;
			float step_cycle_duration = DEFAULT_STEP_CYCLE_DURATION;
		};

		std::string get_x_curve_name(const FeatureDesc& desc)
		{
			return desc.name + "_x";
		}

		std::string get_y_curve_name(const FeatureDesc& desc)
		{
			return desc.name + "_y";
		}

		float wrap01(float value)
		{
			return value - floorf(value);
		}

		void remove_short_runs(std::vector<uint8>& values, uint32 min_run_length)
		{
			const uint32 count = uint32(values.size());

			uint32 run_start = 0;
			while (run_start < count)
			{
				uint32 run_end = run_start + 1;
				while (run_end < count && values[run_end] == values[run_start])
				{
					++run_end;
				}

				const bool is_interior_run = run_start > 0 && run_end < count;
				if (is_interior_run && run_end - run_start < min_run_length)
				{
					std::fill(values.begin() + run_start, values.begin() + run_end, values[run_start - 1]);
				}

				run_start = run_end;
			}
		}

		PhaseCurves compute_phase_curves(const std::vector<vec3>& positions, float timestep, const PhaseFeatureDesc& desc)
		{
			const uint32 count = uint32(positions.size());

			PhaseCurves result;
			result.x.resize(count, 0.0f);
			result.y.resize(count, 0.0f);

			if (count < 2)
			{
				return result;
			}

			std::vector<float> speeds(count);
			float min_height = std::numeric_limits<float>::max();
			for (uint32 i = 0; i < count; ++i)
			{
				const uint32 prev = i > 0 ? i - 1 : i;
				const uint32 next = i + 1 < count ? i + 1 : i;
				speeds[i] = length(positions[next] - positions[prev]) / (float(next - prev) * timestep);

				min_height = min(min_height, positions[i].y);
			}

			std::vector<uint8> contacts(count);
			for (uint32 i = 0; i < count; ++i)
			{
				contacts[i] = positions[i].y - min_height < desc.contact_height_threshold &&
					speeds[i] < desc.contact_speed_threshold;
			}
			remove_short_runs(contacts, MIN_CONTACT_RUN_SAMPLES);

			std::vector<uint32> touchdowns;
			for (uint32 i = 1; i < count; ++i)
			{
				if (contacts[i] && !contacts[i - 1])
				{
					touchdowns.push_back(i);
				}
			}

			auto is_valid_cycle = [timestep](uint32 from, uint32 to)
				{
					const float duration = float(to - from) * timestep;
					return duration >= MIN_STEP_CYCLE_DURATION && duration <= MAX_STEP_CYCLE_DURATION;
				};

			float valid_cycles_duration = 0.0f;
			uint32 valid_cycles_count = 0;
			for (size_t i = 1; i < touchdowns.size(); ++i)
			{
				if (is_valid_cycle(touchdowns[i - 1], touchdowns[i]))
				{
					valid_cycles_duration += float(touchdowns[i] - touchdowns[i - 1]) * timestep;
					++valid_cycles_count;
				}
			}

			result.touchdowns_count = uint32(touchdowns.size());
			if (valid_cycles_count > 0)
			{
				result.step_cycle_duration = valid_cycles_duration / float(valid_cycles_count);
			}

			std::vector<float> phases(count, 0.0f);
			size_t next_touchdown = 0;
			for (uint32 i = 0; i < count; ++i)
			{
				while (next_touchdown < touchdowns.size() && touchdowns[next_touchdown] <= i)
				{
					++next_touchdown;
				}

				const bool has_prev = next_touchdown > 0;
				const bool has_next = next_touchdown < touchdowns.size();
				const uint32 prev = has_prev ? touchdowns[next_touchdown - 1] : 0;
				const uint32 next = has_next ? touchdowns[next_touchdown] : 0;

				if (has_prev && has_next && is_valid_cycle(prev, next))
				{
					phases[i] = float(i - prev) / float(next - prev);
				}
				else if (has_prev && (!has_next || i - prev <= next - i))
				{
					phases[i] = wrap01(float(i - prev) * timestep / result.step_cycle_duration);
				}
				else if (has_next)
				{
					phases[i] = wrap01(1.0f - float(next - i) * timestep / result.step_cycle_duration);
				}
			}

			std::vector<float> speed_prefix_sums(count + 1, 0.0);
			for (uint32 i = 0; i < count; ++i)
			{
				speed_prefix_sums[i + 1] = speed_prefix_sums[i] + speeds[i];
			}

			const int32 window_half_size = int32(std::lrintf(AMPLITUDE_WINDOW_HALF_SIZE / timestep));
			for (uint32 i = 0; i < count; ++i)
			{
				const uint32 from = uint32(max(int32(i) - window_half_size, 0));
				const uint32 to = uint32(min(int32(i) + window_half_size + 1, int32(count)));
				const float mean_speed = (speed_prefix_sums[to] - speed_prefix_sums[from]) / (to - from);

				const float amplitude = saturate(mean_speed / max(desc.full_amplitude_speed, EPSILON));
				const float angle = M_TAU * phases[i];

				result.x[i] = amplitude * cosf(angle);
				result.y[i] = amplitude * sinf(angle);
			}

			return result;
		}
	}

	PhaseFeature::~PhaseFeature()
	{
	}

	uint32 PhaseFeature::get_feature_size() const
	{
		return uint32(descriptors.size()) * 2u;
	}

	std::vector<float> PhaseFeature::compute_features(const FeatureComputationContext& context) const
	{
		using namespace animation;

		std::vector<float> values;
		values.reserve(get_feature_size());

		const ref<AnimationAssetClip>& animation = context.animation_component->current_animation;
		const float current_time = context.animation_component->current_anim_position;

		for (const ref<FeatureDesc>& descriptor : descriptors)
		{
			ASSERT(std::dynamic_pointer_cast<PhaseFeatureDesc>(descriptor) != nullptr);

			float x_value = 0.0f;
			float y_value = 0.0f;
			if (animation != nullptr)
			{
				animation->sample_curve(current_time, get_x_curve_name(*descriptor), x_value);
				animation->sample_curve(current_time, get_y_curve_name(*descriptor), y_value);
			}

			values.emplace_back(x_value);
			values.emplace_back(y_value);
		}

		return values;
	}

	bool PhaseFeature::mark_animation(const animation::SkeletonComponent* skeleton_component, ref<animation::AnimationAssetClip> clip) const
	{
		using namespace animation;

		const Skeleton* skeleton = skeleton_component->skeleton.get();

		std::vector<ref<PhaseFeatureDesc>> phase_descriptors;
		phase_descriptors.reserve(descriptors.size());
		for (const ref<FeatureDesc>& descriptor : descriptors)
		{
			ref<PhaseFeatureDesc> phase_feature_desc = std::dynamic_pointer_cast<PhaseFeatureDesc>(descriptor);
			if (phase_feature_desc == nullptr)
			{
				ASSERT(false);
				return false;
			}
			phase_descriptors.emplace_back(phase_feature_desc);
		}

		AnimationPoseSampler sampler;
		sampler.init(skeleton, clip);

		const uint32 samples_per_track = clip->get_num_samples_per_track();
		const float sample_time = 1.0f / clip->get_sample_rate();

		std::vector<std::vector<vec3>> joint_positions(phase_descriptors.size());
		for (std::vector<vec3>& positions : joint_positions)
		{
			positions.reserve(samples_per_track);
		}

		for (uint32 i = 0; i < samples_per_track; ++i)
		{
			SkeletonPose pose = skeleton->get_default_pose();
			sampler.sample_pose(sample_time * static_cast<float>(i), pose);

			for (size_t desc_index = 0; desc_index < phase_descriptors.size(); ++desc_index)
			{
				const trs joint_transform = SkeletonUtils::get_object_space_joint_transform(pose, skeleton, phase_descriptors[desc_index]->joint_id);
				joint_positions[desc_index].emplace_back(joint_transform.position);
			}
		}

		const std::string clip_name = get_path_from_asset_handle(clip->handle).filename().string();

		for (size_t desc_index = 0; desc_index < phase_descriptors.size(); ++desc_index)
		{
			const PhaseFeatureDesc& desc = *phase_descriptors[desc_index];

			PhaseCurves curves = compute_phase_curves(joint_positions[desc_index], sample_time, desc);

			LOG_MESSAGE("Motion matching> Phase '%s' in '%s': %u touchdowns, step cycle %.2f s.",
				desc.name.c_str(), clip_name.c_str(), curves.touchdowns_count, curves.step_cycle_duration);

			const std::string x_curve_name = get_x_curve_name(desc);
			clip->remove_curve(x_curve_name);
			clip->add_curve(x_curve_name, curves.x);

			const std::string y_curve_name = get_y_curve_name(desc);
			clip->remove_curve(y_curve_name);
			clip->add_curve(y_curve_name, curves.y);
		}

		return true;
	}

	bool PhaseFeature::sample_animation(const animation::Skeleton* skeleton, ref<animation::AnimationAssetClip> clip, float sample_rate, std::vector<ref<MotionMatchingDatabase::Sample>>& out_samples) const
	{
		const uint32 num_samples = std::lrintf(sample_rate * clip->get_duration());

		const float timestep = 1.0f / sample_rate;
		float current_time = 0.0f;

		for (uint32 i = 0; i < num_samples; ++i)
		{
			ref<MotionMatchingDatabase::Sample>& sample = out_samples[i];

			for (const ref<FeatureDesc>& descriptor : descriptors)
			{
				ASSERT(std::dynamic_pointer_cast<PhaseFeatureDesc>(descriptor) != nullptr);

				float x_value = 0.0f;
				clip->sample_curve(current_time, get_x_curve_name(*descriptor), x_value);

				float y_value = 0.0f;
				clip->sample_curve(current_time, get_y_curve_name(*descriptor), y_value);

				sample->features.emplace_back(x_value);
				sample->features.emplace_back(y_value);
			}

			current_time += timestep;
		}

		return true;
	}
}
