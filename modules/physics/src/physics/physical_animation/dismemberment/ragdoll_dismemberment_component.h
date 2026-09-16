#pragma once

#include "physics_api.h"
#include "physics/physical_animation/dismemberment/ragdoll_dismemberment_profile.h"

#include <core/math.h>

#include <geometry/mesh.h>
#include <asset/model_asset.h>

#include <ecs/component.h>
#include <ecs/observable_member.h>

namespace era_engine::physics
{
	struct DismemberedPart
	{
		std::string name;
		std::vector<uint32> bone_ids;

		SubmeshAsset mesh;
		uint32 source_submesh_index = 0;
	};

	struct SubmeshPartSplit
	{
		uint32 source_submesh_index = 0;
		std::vector<std::vector<indexed_triangle32>> group_triangles;
	};

	struct DismembermentState
	{
		std::vector<DismemberedPart> parts;
		std::vector<SubmeshPartSplit> splits;
		std::vector<bool> detached;
		std::vector<SubmeshAsset> source_submeshes;

		bool initialized = false;
	};

	struct DismembermentTaskResult
	{
		SubmeshAsset live;
		std::vector<SubmeshAsset> detached;
		std::vector<uint32> detached_indices;
	};

	struct DismembermentTaskData
	{
		std::vector<SubmeshAsset> source_submeshes;
		std::vector<SubmeshPartSplit> splits;
		std::vector<DismemberedPart> parts;
		std::vector<bool> detached;

		DismembermentTaskResult result;

		std::atomic<bool> completed = false;
	};

	struct BoneGroup
	{
		std::string name;
		std::vector<uint32> bone_indices;
	};

	enum class DismemberState : uint8
	{
		NONE = 0,
		PENDING,
		PARTIAL_DISMEMBERED,
		DISMEMBERED,
	};

	class ERA_PHYSICS_API RagdollDismembermentLimbComponent : public Component
	{
	public:
		RagdollDismembermentLimbComponent() = default;
		RagdollDismembermentLimbComponent(ref<Entity::EcsData> _data);

		void apply_damage(float damage);

		void trigger_dismembering();

		bool is_dismembered() const;
		bool is_pending() const;

		DismemberState get_current_state() const;

	public:
		EntityPtr ragdoll_ptr;

		float current_health = 0.0f;

        ERA_VIRTUAL_REFLECT(Component)

	private:
		DismemberState state = DismemberState::NONE;

		bool can_be_dismembered = true;

		friend class RagdollDismembermentSystem;
	};

	using DismemberEventFunc = void(*)(RagdollDismembermentLimbComponent*);

	class ERA_PHYSICS_API RagdollDismembermentComponent : public Component
	{
	public:
		RagdollDismembermentComponent() = default;
		RagdollDismembermentComponent(ref<Entity::EcsData> _data);

	public:
		ref<RagdollDismembermentProfile> dismemberment_profile;

		DismemberEventFunc on_dismember = nullptr;

		DismembermentState dismemberment_state;

		bool affected_by_collisions = true;

		std::vector<uint32> pending_detached_parts;

		DismembermentTaskData* pending_task = nullptr;
		JobHandle pending_job;

        ERA_VIRTUAL_REFLECT(Component)

	private:
		bool loaded = false;

		friend class RagdollDismembermentSystem;
	};
}