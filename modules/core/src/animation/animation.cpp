// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "animation/animation.h"
#include "animation/skinning.h"

#include "core/memory.h"
#include "core/random.h"
#include "core/imgui.h"
#include "core/string.h"

#include "geometry/mesh.h"

#include "dx/dx_context.h"

#include "rendering/debug_visualization.h"

#include <rttr/registration>

#include <algorithm>

namespace era_engine::animation
{
	static void scale_keyframes(AnimationClip& clip, AnimationJoint& joint, float scale)
	{
		for (uint32 keyID = 0; keyID < joint.num_position_keyframes; ++keyID)
		{
			clip.position_keyframes[joint.first_position_keyframe + keyID] *= scale;
		}
		for (uint32 keyID = 0; keyID < joint.num_scale_keyframes; ++keyID)
		{
			clip.scale_keyframes[joint.first_scale_keyframe + keyID] *= scale;
		}
	}

	static vec3 samplePosition(const AnimationClip& clip, const AnimationJoint& animJoint, float time)
	{
		if (time >= clip.length_in_seconds)
		{
			return clip.position_keyframes[animJoint.first_position_keyframe + animJoint.num_position_keyframes - 1];
		}

		if (animJoint.num_position_keyframes == 1)
		{
			return clip.position_keyframes[animJoint.first_position_keyframe];
		}

		uint32 firstKeyframeIndex = -1;
		for (uint32 i = 0; i < animJoint.num_position_keyframes - 1; ++i)
		{
			uint32 j = i + animJoint.first_position_keyframe;
			if (time < clip.position_timestamps[j + 1])
			{
				firstKeyframeIndex = j;
				break;
			}
		}
		ASSERT(firstKeyframeIndex != -1);

		uint32 secondKeyframeIndex = firstKeyframeIndex + 1;

		float t = inverse_lerp(clip.position_timestamps[firstKeyframeIndex], clip.position_timestamps[secondKeyframeIndex], time);

		vec3 a = clip.position_keyframes[firstKeyframeIndex];
		vec3 b = clip.position_keyframes[secondKeyframeIndex];

		return lerp(a, b, t);
	}

	static quat sampleRotation(const AnimationClip& clip, const AnimationJoint& animJoint, float time)
	{
		if (time >= clip.length_in_seconds)
		{
			return clip.rotation_keyframes[animJoint.first_rotation_keyframe + animJoint.num_rotation_keyframes - 1];
		}

		if (animJoint.num_rotation_keyframes == 1)
		{
			return clip.rotation_keyframes[animJoint.first_rotation_keyframe];
		}

		uint32 firstKeyframeIndex = -1;
		for (uint32 i = 0; i < animJoint.num_rotation_keyframes - 1; ++i)
		{
			uint32 j = i + animJoint.first_rotation_keyframe;
			if (time < clip.rotation_timestamps[j + 1])
			{
				firstKeyframeIndex = j;
				break;
			}
		}
		ASSERT(firstKeyframeIndex != -1);

		uint32 secondKeyframeIndex = firstKeyframeIndex + 1;

		float t = inverse_lerp(clip.rotation_timestamps[firstKeyframeIndex], clip.rotation_timestamps[secondKeyframeIndex], time);

		quat a = clip.rotation_keyframes[firstKeyframeIndex];
		quat b = clip.rotation_keyframes[secondKeyframeIndex];

		if (dot(a.v4, b.v4) < 0.f)
		{
			b.v4 *= -1.f;
		}

		return lerp(a, b, t);
	}

	static vec3 sampleScale(const AnimationClip& clip, const AnimationJoint& animJoint, float time)
	{
		if (time >= clip.length_in_seconds)
			return clip.scale_keyframes[animJoint.first_scale_keyframe + animJoint.num_scale_keyframes - 1];

		if (animJoint.num_scale_keyframes == 1)
			return clip.scale_keyframes[animJoint.first_scale_keyframe];

		if (!clip.scale_timestamps.size())
			return vec3(1.0f);

		uint32 firstKeyframeIndex = -1;
		for (uint32 i = 0; i < animJoint.num_scale_keyframes - 1; ++i)
		{
			uint32 j = i + animJoint.first_scale_keyframe;

			if (time < clip.scale_timestamps[j + 1])
			{
				firstKeyframeIndex = j;
				break;
			}
		}
		ASSERT(firstKeyframeIndex != -1);

		uint32 secondKeyframeIndex = firstKeyframeIndex + 1;

		float t = inverse_lerp(clip.scale_timestamps[firstKeyframeIndex], clip.scale_timestamps[secondKeyframeIndex], time);

		vec3 a = clip.scale_keyframes[firstKeyframeIndex];
		vec3 b = clip.scale_keyframes[secondKeyframeIndex];

		return lerp(a, b, t);
	}

