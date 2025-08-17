#pragma once

#include "physics_api.h"
#include "physics/physx_api.h"

#include <core/math.h>

#include <ecs/component.h>

namespace era_engine::physics
{
	enum class ConstraintLimbStateType : uint8
	{
		KINEMATIC = 0,
		TRANSITION,
		SIMULATION
	};

	class ERA_PHYSICS_API BaseLimbState
	{
	public:
		BaseLimbState() = default;
		BaseLimbState(ComponentPtr _physical_animation_limb_component_ptr);
		virtual ~BaseLimbState();

		virtual ConstraintLimbStateType try_switch_to(ConstraintLimbStateType desired_type) const;

		virtual void on_enter();

		virtual void update(float dt);

		virtual void on_exit();

	protected:
		ComponentPtr physical_animation_limb_component_ptr = nullptr;
	};
}