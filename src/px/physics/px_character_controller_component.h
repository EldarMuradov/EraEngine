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

struct px_character_controller_component
{
	px_character_controller_component(eentity* entt) noexcept;

	px_character_controller_component(eentity* entt, float h, float r, float m = 1.0f) noexcept;

	~px_character_controller_component() {}

	eentity* entity = nullptr;

private:
	void createCharacterController() noexcept;

private:
	physx::PxControllerManager* manager = nullptr;
	physx::PxMaterial* material = nullptr;
	physx::PxController* controller = nullptr;

	trs* transform = nullptr;

	px_cct_type type = px_cct_type::px_capsule;

	float height = 2.0f;
	float radius = 0.5f;
	float mass = 1.0f;
};