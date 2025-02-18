#include <game/movement/private/movement_system.h>
#include <game/movement/movement_component.h>
		 
#include <core/ecs/input_reciever_component.h>
#include <core/ecs/input_sender_component.h>
#include <core/ecs/camera_holder_component.h>

#include <ecs/update_groups.h>
#include <ecs/base_components/transform_component.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<MovementSystem>("MovementSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr)
			.method("update", &MovementSystem::update)(metadata("update_group", update_types::AFTER_PHYSICS))
			.method("reset_input", &MovementSystem::reset_input)(metadata("update_group", update_types::END));
	}

	MovementSystem::MovementSystem(World* _world)
		: System(_world)
	{
	}

	MovementSystem::~MovementSystem()
	{
	}

	void MovementSystem::init()
	{
	}

	void MovementSystem::update(float dt)
	{
		const float ROTATION_SENSITIVITY = 4.0f;
		const float MOVE_SPEED = 4.0f;

		for (auto [handle, transform_component, movement_component, reciever_component] : world->group(components_group<TransformComponent, MovementComponent, InputRecieverComponent>).each())
		{
			Entity movable = world->get_entity(handle);

			const UserInput& user_input = reciever_component.get_frame_input();
			const vec3& input = reciever_component.get_current_input();

			movement_component.apply_input(input);

			movement_component.last_velocity = movement_component.velocity;
			movement_component.velocity = input;

			vec2 turn_angle = vec2(-user_input.mouse.reldx, -user_input.mouse.reldy) * ROTATION_SENSITIVITY;

			quat& rotation = transform_component.transform.rotation;
			rotation = quat(vec3(0.f, 1.f, 0.f), turn_angle.x) * rotation;
			rotation = rotation * quat(vec3(1.f, 0.f, 0.f), turn_angle.y);

			transform_component.transform.position += rotation * movement_component.velocity * dt * MOVE_SPEED;
		}
	}

	void MovementSystem::reset_input(float dt)
	{
		for (auto [handle, transform_component, movement_component] : world->group(components_group<TransformComponent, MovementComponent>).each())
		{
			movement_component.apply_desired_input();
		}
	}

}