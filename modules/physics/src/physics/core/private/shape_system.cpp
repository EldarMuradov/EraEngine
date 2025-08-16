#include "physics/core/shape_system.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_component.h"
#include "physics/shape_utils.h"

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
			.method("update", &ShapeSystem::update)(metadata("update_group", update_types::PHYSICS))
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
		using namespace physx;

		ZoneScopedN("ShapeSystem::update");

		for (auto [entity_handle, changed_flag, shape_component] : world->group(components_group<ObservableMemberChangedFlagComponent, BoxShapeComponent>).each())
		{
			if (shape_component.get_shape() == nullptr)
			{
				continue;
			}

			if (shape_component.local_position.is_changed() ||
				shape_component.local_rotation.is_changed())
			{
				shape_component.get_shape()->setLocalPose(create_PxTransform(trs(shape_component.local_position, shape_component.local_rotation, vec3(1.0f))));
			}

			if (shape_component.use_in_scene_queries.is_changed())
			{
				shape_component.get_shape()->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, shape_component.use_in_scene_queries);
			}

			if (shape_component.is_trigger.is_changed())
			{
				shape_component.get_shape()->setFlag(PxShapeFlag::eTRIGGER_SHAPE, shape_component.is_trigger);
			}

			if (shape_component.collision_filter_data.is_changed() ||
				shape_component.collision_type.is_changed())
			{
				ShapeUtils::setup_filtering(world, shape_component.get_shape(), static_cast<uint32>(shape_component.collision_type.get()), shape_component.collision_filter_data);
			}
		}

		for (auto [entity_handle, changed_flag, shape_component] : world->group(components_group<ObservableMemberChangedFlagComponent, CapsuleShapeComponent>).each())
		{
			if (shape_component.get_shape() == nullptr)
			{
				continue;
			}

			if (shape_component.local_position.is_changed() ||
				shape_component.local_rotation.is_changed())
			{
				shape_component.get_shape()->setLocalPose(create_PxTransform(trs(shape_component.local_position, shape_component.local_rotation, vec3(1.0f))));
			}

			if (shape_component.use_in_scene_queries.is_changed())
			{
				shape_component.get_shape()->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, shape_component.use_in_scene_queries);
			}

			if (shape_component.is_trigger.is_changed())
			{
				shape_component.get_shape()->setFlag(PxShapeFlag::eTRIGGER_SHAPE, shape_component.is_trigger);
			}

			if (shape_component.collision_filter_data.is_changed() ||
				shape_component.collision_type.is_changed())
			{
				ShapeUtils::setup_filtering(world, shape_component.get_shape(), static_cast<uint32>(shape_component.collision_type.get()), shape_component.collision_filter_data);
			}
		}

		for (auto [entity_handle, changed_flag, shape_component] : world->group(components_group<ObservableMemberChangedFlagComponent, SphereShapeComponent>).each())
		{
			if (shape_component.get_shape() == nullptr)
			{
				continue;
			}

			if (shape_component.local_position.is_changed() ||
				shape_component.local_rotation.is_changed())
			{
				shape_component.get_shape()->setLocalPose(create_PxTransform(trs(shape_component.local_position, shape_component.local_rotation, vec3(1.0f))));
			}

			if (shape_component.use_in_scene_queries.is_changed())
			{
				shape_component.get_shape()->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, shape_component.use_in_scene_queries);
			}

			if (shape_component.is_trigger.is_changed())
			{
				shape_component.get_shape()->setFlag(PxShapeFlag::eTRIGGER_SHAPE, shape_component.is_trigger);
			}

			if (shape_component.collision_filter_data.is_changed() ||
				shape_component.collision_type.is_changed())
			{
				ShapeUtils::setup_filtering(world, shape_component.get_shape(), static_cast<uint32>(shape_component.collision_type.get()), shape_component.collision_filter_data);
			}
		}

		for (auto [entity_handle, changed_flag, shape_component] : world->group(components_group<ObservableMemberChangedFlagComponent, ConvexMeshShapeComponent>).each())
		{
			if (shape_component.get_shape() == nullptr)
			{
				continue;
			}

			if (shape_component.local_position.is_changed() ||
				shape_component.local_rotation.is_changed())
			{
				shape_component.get_shape()->setLocalPose(create_PxTransform(trs(shape_component.local_position, shape_component.local_rotation, vec3(1.0f))));
			}

			if (shape_component.use_in_scene_queries.is_changed())
			{
				shape_component.get_shape()->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, shape_component.use_in_scene_queries);
			}

			if (shape_component.is_trigger.is_changed())
			{
				shape_component.get_shape()->setFlag(PxShapeFlag::eTRIGGER_SHAPE, shape_component.is_trigger);
			}

			if (shape_component.collision_filter_data.is_changed() ||
				shape_component.collision_type.is_changed())
			{
				ShapeUtils::setup_filtering(world, shape_component.get_shape(), static_cast<uint32>(shape_component.collision_type.get()), shape_component.collision_filter_data);
			}
		}

		for (auto [entity_handle, changed_flag, shape_component] : world->group(components_group<ObservableMemberChangedFlagComponent, TriangleMeshShapeComponent>).each())
		{
			if (shape_component.get_shape() == nullptr)
			{
				continue;
			}

			if (shape_component.local_position.is_changed() ||
				shape_component.local_rotation.is_changed())
			{
				shape_component.get_shape()->setLocalPose(create_PxTransform(trs(shape_component.local_position, shape_component.local_rotation, vec3(1.0f))));
			}

			if (shape_component.use_in_scene_queries.is_changed())
			{
				shape_component.get_shape()->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, shape_component.use_in_scene_queries);
			}

			if (shape_component.is_trigger.is_changed())
			{
				shape_component.get_shape()->setFlag(PxShapeFlag::eTRIGGER_SHAPE, shape_component.is_trigger);
			}

			if (shape_component.collision_filter_data.is_changed() ||
				shape_component.collision_type.is_changed())
			{
				ShapeUtils::setup_filtering(world, shape_component.get_shape(), static_cast<uint32>(shape_component.collision_type.get()), shape_component.collision_filter_data);
			}
		}
	}

	void ShapeSystem::process_skeleton_attachments(float dt) const
	{
		using namespace physx;
		using namespace animation;

		ZoneScopedN("ShapeSystem::process_skeleton_attachments");

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

				const trs reference_space_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, shape_component->connected_joint_id);
				const trs& reference_world_transform = referenced_entity.get_component<TransformComponent>()->get_world_transform();
				const trs world_space_joint_trasnform = reference_world_transform * reference_space_joint_transform;

				PhysicsUtils::manual_set_physics_transform(world->get_entity(entity_handle), world_space_joint_trasnform, true);
			}
		}
	}

}