	SkeletonPose AnimationSkeleton::sampleAnimation(const AnimationClip& clip, float time, trs* outRootMotion) const
	{
		ASSERT(skeleton != nullptr);
		ASSERT(clip.joints.size() == skeleton->joints.size());

		SkeletonPose result_pose = SkeletonPose(skeleton->joints.size());

		time = clamp(time, 0.f, clip.length_in_seconds);

		uint32 numJoints = (uint32)skeleton->joints.size();
		for (uint32 i = 0; i < numJoints; ++i)
		{
			const AnimationJoint& animJoint = clip.joints[i];
			JointTransform joint_transform;

			if (animJoint.is_animated)
			{
				trs transform;
				transform.position = samplePosition(clip, animJoint, time);
				transform.rotation = sampleRotation(clip, animJoint, time);
				transform.scale = sampleScale(clip, animJoint, time);

				joint_transform.set_transform(transform);
			}
			else
			{
				joint_transform.set_transform(trs::identity);
			}

			result_pose.set_joint_transform(joint_transform, i);
		}

		trs rootMotion;
		if (clip.root_motion_joint.is_animated)
		{
			rootMotion.position = samplePosition(clip, clip.root_motion_joint, time);
			rootMotion.rotation = sampleRotation(clip, clip.root_motion_joint, time);
			rootMotion.scale = sampleScale(clip, clip.root_motion_joint, time);
		}
		else
		{
			rootMotion = trs::identity;
		}

		if (outRootMotion)
		{
			if (clip.bake_root_rotation_into_pose)
			{
				result_pose.set_joint_transform(JointTransform(trs(0.f, rootMotion.rotation) * result_pose.get_joint_transform(0).get_transform()), 0);
				rootMotion.rotation = quat::identity;
			}

			if (clip.bake_root_xz_translation_into_pose)
			{
				result_pose.set_joint_translation(result_pose.get_joint_translation(0)
					+ vec3(rootMotion.position.x, 0.0f, 0.0f) + vec3(0.0f, 0.0f, rootMotion.position.z), 0);
				rootMotion.position.x = 0.f;
				rootMotion.position.z = 0.f;
			}

			if (clip.bake_root_y_translation_into_pose)
			{
				result_pose.set_joint_translation(result_pose.get_joint_translation(0)
					+ vec3(rootMotion.position.x, 0.0f, 0.0f) + vec3(0.0f, rootMotion.position.y, 0.0f), 0);
				rootMotion.position.y = 0.f;
			}

			*outRootMotion = rootMotion;
		}
		else
		{
			result_pose.set_joint_transform(JointTransform(rootMotion * result_pose.get_joint_transform(0).get_transform()), 0);
		}

		if (clip.is_unreal_asset)
		{
			result_pose.set_joint_rotation(result_pose.get_joint_rotation(0) * euler_to_quat(vec3(0.0f, -M_PI / 2.0f, 0.0f)), 0);
		}

		return result_pose;
	}

	SkeletonPose AnimationSkeleton::sampleAnimation(uint32 index, float time, trs* outRootMotion) const
	{
		return sampleAnimation(clips[index], time, outRootMotion);
	}

	std::vector<uint32> AnimationSkeleton::getClipsByName(const std::string& name)
	{
		std::vector<uint32> result;
		for (uint32 i = 0; i < (uint32)clips.size(); ++i)
		{
			if (clips[i].name == name)
			{
				result.push_back(i);
			}
		}
		return result;
	}

	static void pretty_print(const Skeleton& skeleton, uint32 parent, uint32 indent)
	{
		for (uint32 i = 0; i < (uint32)skeleton.joints.size(); ++i)
		{
			if (skeleton.joints[i].parent_id == parent)
			{
				std::cout << std::string(indent, ' ') << skeleton.joints[i].name << '\n';
				pretty_print(skeleton, i, indent + 1);
			}
		}
	}

	void AnimationClip::edit()
	{
		ImGui::Checkbox("Bake root rotation into pose", &bake_root_rotation_into_pose);
		ImGui::Checkbox("Bake xz translation into pose", &bake_root_xz_translation_into_pose);
		ImGui::Checkbox("Bake y translation into pose", &bake_root_y_translation_into_pose);
	}

