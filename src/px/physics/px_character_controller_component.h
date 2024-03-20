#pragma once

#define PX_PHYSX_CHARACTER_STATIC_LIB

#include "px/core/px_physics_engine.h"

namespace physics
{
	using namespace physx;

	struct px_rigidbody_component;

	enum class px_cct_type : uint8
	{
		None,
		Box,
		Capsule
	};

	struct px_cct_component_base : px_rigidbody_component
	{
		px_cct_component_base(uint32_t entt) noexcept;
		virtual ~px_cct_component_base() {}

		void release(bool releaseActor = false) override;

		px_cct_type type = px_cct_type::None;

		float mass = 1.0f;

	protected:
		virtual void createCharacterController() noexcept {};

	protected:
		physx::PxControllerManager* manager = nullptr;
		physx::PxController* controller = nullptr;
	};

	struct px_box_cct_component : px_cct_component_base
	{
		px_box_cct_component() = default;
		px_box_cct_component(uint32_t entt) noexcept;
		px_box_cct_component(uint32_t entt, float hh, float hs, float m = 1.0f) noexcept;

		float halfHeight = 1.0f;
		float halfSideExtent = 0.5f;

	protected:
		void createCharacterController() noexcept override;
	};

	struct px_capsule_cct_component : px_cct_component_base
	{
		px_capsule_cct_component() = default;
		px_capsule_cct_component(uint32_t entt) noexcept;
		px_capsule_cct_component(uint32_t entt, float h, float r, float m = 1.0f) noexcept;
		~px_capsule_cct_component() {}

		float height = 2.0f;
		float radius = 0.5f;

	protected:
		void createCharacterController() noexcept override;
	};
}

#include "core/reflect.h"

REFLECT_STRUCT(physics::px_cct_component_base,
	(type, "Type"),
	(mass, "Mass")
);

REFLECT_STRUCT(physics::px_box_cct_component,
	(type, "Type"),
	(halfHeight, "HalfHeight"),
	(halfSideExtent, "HalfSideExtent")
);

REFLECT_STRUCT(physics::px_capsule_cct_component,
	(type, "Type"),
	(height, "Height"),
	(radius, "Radius")
);