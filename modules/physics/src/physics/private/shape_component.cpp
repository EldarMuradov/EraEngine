#include "physics/shape_component.h"
#include "physics/core/physics.h"
#include "physics/core/physics_utils.h"
#include "physics/body_component.h"
#include "physics/shape_utils.h"

#include "asset/model_asset.h"

#include <rttr/registration>

namespace era_engine::physics
{

    RTTR_REGISTRATION
    {
        using namespace rttr;
        registration::class_<ShapeComponent>("ShapeComponent")
            .constructor<>();

        registration::class_<BoxShapeComponent>("BoxShapeComponent")
            .constructor<>();

        registration::class_<SphereShapeComponent>("SphereShapeComponent")
            .constructor<>();

        registration::class_<CapsuleShapeComponent>("CapsuleShapeComponent")
            .constructor<>();

        registration::class_<TriangleMeshShapeComponent>("TriangleMeshShapeComponent")
            .constructor<>();

        registration::class_<ConvexMeshShapeComponent>("ConvexMeshShapeComponent")
            .constructor<>();
    }

    ShapeComponent::ShapeComponent(ref<Entity::EcsData> _data)
        : Component(_data)
    {
        register_shape();
        handle_data = new Entity::Handle(_data->entity_handle);

        local_position.set_component(ComponentPtr(this));
        local_rotation.set_component(ComponentPtr(this));
    }

    ShapeComponent::~ShapeComponent()
    {
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

    void ShapeComponent::sync_with_joint(weakref<Entity::EcsData> _entity_reference, const std::string& _connected_joint_name)
    {
        connected_joint_name = _connected_joint_name;
        entity_reference = _entity_reference;
        attachment_active = false;
    }

    void ShapeComponent::set_attacment_state(bool active)
    {
        attachment_active = active;
    }

    void ShapeComponent::release()
    {
        PhysicsHolder::physics_ref->remove_shape_from_entity_data(this);

        PhysicsUtils::get_body_component(component_data)->detach_shape(shape);

        PX_RELEASE(shape)
        
        delete handle_data;

        Component::release();
    }

    void ShapeComponent::register_shape()
    {
        PhysicsHolder::physics_ref->add_shape_to_entity_data(this);
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

        ref<Physics> physics = PhysicsHolder::physics_ref;

        shape = physics->get_physics()->createShape(PxBoxGeometry(half_extents.x, half_extents.y, half_extents.z), *physics->get_default_material(), true);
        shape->userData = handle_data;
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

        ref<Physics> physics = PhysicsHolder::physics_ref;

        shape = physics->get_physics()->createShape(PxSphereGeometry(radius), *physics->get_default_material(), true);
        shape->userData = handle_data;

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

        ref<Physics> physics = PhysicsHolder::physics_ref;

        shape = physics->get_physics()->createShape(PxCapsuleGeometry(radius, half_height), *physics->get_default_material(), true);
        shape->userData = handle_data;

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
        PxTriangleMesh* mesh = ShapeUtils::build_triangle_mesh(asset.get(), size);
        ref<Physics> physics = PhysicsHolder::physics_ref;

        shape = physics->get_physics()->createShape(PxTriangleMeshGeometry(mesh, PxMeshScale(create_PxVec3(size))), *physics->get_default_material(), true);
        shape->userData = handle_data;

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
        PxConvexMesh* mesh = ShapeUtils::build_convex_mesh(asset.get(), size);
        ref<Physics> physics = PhysicsHolder::physics_ref;

        shape = physics->get_physics()->createShape(PxConvexMeshGeometry(mesh, PxMeshScale(create_PxVec3(size))), *physics->get_default_material(), true);
        shape->userData = handle_data;

        return shape;
    }

}