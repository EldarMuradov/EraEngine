#pragma once

#include "ecs/system.h"

#include "motion_matching/array.h"

#include "core/math.h"

namespace era_engine
{
	struct Allocator;

	class RendererHolderRootComponent;
	class MotionMatchingComponent;

	class RuntimeMotionMatchingSystem final : public System
	{
	public:
		RuntimeMotionMatchingSystem(World* _world);
		~RuntimeMotionMatchingSystem() override;

		void init() override;
		void update(float dt) override;

		void on_controller_created(entt::registry& registry, entt::entity entity);

		ERA_VIRTUAL_REFLECT(System)

	private:
		void draw_trajectory(
			const slice1d<vec3>& trajectory_positions,
			const slice1d<quat>& trajectory_rotations,
			const vec4& color);

	private:
		Allocator* allocator = nullptr;
		RendererHolderRootComponent* renderer_holder_rc = nullptr;
	};
}