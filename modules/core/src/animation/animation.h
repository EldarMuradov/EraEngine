// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"
#include "core/memory.h"

#include "dx/dx_buffer.h"

#include "ai/state_machine.h"

#include "ecs/component.h"

#define INVALID_JOINT 0xFFFFFFFF

namespace era_engine
{
	struct multi_mesh;
	struct ldr_render_pass;
}

namespace era_engine::animation
{
	struct ERA_CORE_API SkinningWeights
	{
		uint8 skin_indices[4];
		uint8 skin_weights[4];
	};

	enum LimbType
	{
		limb_type_none,

		limb_type_torso,
		limb_type_head,

		limb_type_upper_arm_right,
		limb_type_lower_arm_right,
		limb_type_hand_right,

		limb_type_upper_arm_left,
		limb_type_lower_arm_left,
		limb_type_hand_left,

		limb_type_upper_leg_right,
		limb_type_lower_leg_right,
		limb_type_foot_right,

		limb_type_upper_leg_left,
		limb_type_lower_leg_left,
		limb_type_foot_left,

		limb_type_count,
	};

	extern const char* limb_type_names[limb_type_count];
	extern const vec3 limb_type_colors[limb_type_count];

	struct ERA_CORE_API SkeletonJoint
	{
		std::string name;
		LimbType limb_type;
		bool ik;

		mat4 inv_bind_transform; // Transforms from model space to joint space.
		mat4 bind_transform;	  // Position of joint relative to model space.

		trs transform = trs::identity;

		uint32 parent_id;
	};

	struct ERA_CORE_API AnimationJoint
	{
		bool is_animated = false;

		uint32 first_position_keyframe;
		uint32 num_position_keyframes;

		uint32 first_rotation_keyframe;
		uint32 num_rotation_keyframes;

		uint32 first_scale_keyframe;
		uint32 num_scale_keyframes;
	};

	struct ERA_CORE_API AnimationClip
	{
		void edit();
		trs get_first_root_transform() const;
		trs get_last_root_transform() const;

		std::string name;
		fs::path filename;

		std::vector<float> position_timestamps;
		std::vector<float> rotation_timestamps;
		std::vector<float> scale_timestamps;

		std::vector<vec3> position_keyframes;
		std::vector<quat> rotation_keyframes;
		std::vector<vec3> scale_keyframes;

		std::vector<AnimationJoint> joints;

		AnimationJoint root_motion_joint;

		float length_in_seconds;
		bool looping = true;
		bool bake_root_rotation_into_pose = false;
		bool bake_root_xz_translation_into_pose = false;
		bool bake_root_y_translation_into_pose = false;
	};

	struct ERA_CORE_API LimbDimensions
	{
		float minY, maxY;
		float radius;
		float xOffset, zOffset;
	};

	struct ERA_CORE_API SkeletonLimb
	{
		uint32 representative_joint = INVALID_JOINT;
		LimbDimensions dimensions;
	};

	class ERA_CORE_API Skeleton
	{
	public:
		Skeleton() = default;

		void analyze_joints(const vec3* positions, const void* others, uint32 other_stride, uint32 num_vertices);

		void blend_local_transforms(const trs* local_transforms1, const trs* local_transforms2, float t, trs* out_blended_local_transforms) const;
		void get_skinning_matrices_from_local_transforms(mat4* out_skinning_matrices, const trs& world_transform = trs::identity) const;
		void get_skinning_matrices_from_local_transforms(trs* out_global_transforms, mat4* out_skinning_matrices, const trs& world_transform = trs::identity) const;
		void get_skinning_matrices_from_global_transforms(const trs* global_transforms, mat4* out_skinning_matrices) const;
		void get_skinning_matrices_from_global_transforms(const trs* global_transforms, mat4* out_skinning_matrices, const trs& world_transform) const;

		void pretty_print_hierarchy() const;

	public:
		std::vector<SkeletonJoint> joints;
		std::unordered_map<std::string, uint32> name_to_joint_id;
		SkeletonLimb limbs[limb_type_count];
	};

