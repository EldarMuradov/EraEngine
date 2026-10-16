#include "physics/cct_component.h"
#include "physics/core/physics.h"

#include "ecs/world.h"
#include "ecs/base_components/transform_component.h"

#include <ecs/serialization/component_registry.h>
#include <ecs/serialization/serialization_meta.h>

#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::enumeration<CharacterControllerMoveMode>("CharacterControllerMoveMode")
		(
			value("WALKING", CharacterControllerMoveMode::WALKING),
			value("FLYING", CharacterControllerMoveMode::FLYING)
		);

		registration::enumeration<CharacterControllerComponent::ClimbingMode>("CharacterControllerClimbingMode")
		(
			value("RADIUS_AND_STEP_OFFSET", CharacterControllerComponent::ClimbingMode::RADIUS_AND_STEP_OFFSET),
			value("STEP_OFFSET", CharacterControllerComponent::ClimbingMode::STEP_OFFSET)
		);

		registration::enumeration<CharacterControllerComponent::NonWalkableMode>("CharacterControllerNonWalkableMode")
		(
			value("PREVENT_CLIMBING", CharacterControllerComponent::NonWalkableMode::PREVENT_CLIMBING),
			value("PREVENT_CLIMBING_AND_SLIDE", CharacterControllerComponent::NonWalkableMode::PREVENT_CLIMBING_AND_SLIDE)
		);

		registration::class_<CharacterControllerComponent>("CharacterControllerComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("collision_type", &CharacterControllerComponent::collision_type)
			.property("collision_filter_data", &CharacterControllerComponent::collision_filter_data)
			.property("height", &CharacterControllerComponent::height)
			.property("radius", &CharacterControllerComponent::radius)
			.property("step_offset", &CharacterControllerComponent::step_offset)
			.property("contact_offset", &CharacterControllerComponent::contact_offset)
			.property("volume_growth", &CharacterControllerComponent::volume_growth)
			.property("slope_limit", &CharacterControllerComponent::slope_limit)
			.property("move_mode", &CharacterControllerComponent::move_mode)
			.property("climbing_mode", &CharacterControllerComponent::climbing_mode)
			.property("non_walkable_mode", &CharacterControllerComponent::non_walkable_mode)
			.property("handle_non_cct_contacts", &CharacterControllerComponent::handle_non_cct_contacts);
		ComponentRegistry::register_component<CharacterControllerComponent>();
	}

	CharacterControllerComponent::CharacterControllerComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		velocity.set_component(ComponentPtr{this});
		offset.set_component(ComponentPtr{ this });
		collision_type.set_component(ComponentPtr{ this });
		collision_filter_data.set_component(ComponentPtr{ this });
	}

	CharacterControllerComponent::~CharacterControllerComponent()
	{
	}
}