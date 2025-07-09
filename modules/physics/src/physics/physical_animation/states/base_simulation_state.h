#pragma once

#include "physics_api.h"
#include "physics/physx_api.h"

#include <core/math.h>

#include <ecs/component.h>

namespace era_engine::physics
{
	enum class ConstraintStateType : uint8
	{
		DISABLED = 0,
		BLEND_IN,
		ENABLED,
		BLEND_OUT
	};

	class ERA_PHYSICS_API BaseSimulationState
	{
	public:
		BaseSimulationState() = default;
		BaseSimulationState(ComponentPtr _physical_animation_component_ptr);
		virtual ~BaseSimulationState();

		virtual ConstraintStateType try_switch_to(ConstraintStateType desired_type) const;

		virtual void on_entered();

		virtual void update(float dt);

	protected:
		ComponentPtr physical_animation_component_ptr = nullptr;
	};
}