	struct ERA_CORE_API AnimationSkeleton
	{
		void sampleAnimation(const AnimationClip& clip, float time, trs* outRootMotion = 0) const;
		void sampleAnimation(uint32 index, float time, trs* outRootMotion = 0) const;

		std::vector<uint32> getClipsByName(const std::string& name);

		std::vector<AnimationClip> clips;
		std::vector<fs::path> files;

		Skeleton* skeleton = nullptr;
	};

	class ERA_CORE_API SkeletonComponent : public Component
	{
	public:
		SkeletonComponent(ref<Entity::EcsData> _data);
		virtual ~SkeletonComponent();

		ERA_VIRTUAL_REFLECT(Component)

	public:
		Skeleton* skeleton = nullptr;
	};

	struct ERA_CORE_API AnimationInstance
	{
		AnimationInstance() { }
		AnimationInstance(const AnimationClip* clip, float startTime = 0.0f);

		void set(const AnimationClip* clip, float startTime = 0.0f);
		void update(const AnimationSkeleton& skeleton, float dt, trs& outDeltaRootMotion);

		bool valid() const { return clip != nullptr; }

		const AnimationClip* clip = nullptr;
		float time = 0.0f;

		trs lastRootMotion;

		bool paused = false;
		bool finished = false;
	};

	struct ERA_CORE_API AnimationBlackboard
	{
		AnimationClip* clip = nullptr;
	};

	struct AnimationState : era_engine::ai::StateBase<AnimationBlackboard>
	{
		AnimationState(ref<AnimationInstance> inst) : instance(inst)
		{
		}

		virtual void enter(AnimationBlackboard& state) override
		{
			instance->set(state.clip);
			instance->finished = false;
		}

		virtual void exit(AnimationBlackboard& state) override
		{
			if (instance->clip == state.clip)
				instance->clip = nullptr;
		}

		virtual void pause(AnimationBlackboard& state) override
		{
			instance->paused = true;
		}

		virtual void resume(AnimationBlackboard& state) override
		{
			instance->paused = false;
		}

	private:
		ref<AnimationInstance> instance;
	};

	struct ERA_CORE_API AnimationStateMachine
	{
		void set_state(ref<AnimationState> state, AnimationBlackboard& blackboard);

		void enter(AnimationBlackboard& blackboard);
		
		void pause(AnimationBlackboard& blackboard);

		void resume(AnimationBlackboard& blackboard);

		void update(AnimationBlackboard& blackboard);

		void update();

	private:
		ref<AnimationState> currentState = nullptr;
		std::stack<AnimationBlackboard> input;
		bool paused = false;
	};

	struct ERA_CORE_API AnimationController
	{
		AnimationStateMachine state_machine;
	};

	struct ERA_CORE_API AnimationBlendTree1d
	{
		AnimationBlendTree1d() = default;
		AnimationBlendTree1d(std::initializer_list<AnimationClip*> clips, float startRelTime = 0.0f, float startBlendValue = 0.0f);

		void setBlendValue(float blendValue);
		void update(const AnimationSkeleton& skeleton, float dt, trs* outLocalTransforms, trs& outDeltaRootMotion);

	private:
		AnimationClip* clips[8];
		uint32 numClips = 0;

		uint32 first;
		uint32 second;
		float value;
		float relTime;
		float blendValue;

		trs lastRootMotion;
	};

	class ERA_CORE_API AnimationComponent : public Component
	{
	public:
		AnimationComponent(ref<Entity::EcsData> _data);
		virtual ~AnimationComponent();

		void initialize(std::vector<AnimationClip>& clips, size_t start_index = 0);
		void draw_current_skeleton(const ref<multi_mesh>& mesh, const trs& transform, ldr_render_pass* render_pass) const;

		ERA_VIRTUAL_REFLECT(Component)
	public:
		ref<AnimationInstance> animation = nullptr;

		dx_vertex_buffer_group_view current_vertex_buffer;
		dx_vertex_buffer_group_view prev_frame_vertex_buffer;

		trs* current_global_transforms = 0;

		float time_scale = 1.f;
		bool draw_sceleton = true;
	};
}