// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include <pch.h>
#include <px/blast/px_blast_destructions.h>

#if !_DEBUG

NODISCARD eentity physics::buildChunk(const trs& transform, ref<pbr_material> insideMaterial, ref<pbr_material> outsideMaterial, ::std::pair<ref<submesh_asset>, ref<nvmesh>> mesh, float mass, uint32 generation)
{
    auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

    if (!enttScene->registry.size())
        return eentity{};

    mesh_builder builder{ mesh_creation_flags_default, mesh_index_uint32 };

    auto mm = make_ref<multi_mesh>();

    bounding_box aabb;
    builder.pushMesh(*mesh.first, 1.0f, &aabb);

    mesh_asset asset;
    asset.name = ("Chunk" + ::std::to_string(id++));
    asset.submeshes.push_back(*mesh.first);

    mm->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, outsideMaterial });

    mm->aabb = bounding_box::negativeInfinity();

    eentity chunk = enttScene->createEntity(asset.name.c_str())
        .addComponent<transform_component>(transform)
        .addComponent<physics::px_convex_mesh_collider_component>(&asset)
        .addComponent<physics::px_rigidbody_component>(physics::px_rigidbody_type::Dynamic, false)
        .addComponent<nvmesh_chunk_component>(mesh.second)
        .addComponent<mesh_component>(mm);

    mm->aabb.grow(aabb.minCorner);
    mm->aabb.grow(aabb.maxCorner);

    static constexpr float chunkMass = 3.0f;

    auto& rb = chunk.getComponent<physics::px_rigidbody_component>();

    //rb.setMass(chunkMass);
    rb.setThreshold(0.02f, 0.05f);

    rb.setMaxAngularVelosity(1000.0f);
    rb.setMaxLinearVelosity(1000.0f);
    rb.setMaxContactImpulseFlag(true);
    rb.setAngularDamping(0.01f);
    rb.setLinearDamping(0.01f);

    auto dyn = rb.getRigidActor()->is<PxRigidDynamic>();
    dyn->setSolverIterationCounts(8, 16);

    dyn->setCMassLocalPose(PxTransform(PxVec3(0.0f)));

    dyn->setMaxContactImpulse(3000.0f);
    dyn->clearTorque();
    dyn->clearForce();

    dyn->setLinearVelocity(PxVec3(0.0f));
    dyn->setAngularVelocity(PxVec3(0.0f));

    rb.updateMassAndInertia(chunkMass);

    mm->mesh = builder.createDXMesh();

    return chunk;
}

#endif