#include "physics/basic_objects.h"
#include "physics/core/physics.h"

namespace era_engine::physics
{
    physx::PxRigidDynamic* create_rigid_cube(physx::PxReal half_extent, const physx::PxVec3& position)
    {
        using namespace physx;
        auto* physics = PhysicsEngine::get_physics_core()->get_physics();

        PxMaterial* material = physics->createMaterial(0.8f, 0.8f, 0.6f);

        PxShape* shape = physics->createShape(PxBoxGeometry(half_extent, half_extent, half_extent), *material);

        shape->setSimulationFilterData(PxFilterData(0, 0, 1, 0));

        PxTransform localTm(position);
        PxRigidDynamic* body = physics->createRigidDynamic(localTm);
        body->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);

        shape->release();

        return body;
    }

    void create_cube(physx::PxArray<physx::PxVec3>& tri_verts, physx::PxArray<physx::PxU32>& tri_indices, const physx::PxVec3& pos, physx::PxReal scaling)
    {
        using namespace physx;

        tri_verts.clear();
        tri_indices.clear();

        tri_verts.pushBack(scaling * PxVec3(0.5f, -0.5f, -0.5f) + pos);
        tri_verts.pushBack(scaling * PxVec3(0.5f, -0.5f, 0.5f) + pos);
        tri_verts.pushBack(scaling * PxVec3(-0.5f, -0.5f, 0.5f) + pos);
        tri_verts.pushBack(scaling * PxVec3(-0.5f, -0.5f, -0.5f) + pos);
        tri_verts.pushBack(scaling * PxVec3(0.5f, 0.5f, -0.5f) + pos);
        tri_verts.pushBack(scaling * PxVec3(0.5f, 0.5f, 0.5f) + pos);
        tri_verts.pushBack(scaling * PxVec3(-0.5f, 0.5f, 0.5f) + pos);
        tri_verts.pushBack(scaling * PxVec3(-0.5f, 0.5f, -0.5f) + pos);

        tri_indices.pushBack(1);
        tri_indices.pushBack(2);
        tri_indices.pushBack(3);
        tri_indices.pushBack(7);
        tri_indices.pushBack(6);
        tri_indices.pushBack(5);
        tri_indices.pushBack(4);
        tri_indices.pushBack(5);
        tri_indices.pushBack(1);
        tri_indices.pushBack(5);
        tri_indices.pushBack(6);
        tri_indices.pushBack(2);

        tri_indices.pushBack(2);
        tri_indices.pushBack(6);
        tri_indices.pushBack(7);
        tri_indices.pushBack(0);
        tri_indices.pushBack(3);
        tri_indices.pushBack(7);
        tri_indices.pushBack(0);
        tri_indices.pushBack(1);
        tri_indices.pushBack(3);
        tri_indices.pushBack(4);
        tri_indices.pushBack(7);
        tri_indices.pushBack(5);

        tri_indices.pushBack(0);
        tri_indices.pushBack(4);
        tri_indices.pushBack(1);
        tri_indices.pushBack(1);
        tri_indices.pushBack(5);
        tri_indices.pushBack(2);
        tri_indices.pushBack(3);
        tri_indices.pushBack(2);
        tri_indices.pushBack(7);
        tri_indices.pushBack(4);
        tri_indices.pushBack(0);
        tri_indices.pushBack(7);
    }
}