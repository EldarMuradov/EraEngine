#include "physics/core/shape_system.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_component.h"

#include <core/cpu_profiling.h>

#include <ecs/base_components/transform_component.h>
#include <ecs/update_groups.h>

#include <animation/animation.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<ShapeSystem>("ShapeSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))
			//.method("update", &ShapeSystem::update)(metadata("update_group", update_types::PHYSICS), metadata("After", std::vector<std::string>{"AnimationSystem::update"}))
			.method("process_skeleton_attachments", &ShapeSystem::process_skeleton_attachments)(metadata("update_group", update_types::AFTER_PHYSICS));
	}

	ShapeSystem::ShapeSystem(World* _world)
		: System(_world)
	{
	}

	void ShapeSystem::init()
	{
	}

	void ShapeSystem::update(float dt)
	{

	}

	void ShapeSystem::process_skeleton_attachments(float dt) const
	{
		using namespace physx;
		using namespace animation;

		auto& physics_ref = PhysicsHolder::physics_ref;

		for (auto [entity_handle, dynamic_body_component, transform_component] 
			: world->group(components_group<DynamicBodyComponent, TransformComponent>).each())
		{
			auto& colliders = physics_ref->colliders_map[entity_handle];

			if (colliders.empty())
			{
				continue;
			}

			auto& shape_component = colliders.front(); // Only for single-shape entities.
			if (!shape_component->attachment_active)
			{
				continue;
			}

			if (ref<Entity::EcsData> reference_data = shape_component->entity_reference.lock())
			{
				Entity referenced_entity = world->get_entity(reference_data->entity_handle);

				const SkeletonComponent* skeleton_component = referenced_entity.get_component_if_exists<SkeletonComponent>();
				if (skeleton_component == nullptr)
				{
					continue;
				}

				const Skeleton* skeleton = skeleton_component->skeleton;
				if (skeleton == nullptr)
				{
					continue;
				}

				const uint32 joint_id = skeleton->name_to_joint_id.at(shape_component->connected_joint_name);
				const trs reference_space_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_id);
				const trs& reference_world_transform = referenced_entity.get_component<TransformComponent>().transform;
				const trs world_space_joint_trasnform = reference_world_transform * reference_space_joint_transform;

				PhysicsUtils::manual_set_physics_transform(world->get_entity(entity_handle), world_space_joint_trasnform, true);
			}
		}
	}

}