	trs AnimationClip::get_first_root_transform() const
	{
		if (root_motion_joint.is_animated)
		{
			trs t;
			t.position = position_keyframes[root_motion_joint.first_position_keyframe];
			t.rotation = rotation_keyframes[root_motion_joint.first_rotation_keyframe];
			t.scale = scale_keyframes[root_motion_joint.first_scale_keyframe];

			if (bake_root_rotation_into_pose)
			{
				t.rotation = quat::identity;
			}
			if (bake_root_xz_translation_into_pose)
			{
				t.position.x = 0.f;
				t.position.z = 0.f;
			}
			if (bake_root_y_translation_into_pose)
			{
				t.position.y = 0.f;
			}

			return t;
		}
		return trs::identity;
	}

	trs AnimationClip::get_last_root_transform() const
	{
		if (root_motion_joint.is_animated)
		{
			trs t;
			t.position = position_keyframes[root_motion_joint.first_position_keyframe + root_motion_joint.num_position_keyframes - 1];
			t.rotation = rotation_keyframes[root_motion_joint.first_rotation_keyframe + root_motion_joint.num_rotation_keyframes - 1];
			t.scale = scale_keyframes[root_motion_joint.first_scale_keyframe + root_motion_joint.num_scale_keyframes - 1];

			if (bake_root_rotation_into_pose)
			{
				t.rotation = quat::identity;
			}
			if (bake_root_xz_translation_into_pose)
			{
				t.position.x = 0.f;
				t.position.z = 0.f;
			}
			if (bake_root_y_translation_into_pose)
			{
				t.position.y = 0.f;
			}

			return t;
		}
		return trs::identity;
	}

	AnimationInstance::AnimationInstance(const AnimationClip* clip, float startTime)
	{
		set(clip, startTime);
	}

	void AnimationInstance::set(const AnimationClip* clip, float startTime)
	{
		this->clip = clip;
		time = startTime;
		lastRootMotion = clip->get_first_root_transform();
	}

	SkeletonPose AnimationInstance::update(const AnimationSkeleton& skeleton, float dt, trs& outDeltaRootMotion)
	{
		if (paused)
		{
			return SkeletonPose();
		}

		if (valid())
		{
			time += dt;
			if (time >= clip->length_in_seconds)
			{
				if (clip->looping)
				{
					time = fmod(time, clip->length_in_seconds);
					lastRootMotion = clip->get_first_root_transform();
				}
				else
				{
					time = clip->length_in_seconds;
					finished = true;
				}
			}

			trs rootMotion;
			SkeletonPose result_pose = skeleton.sampleAnimation(*clip, time, &rootMotion);

			outDeltaRootMotion = invert(lastRootMotion) * rootMotion;
			lastRootMotion = rootMotion;
			return result_pose;
		}
		return SkeletonPose();
	}

#if 0
	AnimationBlendTree1d::AnimationBlendTree1d(std::initializer_list<AnimationClip*> clips, float startBlendValue, float startRelTime)
	{
		ASSERT(clips.size() <= arraysize(this->clips));
		ASSERT(clips.size() > 1);

		numClips = 0;
		for (AnimationClip* clip : clips)
		{
			this->clips[numClips++] = clip;
		}

		first = 0;
		second = 1;
		relTime = startRelTime;

		setBlendValue(startBlendValue);

		lastRootMotion = lerp(this->clips[first]->get_first_root_transform(), this->clips[second]->get_first_root_transform(), blendValue);
	}

	void AnimationBlendTree1d::update(const AnimationSkeleton& skeleton, float dt, trs* outLocalTransforms, trs& outDeltaRootMotion)
	{
		const AnimationClip* first = clips[this->first];
		const AnimationClip* second = clips[this->second];

		float firstLength = first->length_in_seconds;
		float secondLength = second->length_in_seconds;

		float lengthDist = secondLength - firstLength;
		float targetLength = firstLength + lengthDist * blendValue;

		float start = relTime;
		relTime += dt / targetLength;
		relTime = fmodf(relTime, 1.f);

		trs* totalLocalTransforms = (trs*)alloca(sizeof(trs) * skeleton.skeleton->joints.size() * 2);
		trs* localTransforms1 = totalLocalTransforms;
		trs* localTransforms2 = totalLocalTransforms + skeleton.skeleton->joints.size();

		trs rootMotion1, rootMotion2;
		skeleton.sampleAnimation(*first, first->length_in_seconds * relTime, &rootMotion1);
		skeleton.sampleAnimation(*second, second->length_in_seconds * relTime, &rootMotion2);

		skeleton.skeleton->blend_local_transforms(localTransforms1, localTransforms2, blendValue, outLocalTransforms);

		trs rootMotion = lerp(rootMotion1, rootMotion2, blendValue);

		outDeltaRootMotion = invert(lastRootMotion) * rootMotion;
		lastRootMotion = rootMotion;
	}

