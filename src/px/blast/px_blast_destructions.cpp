// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include <pch.h>
#include <px/blast/px_blast_destructions.h>

#if !_DEBUG

NODISCARD eentity physics::buildChunk(const trs& transform, ref<pbr_material> insideMaterial, ref<pbr_material> outsideMaterial, ::std::pair<ref<submesh_asset>, ref<nvmesh>> mesh, float mass, uint32 generation)
{
    auto enttScene = globalApp.getCurrentScene();

    if (!enttScene->registry.size())
        return eentity{};

    mesh_builder builder{ mesh_creation_flags_default, mesh_index_uint32 };

    auto mm = make_ref<multi_mesh>();

    bounding_box aabb;
    builder.pushMesh(*mesh.first, 1.0f, &aabb);

    mesh_asset asset;
    asset.name = ("Chunk_" + ::std::to_string(id++));
    asset.submeshes.push_back(*mesh.first);

    mm->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, outsideMaterial });

    mm->aabb = bounding_box::negativeInfinity();

    eentity chunk = enttScene->createEntity(asset.name.c_str())
        .addComponent<transform_component>(transform)
        //.addComponent<physics::px_convex_mesh_collider_component>(&asset)
        .addComponent<physics::px_rigidbody_component>(physics::px_rigidbody_type::rigidbody_type_dynamic)
        .addComponent<nvmesh_chunk_component>(mesh.second)
        .addComponent<mesh_component>(mm);

    mm->aabb.grow(aabb.minCorner);
    mm->aabb.grow(aabb.maxCorner);

    mm->mesh = builder.createDXMesh();

    return chunk;
}

#endif