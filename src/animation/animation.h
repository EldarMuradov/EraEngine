// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"
#include "core/memory.h"

#include "dx/dx_buffer.h"

#include "ai/state_machine.h"

#define INVALID_JOINT 0xFFFFFFFF

namespace era_engine
{
	struct multi_mesh;
	struct ldr_render_pass;
}

namespace era_engine::animation
{
	struct skinning_weights
	{
		uint8 skinIndices[4];
		uint8 skinWeights[4];
	};

	enum limb_type
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

	extern const char* limbTypeNames[limb_type_count];
	extern const vec3 limbTypeColors[limb_type_count];

	struct skeleton_joint
	{
		std::string name;
		limb_type limbType;
		bool ik;

		mat4 invBindTransform; // Transforms from model space to joint space.
		mat4 bindTransform;	  // Position of joint relative to model space.
		uint32 parentID;
	};

	struct animation_joint
	{
		bool isAnimated = false;

		uint32 firstPositionKeyframe;
		uint32 numPositionKeyframes;

		uint32 firstRotationKeyframe;
		uint32 numRotationKeyframes;

		uint32 firstScaleKeyframe;
		uint32 numScaleKeyframes;
	};

	struct animation_clip
	{
		void edit();
		NODISCARD trs getFirstRootTransform() const;
		NODISCARD trs getLastRootTransform() const;

		std::string name;
		fs::path filename;

		std::vector<float> positionTimestamps;
		std::vector<float> rotationTimestamps;
		std::vector<float> scaleTimestamps;

		std::vector<vec3> positionKeyframes;
		std::vector<quat> rotationKeyframes;
		std::vector<vec3> scaleKeyframes;

		std::vector<animation_joint> joints;

		animation_joint rootMotionJoint;

		float lengthInSeconds;
		bool looping = true;
		bool bakeRootRotationIntoPose = false;
		bool bakeRootXZTranslationIntoPose = false;
		bool bakeRootYTranslationIntoPose = false;
	};

	struct limb_dimensions
	{
		float minY, maxY;
		float radius;
		float xOffset, zOffset;
	};

	struct skeleton_limb
	{
		uint32 representativeJoint = INVALID_JOINT;
		limb_dimensions dimensions;
	};

	struct animation_skeleton
	{
		void analyzeJoints(const vec3* positions, const void* others, uint32 otherStride, uint32 numVertices);

		void sampleAnimation(const animation_clip& clip, float time, trs* outLocalTransforms, trs* outRootMotion = 0) const;
		void sampleAnimation(uint32 index, float time, trs* outLocalTransforms, trs* outRootMotion = 0) const;
		void blendLocalTransforms(const trs* localTransforms1, const trs* localTransforms2, float t, trs* outBlendedLocalTransforms) const;
		void getSkinningMatricesFromLocalTransforms(const trs* localTransforms, mat4* outSkinningMatrices, const trs& worldTransform = trs::identity) const;
		void getSkinningMatricesFromLocalTransforms(const trs* localTransforms, trs* outGlobalTransforms, mat4* outSkinningMatrices, const trs& worldTransform = trs::identity) const;
		void getSkinningMatricesFromGlobalTransforms(const trs* globalTransforms, mat4* outSkinningMatrices) const;

		std::vector<uint32> getClipsByName(const std::string& name);

		void prettyPrintHierarchy() const;

		std::vector<skeleton_joint> joints;
		std::unordered_map<std::string, uint32> nameToJointID;

		std::vector<animation_clip> clips;
		std::vector<fs::path> files;

		skeleton_limb limbs[limb_type_count];
	};

	struct animation_instance
	{
		animation_instance() { }
		animation_instance(const animation_clip* clip, float startTime = 0.f);

		void set(const animation_clip* clip, float startTime = 0.f);
		void update(const animation_skeleton& skeleton, float dt, trs* outLocalTransforms, trs& outDeltaRootMotion);

		bool valid() const { return clip != 0; }

		const animation_clip* clip = 0;
		float time = 0.f;

		trs lastRootMotion;

		bool paused = false;
		bool finished = false;
	};

	struct animation_blackboard
	{
		animation_clip* clip = nullptr;
	};

	struct animation_state : era_engine::ai::state_base<animation_blackboard>
	{
		animation_state(ref<animation_instance> inst) noexcept : instance(inst)
		{
		}

		virtual void enter(animation_blackboard& state) override
		{
			instance->set(state.clip);
			instance->finished = false;
		}

		virtual void exit(animation_blackboard& state) override
		{
			if (instance->clip == state.clip)
				instance->clip = nullptr;
		}

		virtual void pause(animation_blackboard& state) override
		{
			instance->paused = true;
		}

		virtual void resume(animation_blackboard& state) override
		{
			instance->paused = false;
		}

	private:
		ref<animation_instance> instance;
	};

	struct animation_state_machine
	{
		void set_state(ref<animation_state> state, animation_blackboard& blackboard) noexcept;

		void enter(animation_blackboard& blackboard) noexcept;
		
		void pause(animation_blackboard& blackboard) noexcept;

		void resume(animation_blackboard& blackboard) noexcept;

		void update(animation_blackboard& blackboard) noexcept;

		void update() noexcept;

	private:
		ref<animation_state> currentState = nullptr;
		std::stack<animation_blackboard> input;
		bool paused = false;
	};

	struct animation_controller
	{
		animation_state_machine stateMachine;
	};

	struct animation_blend_tree_1d
	{
		animation_blend_tree_1d() { }
		animation_blend_tree_1d(std::initializer_list<animation_clip*> clips, float startRelTime = 0.f, float startBlendValue = 0.f);

		void setBlendValue(float blendValue);
		void update(const animation_skeleton& skeleton, float dt, trs* outLocalTransforms, trs& outDeltaRootMotion);

	private:
		animation_clip* clips[8];
		uint32 numClips = 0;

		uint32 first;
		uint32 second;
		float value;
		float relTime;
		float blendValue;

		trs lastRootMotion;
	};

	struct animation_component
	{
		void initialize(std::vector<animation_clip>& clips, size_t startIndex = 0);
		void update(const ref<multi_mesh>& mesh, eallocator& arena, float dt, trs* transform = 0);
		void drawCurrentSkeleton(const ref<multi_mesh>& mesh, const trs& transform, ldr_render_pass* renderPass) const;

		ref<animation_instance> animation = nullptr;

		ref<animation_controller> controller = nullptr;

		dx_vertex_buffer_group_view currentVertexBuffer;
		dx_vertex_buffer_group_view prevFrameVertexBuffer;

		trs* currentGlobalTransforms = 0;

		float timeScale = 1.f;
		bool drawSceleton = false;
	};
}