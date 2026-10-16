#include "physics/shape_component.h"
#include "physics/core/physics.h"
#include "physics/core/physics_utils.h"
#include "physics/body_component.h"
#include "physics/shape_utils.h"

#include "asset/model_asset.h"

#include <ecs/serialization/component_registry.h>
#include <ecs/serialization/serialization_meta.h>

#include <rttr/registration>

namespace era_engine::physics
{

    RTTR_REGISTRATION
    {
        using namespace rttr;
        registration::class_<ShapeComponent>("ShapeComponent")
            .constructor<>()
            .property("local_position", &ShapeComponent::local_position)
            .property("local_rotation", &ShapeComponent::local_rotation)
            .property("collision_type", &ShapeComponent::collision_type)
            .property("collision_filter_data", &ShapeComponent::collision_filter_data)
            .property("use_in_scene_queries", &ShapeComponent::use_in_scene_queries)
            .property("is_trigger", &ShapeComponent::is_trigger);

        registration::class_<BoxShapeComponent>("BoxShapeComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("half_extents", &BoxShapeComponent::half_extents);
        ComponentRegistry::register_component<BoxShapeComponent>();

        registration::class_<SphereShapeComponent>("SphereShapeComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("radius", &SphereShapeComponent::radius);
        ComponentRegistry::register_component<SphereShapeComponent>();

        registration::class_<CapsuleShapeComponent>("CapsuleShapeComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("radius", &CapsuleShapeComponent::radius)
            .property("half_height", &CapsuleShapeComponent::half_height);
        ComponentRegistry::register_component<CapsuleShapeComponent>();

        registration::class_<TriangleMeshShapeComponent>("TriangleMeshShapeComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
            .constructor<ref<Entity::EcsData>>()
            .property("size", &TriangleMeshShapeComponent::size);
        ComponentRegistry::register_component<TriangleMeshShapeComponent>();

        registration::class_<ConvexMeshShapeComponent>("ConvexMeshShapeComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
            .constructor<ref<Entity::EcsData>>()
            .property("size", &ConvexMeshShapeComponent::size);
        ComponentRegistry::register_component<ConvexMeshShapeComponent>();
    }

    ShapeComponent::ShapeComponent(ref<Entity::EcsData> _data)
        : Component(_data)
    {
        register_shape();

        local_position.set_component(ComponentPtr(this));
        local_rotation.set_component(ComponentPtr(this));
        collision_type.set_component(ComponentPtr(this));
        collision_filter_data.set_component(ComponentPtr(this));
        is_trigger.set_component(ComponentPtr(this));
        use_in_scene_queries.set_component(ComponentPtr(this));
    }

    ShapeComponent::~ShapeComponent()
    {
        PhysicsEngine::get_physics_core()->remove_shape_from_entity_data(this);

        if (shape == nullptr)
        {
            return;
        }

        BodyComponent* body_component = PhysicsUtils::get_body_component(component_data);
        if (body_component != nullptr)
        {
            body_component->detach_shape(shape);
        }

        PhysicsEngine::execute_write([&]() {
            if (shape->isReleasable())
            {
                PX_RELEASE(shape)
            }
        });
    }

    physx::PxShape* ShapeComponent::get_shape() const
    {
        return shape;
    }

    const physx::PxGeometry& ShapeComponent::get_geometry() const
    {
        ASSERT(shape != nullptr);
        return shape->getGeometry();
    }

    void ShapeComponent::sync_with_joint(weakref<Entity::EcsData> _entity_reference, uint32 _connected_joint_id)
    {
        connected_joint_id = _connected_joint_id;
        entity_reference = _entity_reference;
        attachment_active = false;
    }

    void ShapeComponent::set_attacment_state(bool active)
    {
        attachment_active = active;
    }

    void ShapeComponent::register_shape()
    {
        PhysicsEngine::get_physics_core()->add_shape_to_entity_data(this);
    }

    physx::PxShape* ShapeComponent::create_shape()
    {
        return nullptr;
    }

    BoxShapeComponent::BoxShapeComponent(ref<Entity::EcsData> _data)
        : ShapeComponent(_data)
    {
    }

    BoxShapeComponent::~BoxShapeComponent()
    {
    }

    physx::PxShape* BoxShapeComponent::create_shape()
    {
        using namespace physx;

        ref<Physics> physics = PhysicsEngine::get_physics_core();

        PxMaterial* used_material = material == nullptr 
            ? physics->get_default_material()->get_native_material() 
            : material->get_native_material();

        shape = physics->get_physics()->createShape(PxBoxGeometry(half_extents.x, half_extents.y, half_extents.z), 
            *used_material, true);
        shape->userData = this;

        //PhysicsEngine::execute_write([&]() {
        //    shape->setFlag(physx::PxShapeFlag::eVISUALIZATION, false);
        //    });

        return shape;
    }

    SphereShapeComponent::SphereShapeComponent(ref<Entity::EcsData> _data)
        : ShapeComponent(_data)
    {
    }

    SphereShapeComponent::~SphereShapeComponent()
    {
    }

    physx::PxShape* SphereShapeComponent::create_shape()
    {
        using namespace physx;

        ref<Physics> physics = PhysicsEngine::get_physics_core();

        PxMaterial* used_material = material == nullptr
            ? physics->get_default_material()->get_native_material()
            : material->get_native_material();

        shape = physics->get_physics()->createShape(PxSphereGeometry(radius), 
            *used_material, true);
        shape->userData = this;

        //PhysicsEngine::execute_write([&]() {
        //    shape->setFlag(physx::PxShapeFlag::eVISUALIZATION, false);
        //    });

        return shape;
    }

    CapsuleShapeComponent::CapsuleShapeComponent(ref<Entity::EcsData> _data)
        : ShapeComponent(_data)
    {
    }

    CapsuleShapeComponent::~CapsuleShapeComponent()
    {
    }

    physx::PxShape* CapsuleShapeComponent::create_shape()
    {
        using namespace physx;

        ref<Physics> physics = PhysicsEngine::get_physics_core();

        PxMaterial* used_material = material == nullptr
			? physics->get_default_material()->get_native_material()
			: material->get_native_material();

		shape = physics->get_physics()->createShape(PxCapsuleGeometry(radius, half_height),
			*used_material, true);
		shape->userData = this;

		//PhysicsEngine::execute_write([&]() {
		//	shape->setFlag(physx::PxShapeFlag::eVISUALIZATION, false);
		//	});

        return shape;
    }

    TriangleMeshShapeComponent::TriangleMeshShapeComponent(ref<Entity::EcsData> _data)
        : ShapeComponent(_data)
    {
    }

    TriangleMeshShapeComponent::~TriangleMeshShapeComponent()
    {
    }

    physx::PxShape* TriangleMeshShapeComponent::create_shape()
    {
        using namespace physx;
        PxTriangleMesh* mesh = ShapeUtils::build_triangle_mesh(asset);
        ref<Physics> physics = PhysicsEngine::get_physics_core();

        PxMaterial* used_material = material == nullptr
            ? physics->get_default_material()->get_native_material()
            : material->get_native_material();

        shape = physics->get_physics()->createShape(PxTriangleMeshGeometry(mesh, PxMeshScale(create_PxVec3(size)), PxMeshGeometryFlag::eTIGHT_BOUNDS),
            *used_material, true);
        shape->userData = this;

        PhysicsEngine::execute_write([&]() {
            shape->setRestOffset(-0.005f);
            //shape->setFlag(physx::PxShapeFlag::eVISUALIZATION, false);
            });

        return shape;
    }

    ConvexMeshShapeComponent::ConvexMeshShapeComponent(ref<Entity::EcsData> _data)
        : ShapeComponent(_data)
    {
    }

    ConvexMeshShapeComponent::~ConvexMeshShapeComponent()
    {
    }

    physx::PxShape* ConvexMeshShapeComponent::create_shape()
    {
        using namespace physx;
        PxConvexMesh* mesh = ShapeUtils::build_convex_mesh(asset, size);
        ref<Physics> physics = PhysicsEngine::get_physics_core();

        PxMaterial* used_material = material == nullptr
            ? physics->get_default_material()->get_native_material()
            : material->get_native_material();

        shape = physics->get_physics()->createShape(PxConvexMeshGeometry(mesh, PxMeshScale(), PxConvexMeshGeometryFlag::eTIGHT_BOUNDS),
            *used_material, true);
        shape->userData = this;

        PhysicsEngine::execute_write([&]() {
            shape->setRestOffset(-0.005f);
            //shape->setFlag(physx::PxShapeFlag::eVISUALIZATION, false);
            });

        return shape;
    }

}