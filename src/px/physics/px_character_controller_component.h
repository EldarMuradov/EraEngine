#pragma once

#define PX_PHYSX_CHARACTER_STATIC_LIB

#include "px/core/px_physics_engine.h"

struct eentity;

enum class px_cct_type : uint8
{
	px_none,
	px_box,
	px_capsule
};

struct px_cct_component_base : px_rigidbody_component
{
	px_cct_component_base(eentity* entt) noexcept;
	virtual ~px_cct_component_base() {}

	eentity* entity = nullptr;

protected:
	virtual void createCharacterController() noexcept {};

protected:
	physx::PxControllerManager* manager = nullptr;
	physx::PxMaterial* material = nullptr;
	physx::PxController* controller = nullptr;

	trs* transform = nullptr;

	px_cct_type type = px_cct_type::px_none;
	float mass = 1.0f;
};

struct px_box_cct_component : px_cct_component_base
{
	px_box_cct_component() = default;
	px_box_cct_component(eentity* entt) noexcept;
	px_box_cct_component(eentity* entt, float hh, float hs, float m = 1.0f) noexcept;

	float getHalfHeight() const noexcept { return halfHeight; }
	float getHalfSideExtent() const noexcept { return halfSideExtent; }

protected:
	void createCharacterController() noexcept override;

private:
	float halfHeight = 1.0f;
	float halfSideExtent = 0.5f;
};

struct px_capsule_cct_component : px_cct_component_base
{
	px_capsule_cct_component() = default;
	px_capsule_cct_component(eentity* entt) noexcept;
	px_capsule_cct_component(eentity* entt, float h, float r, float m = 1.0f) noexcept;
	~px_capsule_cct_component() {}

	float getHeight() const noexcept { return height; }
	float getRadius() const noexcept { return radius; }

protected:
	void createCharacterController() noexcept override;

private:
	float height = 2.0f;
	float radius = 0.5f;
};