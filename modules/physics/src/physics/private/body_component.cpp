#include "physics/body_component.h"
#include "physics/core/physics.h"
#include "physics/shape_component.h"
#include "physics/cct_component.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_utils.h"

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
		registration::class_<BodyComponent>("BodyComponent")
			.constructor<>();

		registration::class_<DynamicBodyComponent>("DynamicBodyComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("compute_center_of_mass", &DynamicBodyComponent::compute_center_of_mass)
			.property("use_gravity", &DynamicBodyComponent::use_gravity)
			.property("ccd", &DynamicBodyComponent::ccd)
			.property("speculative_ccd", &DynamicBodyComponent::speculative_ccd)
			.property("kinematic", &DynamicBodyComponent::kinematic)
			.property("kinematic_motion_type", &DynamicBodyComponent::kinematic_motion_type)
			.property("simulated", &DynamicBodyComponent::simulated)
			.property("constraints", &DynamicBodyComponent::constraints)
			.property("mass", &DynamicBodyComponent::mass)
			.property("angular_damping", &DynamicBodyComponent::angular_damping)
			.property("linear_damping", &DynamicBodyComponent::linear_damping)
			.property("sleep_threshold", &DynamicBodyComponent::sleep_threshold)
			.property("stabilization_threshold", &DynamicBodyComponent::stabilization_threshold)
			.property("solver_position_iterations_count", &DynamicBodyComponent::solver_position_iterations_count)
			.property("solver_velocity_iterations_count", &DynamicBodyComponent::solver_velocity_iterations_count)
			.property("max_angular_velocity", &DynamicBodyComponent::max_angular_velocity)
			.property("max_linear_velocity", &DynamicBodyComponent::max_linear_velocity)
			.property("max_contact_impulse", &DynamicBodyComponent::max_contact_impulse)
			.property("max_depenetration_velocity", &DynamicBodyComponent::max_depenetration_velocity)
			.property("linear_velocity", &DynamicBodyComponent::linear_velocity)
			.property("angular_velocity", &DynamicBodyComponent::angular_velocity)
			.property("center_of_mass", &DynamicBodyComponent::center_of_mass)
			.property("mass_space_inertia_tensor", &DynamicBodyComponent::mass_space_inertia_tensor)
			.property("enable_gyroscopic_forces", &DynamicBodyComponent::enable_gyroscopic_forces);
		ComponentRegistry::register_component<DynamicBodyComponent>();

		registration::class_<StaticBodyComponent>("StaticBodyComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("simulated", &StaticBodyComponent::simulated);
		ComponentRegistry::register_component<StaticBodyComponent>();
	}

	BodyComponent::BodyComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	BodyComponent::~BodyComponent()
	{
	}

	physx::PxRigidActor* BodyComponent::get_rigid_actor() const
	{
		return actor;
	}

	void BodyComponent::detach_shape(physx::PxShape* shape)
	{
		if (actor != nullptr)
		{
			PhysicsEngine::execute_write([&]() {
				actor->detachShape(*shape);
				});
		}
	}

	DynamicBodyComponent::DynamicBodyComponent(ref<Entity::EcsData> _data)
		: BodyComponent(_data)
	{
		ComponentPtr this_component = ComponentPtr(this);

		simulated.set_component(this_component);
		kinematic.set_component(this_component);
		ccd.set_component(this_component);
		speculative_ccd.set_component(this_component);
		use_gravity.set_component(this_component);
		constraints.set_component(this_component);
		linear_damping.set_component(this_component);
		angular_damping.set_component(this_component);
		linear_velocity.set_component(this_component);
		angular_velocity.set_component(this_component);
		center_of_mass.set_component(this_component);
		mass.set_component(this_component);
		max_angular_velocity.set_component(this_component);
		max_linear_velocity.set_component(this_component);
		max_contact_impulse.set_component(this_component);
		max_depenetration_velocity.set_component(this_component);
		sleep_threshold.set_component(this_component);
		stabilization_threshold.set_component(this_component);
		solver_velocity_iterations_count.set_component(this_component);
		solver_position_iterations_count.set_component(this_component);
		mass_space_inertia_tensor.set_component(this_component);
		kinematic_motion_type.set_component(this_component);
		enable_gyroscopic_forces.set_component(this_component);
	}

	DynamicBodyComponent::~DynamicBodyComponent()
	{
	}

	physx::PxRigidDynamic* DynamicBodyComponent::get_rigid_dynamic() const
	{
		return actor == nullptr ? nullptr : actor->is<physx::PxRigidDynamic>();
	}

	vec3 DynamicBodyComponent::get_physics_position() const
	{
		using namespace physx;

		PxVec3 pos;

		PhysicsEngine::execute_read([&]() {
			pos = actor->getGlobalPose().p;
			});

		return create_vec3(pos);
	}

	void DynamicBodyComponent::wake_up()
	{
		using namespace physx;

		if (actor == nullptr)
		{
			return;
		}

		if (PxRigidDynamic* body = actor->is<PxRigidDynamic>())
		{
			if(simulated)
			{
				PhysicsEngine::execute_write([&]() {
					body->wakeUp();
					});
			}
		}
	}

	StaticBodyComponent::StaticBodyComponent(ref<Entity::EcsData> _data)
		: BodyComponent(_data)
	{
		simulated.set_component(ComponentPtr(this));
	}

	StaticBodyComponent::~StaticBodyComponent()
	{
	}

	physx::PxRigidStatic* StaticBodyComponent::get_rigid_static() const
	{
		return actor->is<physx::PxRigidStatic>();
	}

}