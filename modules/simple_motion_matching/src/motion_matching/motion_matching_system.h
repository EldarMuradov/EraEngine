#pragma once

#include "ecs/system.h"

#include "motion_matching/array.h"
#include "motion_matching/database.h"

#include "core/math.h"

namespace era_engine
{
	struct Allocator;
	class RendererHolderRootComponent;
	class MotionMatchingControllerComponent;

	class MotionMatchingSystem final : public System
	{
	public:
		MotionMatchingSystem(World* _world);
		~MotionMatchingSystem();

		void init() override;
		void update(float dt) override;

		void on_controller_created(entt::registry& registry, entt::entity entity);

		ERA_VIRTUAL_REFLECT(System)

	private:
		void draw_features(const slice1d<float> features, const Vec3 pos, const Quat rot, const vec4 color);

		void draw_trajectory(
			const slice1d<Vec3> trajectory_positions,
			const slice1d<Quat> trajectory_rotations,
			const vec4 color);

	private:
		Allocator* allocator = nullptr;
		RendererHolderRootComponent* renderer_holder_rc = nullptr;

		database db{};

		array1d<Vec3> obstacles_positions;
		array1d<Vec3> obstacles_scales;

		float feature_weight_foot_position = 0.75f;
		float feature_weight_foot_velocity = 1.0f;
		float feature_weight_hip_velocity = 1.0f;
		float feature_weight_trajectory_positions = 1.0f;
		float feature_weight_trajectory_directions = 1.5f;
	};
}