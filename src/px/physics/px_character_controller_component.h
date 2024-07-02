// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#define PX_PHYSX_CHARACTER_STATIC_LIB

#include "px/core/px_physics_engine.h"

namespace era_engine::physics
{
	using namespace physx;

	enum class px_cct_type : uint8
	{
		cct_type_none,
		cct_type_box,
		cct_type_capsule
	};

	struct px_cct_component_base : px_rigidbody_component
	{
		px_cct_component_base(uint32 handle);
		virtual ~px_cct_component_base() {}

		virtual void release(bool releaseActor = true) noexcept override;

		px_cct_type type = px_cct_type::cct_type_none;

		float mass = 1.0f;

	protected:
		virtual void createCharacterController() noexcept {};

		physx::PxControllerManager* manager = nullptr;
		physx::PxController* controller = nullptr;
	};

	struct px_box_cct_component : px_cct_component_base
	{
		px_box_cct_component() = default;
		px_box_cct_component(uint32 handle);
		px_box_cct_component(uint32 handle, float hh, float hs, float m = 1.0f);

		float halfHeight = 1.0f;
		float halfSideExtent = 0.5f;

	protected:
		void createCharacterController() noexcept override;
	};

	struct px_capsule_cct_component : px_cct_component_base
	{
		px_capsule_cct_component() = default;
		px_capsule_cct_component(uint32 handle);
		px_capsule_cct_component(uint32 handle, float h, float r, float m = 1.0f);
		~px_capsule_cct_component() {}

		float height = 2.0f;
		float radius = 0.5f;

	protected:
		void createCharacterController() noexcept override;
	};
}

#include "core/reflect.h"

namespace era_engine
{
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
}