	void AnimationBlendTree1d::setBlendValue(float value)
	{
		value = clamp01(value);

		float step = 1.f / (numClips - 1);

		uint32 newFirstAnimationIndex = numClips - 2, newSecondAnimationIndex = numClips - 1;
		if (value < 1.f)
		{
			newFirstAnimationIndex = (uint32)(value / step);
			newSecondAnimationIndex = newFirstAnimationIndex + 1;
		}

		first = newFirstAnimationIndex;
		second = newSecondAnimationIndex;

		float begin = newFirstAnimationIndex * step;
		this->blendValue = (value - begin) / step;
		this->value = value;
	}
#endif

	const char* limb_type_names[] =
	{
		"Unknown",

		"Torso",
		"Head",

		"Upper arm right",
		"Lower arm right",
		"Hand right",

		"Upper arm left",
		"Lower arm left",
		"Hand left",

		"Upper leg right",
		"Lower leg right",
		"Foot right",

		"Upper leg left",
		"Lower leg left",
		"Foot left",
	};

	const vec3 limb_type_colors[] =
	{
		vec3(1.f, 0.f, 1.f),

		vec3(1.f, 0.f, 0.f),
		vec3(0.f, 1.f, 0.f),

		vec3(1.f, 1.f, 0.f),
		vec3(0.f, 1.f, 1.f),
		vec3(1.f, 1.f, 1.f),

		vec3(1.f, 1.f, 0.f),
		vec3(0.f, 1.f, 1.f),
		vec3(1.f, 1.f, 1.f),

		vec3(1.f, 1.f, 0.f),
		vec3(0.f, 1.f, 1.f),
		vec3(1.f, 1.f, 1.f),

		vec3(1.f, 1.f, 0.f),
		vec3(0.f, 1.f, 1.f),
		vec3(1.f, 1.f, 1.f),
	};

	void AnimationStateMachine::set_state(ref<AnimationState> state, AnimationBlackboard& blackboard)
	{
		if (currentState)
		{
			currentState->exit(blackboard);
		}

		currentState = state;

		if (currentState)
		{
			currentState->enter(blackboard);

			if (paused)
			{
				currentState->pause(blackboard);
			}
		}
	}

	void AnimationStateMachine::enter(AnimationBlackboard& blackboard)
	{
		if (currentState)
		{
			currentState->enter(blackboard);
			input.push(blackboard);
			if (paused)
			{
				currentState->pause(blackboard);
			}
		}
	}

	void AnimationStateMachine::pause(AnimationBlackboard& blackboard)
	{
		paused = true;

		if (currentState)
		{
			currentState->pause(blackboard);
		}
	}

	void AnimationStateMachine::resume(AnimationBlackboard& blackboard)
	{
		paused = false;

		if (currentState)
			currentState->resume(blackboard);
	}

	void AnimationStateMachine::update(AnimationBlackboard& blackboard)
	{
		if (paused)
			return;

		if (currentState)
		{
			input.push(blackboard);
			currentState->update(blackboard);
		}
	}

	void AnimationStateMachine::update()
	{
		if (paused)
			return;

		if (currentState && input.size())
		{
			currentState->enter(input.top());
			input.pop();
		}
	}

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<AnimationComponent>("AnimationComponent")
			.constructor<ref<Entity::EcsData>>();
		registration::class_<SkeletonComponent>("SkeletonComponent")
			.constructor<ref<Entity::EcsData>>();
	}

	JointTransform::JointTransform(const trs& initial_transform)
	{
		local_transform = initial_transform;
	}

	void JointTransform::set_translation(const vec3& new_translation)
	{
		local_transform.position = new_translation;
	}

	const vec3& JointTransform::get_translation() const
	{
		return local_transform.position;
	}

	void JointTransform::set_rotation(const quat& new_rotation)
	{
		local_transform.rotation = new_rotation;
	}

	const quat& JointTransform::get_rotation() const
	{
		return local_transform.rotation;
	}

	void JointTransform::set_transform(const trs& new_transform)
	{
		local_transform = new_transform;
	}

	const trs& JointTransform::get_transform() const
	{
		return local_transform;
	}

	SkeletonPose::SkeletonPose(uint32 joints_size)
	{
		local_transforms.resize(joints_size);
	}

	bool SkeletonPose::is_valid() const
	{
		return local_transforms.size() > 0;
	}

	uint32 SkeletonPose::size() const
	{
		return static_cast<uint32>(local_transforms.size());
	}

	void SkeletonPose::set_joint_transform(const JointTransform& new_joint_transform, uint32 joint_id)
	{
		local_transforms[joint_id] = new_joint_transform;
	}

	void SkeletonPose::set_joint_translation(const vec3& new_translation, uint32 joint_id)
	{
		local_transforms[joint_id].set_translation(new_translation);
	}

	void SkeletonPose::set_joint_rotation(const quat& new_rotaiton, uint32 joint_id)
	{
		local_transforms[joint_id].set_rotation(new_rotaiton);
	}

	const vec3& SkeletonPose::get_joint_translation(uint32 joint_id) const
	{
		return local_transforms[joint_id].get_translation();
	}

	const JointTransform& SkeletonPose::get_joint_transform(uint32 joint_id) const
	{
		return local_transforms[joint_id];
	}

	const quat& SkeletonPose::get_joint_rotation(uint32 joint_id) const
	{
		return local_transforms[joint_id].get_rotation();
	}

	trs SkeletonUtils::get_object_space_joint_transform(const Skeleton* skeleton, uint32 joint_id)
	{
		if (joint_id >= skeleton->local_transforms.size()) 
		{
			return trs::identity;
		}

		trs result = skeleton->local_transforms[joint_id].get_transform();
		uint32 parent_id = skeleton->joints[joint_id].parent_id;

		while (parent_id != INVALID_JOINT && parent_id < skeleton->joints.size())
		{
			result = skeleton->local_transforms[parent_id].get_transform() * result;
			parent_id = skeleton->joints[parent_id].parent_id;
		}

		return result;
	}

	trs SkeletonUtils::get_object_space_joint_transform(const SkeletonPose& pose, const Skeleton* skeleton, uint32 joint_id)
	{
		if (joint_id >= pose.size())
		{
			return trs::identity;
		}

		trs result = pose.get_joint_transform(joint_id).get_transform();
		uint32 parent_id = skeleton->joints[joint_id].parent_id;

		while (parent_id != INVALID_JOINT && parent_id < skeleton->joints.size())
		{
			result = pose.get_joint_transform(parent_id).get_transform() * result;
			parent_id = skeleton->joints[parent_id].parent_id;
		}

		return result;
	}

	AnimationComponent::AnimationComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	AnimationComponent::~AnimationComponent()
	{
	}

	void AnimationComponent::initialize(std::vector<AnimationClip>& clips, size_t start_index)
	{
		animation = make_ref<AnimationInstance>();
		ref<AnimationState> state = make_ref<AnimationState>(animation);

		if (clips.empty())
		{
			return;
		}

		animation->set(&clips[clips.size() - 1]);
	}

	void AnimationComponent::draw_current_skeleton(const ref<multi_mesh>& mesh, const trs& transform, ldr_render_pass* render_pass) const
	{
		const dx_mesh& dxMesh = mesh->mesh;
		Skeleton& skeleton = mesh->skeleton;
		AnimationSkeleton& animation_skeleton = mesh->animation_skeleton;

#if 1
		uint32 numJoints = (uint32)skeleton.joints.size();

		auto [vb, vertexPtr] = dxContext.createDynamicVertexBuffer(sizeof(position_color), numJoints * 2);
		auto [ib, indexPtr] = dxContext.createDynamicIndexBuffer(sizeof(uint16), numJoints * 2);

		position_color* vertices = (position_color*)vertexPtr;
		indexed_line16* lines = (indexed_line16*)indexPtr;

		for (uint32 i = 0; i < numJoints; ++i)
		{
			const auto& joint = skeleton.joints[i];
			if (joint.parent_id != INVALID_JOINT && !joint.ik)
			{
				const auto& parentJoint = skeleton.joints[joint.parent_id];
				if (current_global_transforms)
				{
					*vertices++ = { current_global_transforms[joint.parent_id].position, limb_type_colors[parentJoint.limb_type] };
					*vertices++ = { current_global_transforms[i].position, limb_type_colors[parentJoint.limb_type] };
				}
				else
				{
#if ROW_MAJOR
					vec3 vrt1 = vec3(skeleton.joints[joint.parentID].bindTransform.m03,
						skeleton.joints[joint.parentID].bindTransform.m13,
						skeleton.joints[joint.parentID].bindTransform.m23);

					vec3 vrt2 = vec3(joint.bindTransform.m03,
						joint.bindTransform.m13,
						joint.bindTransform.m23);

					*vertices++ = { vrt1, limbTypeColors[parentJoint.limbType] };
					*vertices++ = { vrt2, limbTypeColors[parentJoint.limbType] };
#else
					*vertices++ = { skeleton.joints[joint.parent_id].bind_transform.col3.xyz, limb_type_colors[parentJoint.limb_type] };
					*vertices++ = { joint.bind_transform.col3.xyz, limb_type_colors[parentJoint.limb_type] };
#endif
				}
			}
			else
			{
				*vertices++ = { vec3(0.f, 0.f, 0.f), vec3(1.f, 1.f, 1.f) };
				*vertices++ = { vec3(0.f, 0.f, 0.f), vec3(1.f, 1.f, 1.f) };
			}

			*lines++ = { (uint16)(2 * i), (uint16)(2 * i + 1) };
		}

		renderDebug<debug_unlit_line_pipeline::position_color>(trs_to_mat4(transform), vb, ib, vec4(1.f, 1.f, 1.f, 1.f), render_pass, true);

#else
		for (uint32 i = 0; i < limb_type_count; ++i)
		{
			const auto& limb = skeleton.limbs[i];

			if (limb.representativeJoint != INVALID_JOINT)
			{
				const auto& joint = skeleton.joints[limb.representativeJoint];
				vec3 a = vec3(limb.dimensions.xOffset, limb.dimensions.minY, limb.dimensions.zOffset);
				vec3 b = vec3(limb.dimensions.xOffset, limb.dimensions.maxY, limb.dimensions.zOffset);
				a = transformPosition(joint.bindTransform, a);
				b = transformPosition(joint.bindTransform, b);
				a = transformPosition(transform, a);
				b = transformPosition(transform, b);

				renderWireCapsule(a, b, limb.dimensions.radius * transform.scale.x, vec4(limbTypeColors[i], 1.f), renderPass, true);
			}
		}
#endif
	}


	void Skeleton::analyze_joints(const vec3* positions, const void* others, uint32 other_stride, uint32 num_vertices)
	{
		for (uint32 jointID = 0; jointID < (uint32)joints.size(); ++jointID)
		{
			SkeletonJoint& j = joints[jointID];

			std::string name = j.name;
			std::transform(name.begin(), name.end(), name.begin(), ::tolower);

			LimbType c = limb_type_none;

			bool left = contains(name, "left") || ends_with(name, ".l") || ends_with(name, "_l");

			if (contains(name, "spine") || contains(name, "hip") || contains(name, "rib") || contains(name, "pelvis") || contains(name, "shoulder") || contains(name, "clavicle")) { c = limb_type_torso; }
			else if (contains(name, "head") || contains(name, "neck")) { c = limb_type_head; }
			else if (contains(name, "arm"))
			{
				LimbType parentType = (j.parent_id != INVALID_JOINT) ? joints[j.parent_id].limb_type : limb_type_none;

				if (contains(name, "lower") || contains(name, "lo") || contains(name, "fore")) { c = left ? limb_type_lower_arm_left : limb_type_lower_arm_right; }
				else if (contains(name, "upper") || contains(name, "up")) { c = left ? limb_type_upper_arm_left : limb_type_upper_arm_right; }
				else if (parentType == limb_type_torso) { c = left ? limb_type_upper_arm_left : limb_type_upper_arm_right; }
				else { c = left ? limb_type_lower_arm_left : limb_type_lower_arm_right; }
			}
			else if (contains(name, "hand") || contains(name, "finger") || contains(name, "thumb") || contains(name, "index") || contains(name, "middle") || contains(name, "ring") || contains(name, "pinky"))
			{
				c = left ? limb_type_hand_left : limb_type_hand_right;
			}
			else if (contains(name, "leg") || contains(name, "thigh") || contains(name, "shin") || contains(name, "calf"))
			{
				LimbType parentType = (j.parent_id != INVALID_JOINT) ? joints[j.parent_id].limb_type : limb_type_none;

				if (contains(name, "lower") || contains(name, "lo") || contains(name, "shin") || contains(name, "calf")) { c = left ? limb_type_lower_leg_left : limb_type_lower_leg_right; }
				else if (contains(name, "upper") || contains(name, "up") || contains(name, "thigh")) { c = left ? limb_type_upper_leg_left : limb_type_upper_leg_right; }
				else if (parentType == limb_type_torso) { c = left ? limb_type_upper_leg_left : limb_type_upper_leg_right; }
				else { c = left ? limb_type_lower_leg_left : limb_type_lower_leg_right; }
			}
			else if (contains(name, "foot") || contains(name, "toe") || contains(name, "ball"))
			{
				c = left ? limb_type_foot_left : limb_type_foot_right;
			}

			j.limb_type = c;
			j.ik = contains(name, "ik");

			if (limbs[c].representative_joint == INVALID_JOINT)
			{
				// The highest joint in the hierarchy for this type is chosen as the representative.
				// Because we order the joints, the first joint we encounter is automatically the highest in the hierarchy.
				limbs[c].representative_joint = jointID;
			}
		}

#if 0

		struct limb_analysis
		{
			vec3 vertexMean;
			uint32 numVertices;
			mat3 covariance;
		};

		limb_analysis analysis[limb_type_count] = {};

		for (uint32 i = 0; i < numVertices; ++i)
		{
			vec3 p = positions[i];
			skinning_weights w = *(skinning_weights*)((uint8*)others + i * otherStride);
			for (uint32 j = 0; j < 4; ++j)
			{
				if (w.skinWeights[j] > 150)
				{
					const auto& joint = joints[w.skinIndices[j]];
					limb_type c = joint.limbType;
					if (c != limb_type_unknown)
					{
						limb_analysis& a = analysis[c];
						a.vertexMean += p;
						++a.numVertices;
					}
				}
			}
		}

		for (uint32 i = 0; i < limb_type_count; ++i)
		{
			if (analysis[i].numVertices)
			{
				analysis[i].vertexMean /= (float)analysis[i].numVertices;
			}
		}


		for (uint32 i = 0; i < numVertices; ++i)
		{
			vec3 p = positions[i];
			skinning_weights w = *(skinning_weights*)((uint8*)others + i * otherStride);
			for (uint32 j = 0; j < 4; ++j)
			{
				if (w.skinWeights[j] > 150)
				{
					const auto& joint = joints[w.skinIndices[j]];
					limb_type c = joint.limbType;
					if (c != limb_type_unknown)
					{
						limb_analysis& a = analysis[c];
						vec3 m = a.vertexMean;

						for (uint32 y = 0; y < 3; ++y)
						{
							for (uint32 x = 0; x < 3; ++x)
							{
								a.covariance.m[3 * y + x] += (m.data[y] - p.data[y]) * (m.data[x] - p.data[x]);
							}
						}
					}
				}
			}
		}

		for (uint32 i = 0; i < limb_type_count; ++i)
		{
			if (analysis[i].numVertices)
			{
				analysis[i].covariance *= 1.f / (float)analysis[i].numVertices;

				//singular_value_decomposition svd = computeSVD(analysis[i].covariance);
				//vec3 principalAxis = col(svd.U, 0);

				vec3 eigenValues;
				mat3 eigenVectors;

				getEigen(analysis[i].covariance, eigenValues, eigenVectors);
				uint32 maxComponent = (eigenValues.x > eigenValues.y) ? (eigenValues.x > eigenValues.z) ? 0 : 2 : (eigenValues.y > eigenValues.z) ? 1 : 2;

				vec3 principalAxis = col(eigenVectors, maxComponent);

				limbs[i].mean = analysis[i].vertexMean;
				limbs[i].principalAxis = principalAxis;
			}
		}

#else

		for (uint32 i = 0; i < limb_type_count; ++i)
		{
			limbs[i].dimensions = { FLT_MAX, -FLT_MAX, -FLT_MAX };
		}

		for (uint32 i = 0; i < num_vertices; ++i)
		{
			vec3 p = positions[i];
			SkinningWeights w = *(SkinningWeights*)((uint8*)others + i * other_stride);
			for (uint32 j = 0; j < 4; ++j)
			{
				if (w.skin_weights[j] > 200)
				{
					LimbType type = joints[w.skin_indices[j]].limb_type;
					if (type != limb_type_none)
					{
						if (limbs[type].representative_joint != INVALID_JOINT)
						{
							const SkeletonJoint& j = joints[limbs[type].representative_joint];
							LimbDimensions& d = limbs[type].dimensions;

							p = transform_position(j.inv_bind_transform, p);

							d.minY = min(d.minY, p.y);
							d.maxY = max(d.maxY, p.y);
							d.radius = max(d.radius, squared_length(vec2(p.x, p.z)));
						}
					}
				}
			}
		}

		for (uint32 i = 0; i < limb_type_count; ++i)
		{
			LimbDimensions& d = limbs[i].dimensions;

			d.radius = sqrt(d.radius); // Above, we calculate the squared radius.

			float c = 0.5f * (d.minY + d.maxY);

			const float scaleFactor = 0.8f;
			d.minY = (d.minY - c) * scaleFactor + c;
			d.maxY = (d.maxY - c) * scaleFactor + c;
			d.radius *= scaleFactor;

			d.minY += d.radius;
			d.maxY -= d.radius;
			if (d.minY > d.maxY)
			{
				d.minY = c - EPSILON;
				d.maxY = c + EPSILON;
			}
		}

#endif
	}

	void Skeleton::blend_local_transforms(const trs* local_transforms1, const trs* local_transforms2, float t, trs* out_blended_local_transforms) const
	{
		t = clamp01(t);
		for (uint32 jointID = 0; jointID < (uint32)joints.size(); ++jointID)
		{
			out_blended_local_transforms[jointID] = lerp(local_transforms1[jointID], local_transforms2[jointID], t);
		}
	}

	void Skeleton::get_skinning_matrices_from_local_transforms(mat4* out_skinning_matrices, const trs& world_transform) const
	{
		uint32 num_joints = (uint32)joints.size();
		trs* global_transforms = (trs*)alloca(sizeof(trs) * num_joints);

		for (uint32 i = 0; i < num_joints; ++i)
		{
			const SkeletonJoint& joint = joints[i];
			const JointTransform& joint_transform = local_transforms[i];
			if (joint.parent_id != INVALID_JOINT && joint.parent_id < joints.size())
			{
				ASSERT(i > joint.parent_id); // Parent already processed.
				global_transforms[i] = global_transforms[joint.parent_id] * joint_transform.get_transform();
			}
			else
			{
				global_transforms[i] = world_transform * joint_transform.get_transform();
			}

			out_skinning_matrices[i] = trs_to_mat4(global_transforms[i]) * joints[i].inv_bind_transform;
		}
	}

	void Skeleton::get_skinning_matrices_from_local_transforms(trs* out_global_transforms, mat4* out_skinning_matrices, const trs& world_transform) const
	{
		uint32 num_joints = (uint32)joints.size();

		for (uint32 i = 0; i < num_joints; ++i)
		{
			const SkeletonJoint& joint = joints[i];
			const JointTransform& joint_transform = local_transforms[i];

			if (joint.parent_id != INVALID_JOINT && joint.parent_id < joints.size())
			{
				ASSERT(i > joint.parent_id); // Parent already processed
				out_global_transforms[i] = out_global_transforms[joint.parent_id] * joint_transform.get_transform();
			}
			else
			{
				out_global_transforms[i] = world_transform * joint_transform.get_transform();
			}

			out_skinning_matrices[i] = trs_to_mat4(out_global_transforms[i]) * joints[i].inv_bind_transform;
		}
	}

	void Skeleton::get_skinning_matrices_from_global_transforms(const trs* global_transforms, mat4* out_skinning_matrices) const
	{
		uint32 num_joints = (uint32)joints.size();

		for (uint32 i = 0; i < num_joints; ++i)
		{
			out_skinning_matrices[i] = trs_to_mat4(global_transforms[i]) * joints[i].inv_bind_transform;
		}
	}

	void Skeleton::get_skinning_matrices_from_global_transforms(const trs* global_transforms, mat4* out_skinning_matrices, const trs& world_transform) const
	{
		uint32 num_joints = (uint32)joints.size();

		for (uint32 i = 0; i < num_joints; ++i)
		{
			out_skinning_matrices[i] = trs_to_mat4(world_transform) * trs_to_mat4(global_transforms[i]) * joints[i].inv_bind_transform;
		}
	}

	void Skeleton::pretty_print_hierarchy() const
	{
		pretty_print(*this, INVALID_JOINT, 0);
	}

	void Skeleton::set_joint_transform(const trs& new_transform, uint32 joint_id)
	{
		local_transforms[joint_id].set_transform(new_transform);
	}

	void Skeleton::set_joint_rotation(const quat& new_rotaiton, uint32 joint_id)
	{
		local_transforms[joint_id].set_rotation(new_rotaiton);
	}

	const trs& Skeleton::get_joint_transform(uint32 joint_id) const
	{
		return local_transforms[joint_id].get_transform();
	}

	const quat& Skeleton::get_joint_rotation(uint32 joint_id) const
	{
		return local_transforms[joint_id].get_rotation();
	}

	void Skeleton::apply_pose(const SkeletonPose& pose)
	{
		local_transforms = pose.local_transforms;
	}

	SkeletonComponent::SkeletonComponent(ref<Entity::EcsData> _data) 
		: Component(_data) 
	{
	}

	SkeletonComponent::~SkeletonComponent()
	{
	}
}