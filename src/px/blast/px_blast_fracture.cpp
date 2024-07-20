// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "pch.h"

#include "px/blast/px_blast_fracture.h"
#include "px/blast/px_nvmesh.h"
#include "px/blast/px_blast_destructions.h"
#include "px/core/px_extensions.h"

#include <NvBlastExtAuthoring.h>

namespace era_engine::physics
{
	fracture_tool::fracture_tool()
    {
        fractureTool = NvBlastExtAuthoringCreateFractureTool();
    }

    fracture_tool::~fracture_tool()
    {
        PX_RELEASE(fractureTool)
    }

    float randomGenerator::getRandomValue()
    {
        std::random_device rd;
        std::mt19937 eng(rd());
        std::uniform_real_distribution<float> distr(0.0f, 1.0f);
        float nb = distr(eng);
        return nb;
    }

    void randomGenerator::seed(int32_t seed)
    {
        seedResult = seed;
    }

    voronoi_sites_generator::voronoi_sites_generator(ref<nvmesh> mesh)
    {
        rndGen = make_ref<randomGenerator>();
        generator = NvBlastExtAuthoringCreateVoronoiSitesGenerator(mesh->mesh, rndGen.get());
    }

    void voronoi_sites_generator::release()
    {
        RELEASE_PTR(generator)
    }

    std::vector<std::pair<ref<submesh_asset>, ref<nvmesh>>> fractureMeshesInNvblast(int totalChunks, ref<nvmesh> nvMesh, bool replace)
    {
        ref<fracture_tool> fractureTool = make_ref<fracture_tool>();
        fractureTool->fractureTool->setRemoveIslands(true);
        fractureTool->fractureTool->setSourceMeshes(&nvMesh->mesh, 1);
        ref<voronoi_sites_generator> generator = make_ref<voronoi_sites_generator>(nvMesh);
        generator->generator->setBaseMesh(nvMesh->mesh);

        //auto fractureFunc = [&]()
        //    {


        //        //fractureTool->fractureTool->uniteChunks(0.001f, 10, nullptr, 0, nullptr, 0);
        //    };

        //std::packaged_task<void()> task{ fractureFunc };
        //std::future<void> future = task.get_future();

        //std::jthread thread{ ::std::move(task) };

        //future.get();

        generator->generator->uniformlyGenerateSitesInMesh(totalChunks);

        const NvcVec3* sites;
        size_t nbSites = generator->generator->getVoronoiSites(sites);

        int result = fractureTool->fractureTool->voronoiFracturing(0, nbSites, sites, replace);

        if (result != 0)
        {
            LOG_ERROR("NvBlast> Failed to fracture mesh!");
            return {};
        }

        fractureTool->fractureTool->finalizeFracturing();

        // Extract meshes
        size_t meshCount = fractureTool->fractureTool->getChunkCount();

        std::vector<std::pair<ref<submesh_asset>, ref<nvmesh>>> meshes;
        meshes.reserve(meshCount);

        std::vector<std::vector<Nv::Blast::Triangle>> chunkMeshes;
        chunkMeshes.reserve(meshCount);

        Nv::Blast::Triangle* trigs;

        for (size_t i = 1; i < meshCount; ++i)
        {
            uint32_t nbTrigs = fractureTool->fractureTool->getBaseMesh(i, trigs);
            std::vector<Nv::Blast::Triangle> trigList;
            for (size_t j = 0; j < nbTrigs; ++j)
            {
                trigList.push_back(trigs[j]);
            }

            chunkMeshes.push_back(trigList);
        }

        for (size_t i = 0; i < chunkMeshes.size(); ++i)
        {
            std::vector<physx::PxVec3> pos;
            std::vector<physx::PxVec3> norm;
            std::vector<physx::PxVec2> tex;
            std::vector<uint32_t> indexs;

            uint32_t index = 0;
            std::vector<Nv::Blast::Triangle>& chunk = chunkMeshes[i];

            for (size_t j = 0; j < chunk.size(); ++j)
            {
                pos.push_back({ chunk[j].a.p.x, chunk[j].a.p.y, chunk[j].a.p.z });
                pos.push_back({ chunk[j].b.p.x, chunk[j].b.p.y, chunk[j].b.p.z });
                pos.push_back({ chunk[j].c.p.x, chunk[j].c.p.y, chunk[j].c.p.z });

                norm.push_back({ chunk[j].a.n.x, chunk[j].a.n.y, chunk[j].a.n.z });
                norm.push_back({ chunk[j].b.n.x, chunk[j].b.n.y, chunk[j].b.n.z });
                norm.push_back({ chunk[j].c.n.x, chunk[j].c.n.y, chunk[j].c.n.z });

                tex.push_back({ chunk[j].a.uv[0].x, chunk[j].a.uv[0].y });
                tex.push_back({ chunk[j].b.uv[0].x, chunk[j].b.uv[0].y });
                tex.push_back({ chunk[j].c.uv[0].x, chunk[j].c.uv[0].y });

                indexs.push_back(index++);
                indexs.push_back(index++);
                indexs.push_back(index++);
            }

            ref<nvmesh> mesh = make_ref<nvmesh>(pos, norm, tex, indexs);

            if (fractureTool->fractureTool->isMeshContainOpenEdges(mesh->mesh))
            {
                LOG_WARNING("NvBlast> Mesh contains open edges!");
                //return {};
            }

            auto chunkMesh = createRenderMesh(*mesh);

            meshes.push_back(std::make_pair(chunkMesh, mesh));
        }

        return { meshes };
    }

    entity_handle fracture::fractureGameObject(ref<submesh_asset> meshAsset, const eentity& gameObject, anchor anchor, unsigned int seed, int totalChunks, ref<pbr_material> insideMaterial, ref<pbr_material> outsideMaterial, float jointBreakForce, float density)
    {
        if (totalChunks <= 0)
            return null_entity;

        auto enttScene = globalApp.getCurrentScene();

        eentity fractureGameObject = enttScene->createEntity("Fracture")
            .addComponent<transform_component>(vec3(0.0f), quat::identity, vec3(1.f));
        auto& graphManager = fractureGameObject.addComponent<chunk_graph_manager>().getComponent<chunk_graph_manager>();

        randomGenerator::seedResult = seed;

        std::vector<uint32_t> indices;
        for (size_t i = 0; i < meshAsset->triangles.size(); i++)
        {
            indices.push_back(meshAsset->triangles[i].a);
            indices.push_back(meshAsset->triangles[i].b);
            indices.push_back(meshAsset->triangles[i].c);
        }

        ref<nvmesh> nvMesh = make_ref<nvmesh>(
            createStdVectorPxVec3(meshAsset->positions),
            createStdVectorPxVec3(meshAsset->normals),
            createStdVectorPxVec2(meshAsset->uvs),
            indices
        );

        std::vector<std::pair<ref<submesh_asset>, ref<nvmesh>>> meshes;

        if (totalChunks == 1)
            meshes.push_back({ meshAsset, nvMesh });
        else
            meshes = fractureMeshesInNvblast(totalChunks, nvMesh);

        // Build chunks gameobjects
        float chunkMass = volumeOfMesh(meshAsset) * density / totalChunks;
        auto chunks = buildChunks(gameObject.getComponent<transform_component>(), insideMaterial, outsideMaterial, meshes, chunkMass);

        // Graph manager freezes/unfreezes blocks depending on whether they are connected to the graph or not
        graphManager.setup(chunks);

        // Connect blocks that are touching with fixed joints
        //for (size_t i = 0; i < chunks.size(); i++)
        //{
        //    connectTouchingChunks(graphManager, meshes[i].first, chunks[i], jointBreakForce);
        //}

        for (auto chunk : chunks)
        {
            eentity renderEntity{ chunk, &enttScene->registry };
            renderEntity.setParent(fractureGameObject);
        }

        //anchorChunks(gameObject.handle, anchor);

        return fractureGameObject.handle;
    }

    void fracture::anchorChunks(entity_handle gameObject, anchor anchor)
    {
        if (anchor == anchor::anchor_none)
            return;

        auto enttScene = globalApp.getCurrentScene();

        eentity entt{ gameObject, &enttScene->registry };

        auto& transform = entt.getComponent<transform_component>();
        auto bounds = getCompositeMeshBounds(entt);
        auto anchoredColliders = getAnchoredColliders(anchor, transform, bounds);

        for (auto& collider : anchoredColliders)
        {
            eentity coll{ collider, &enttScene->registry };

            coll.getComponent<physics::px_dynamic_body_component>().setKinematic(true);
        }
    }

    std::unordered_set<entity_handle> fracture::getAnchoredColliders(anchor anchor, const trs& meshTransform, const bounds& bounds)
    {
        std::unordered_set<entity_handle> anchoredChunks;
        float frameWidth = 0.01f;

        auto meshWorldCenter = localToWorld(bounds.center, meshTransform);
        auto meshWorldExtents = bounds.extents * meshTransform.scale;

        if (anchor & anchor::anchor_left)
        {
            auto right = transformDirection(meshTransform, vec3(1.0f, 0.0f, 0.0f));

            auto center = meshWorldCenter - right * meshWorldExtents.x;
            auto abs = ::abs(meshWorldExtents);
            vec3 halfExtents = { frameWidth , abs.y, abs.z };

            px_overlap_info overlapResult = physics::physics_holder::physicsRef->
                overlapBox(center, halfExtents, meshTransform.rotation);

            for (auto& res : overlapResult.results)
            {
                anchoredChunks.emplace((entity_handle)res);
            }
        }

        if (anchor & anchor::anchor_right)
        {
            auto right = transformDirection(meshTransform, vec3(1.0f, 0.0f, 0.0f));

            auto center = meshWorldCenter + right * meshWorldExtents.x;
            auto abs = ::abs(meshWorldExtents);
            vec3 halfExtents = { frameWidth , abs.y, abs.z };

            px_overlap_info overlapResult = physics::physics_holder::physicsRef->
                overlapBox(center, halfExtents, meshTransform.rotation);

            for (auto& res : overlapResult.results)
            {
                anchoredChunks.emplace((entity_handle)res);
            }
        }

        if (anchor & anchor::anchor_bottom)
        {
            auto up = transformDirection(meshTransform, vec3(0.0f, 1.0f, 0.0f));

            auto center = meshWorldCenter - up * meshWorldExtents.y;
            auto abs = ::abs(meshWorldExtents);
            vec3 halfExtents = { abs.x, frameWidth, abs.z };

            px_overlap_info overlapResult = physics::physics_holder::physicsRef->
                overlapBox(center, halfExtents, meshTransform.rotation);

            for (auto& res : overlapResult.results)
            {
                anchoredChunks.emplace((entity_handle)res);
            }
        }

        if (anchor & anchor::anchor_top)
        {
            auto up = transformDirection(meshTransform, vec3(0.0f, 1.0f, 0.0f));

            auto center = meshWorldCenter + up * meshWorldExtents.y;
            auto abs = ::abs(meshWorldExtents);
            vec3 halfExtents = { abs.x, frameWidth, abs.z };

            px_overlap_info overlapResult = physics::physics_holder::physicsRef->
                overlapBox(center, halfExtents, meshTransform.rotation);

            for (auto& res : overlapResult.results)
            {
                anchoredChunks.emplace((entity_handle)res);
            }
        }

        if (anchor & anchor::anchor_front)
        {
            auto forward = transformDirection(meshTransform, vec3(0.0f, 0.0f, 1.0f));

            auto center = meshWorldCenter - forward * meshWorldExtents.z;
            auto abs = ::abs(meshWorldExtents);
            vec3 halfExtents = { abs.x, abs.y, frameWidth };

            px_overlap_info overlapResult = physics::physics_holder::physicsRef->
                overlapBox(center, halfExtents, meshTransform.rotation);

            for (auto& res : overlapResult.results)
            {
                anchoredChunks.emplace((entity_handle)res);
            }
        }

        if (anchor & anchor::anchor_back)
        {
            auto forward = transformDirection(meshTransform, vec3(0.0f, 0.0f, 1.0f));

            auto center = meshWorldCenter + forward * meshWorldExtents.z;
            auto abs = ::abs(meshWorldExtents);
            vec3 halfExtents = { abs.x, abs.y, frameWidth };

            px_overlap_info overlapResult = physics::physics_holder::physicsRef->
                overlapBox(center, halfExtents, meshTransform.rotation);

            for (auto& res : overlapResult.results)
            {
                anchoredChunks.emplace((entity_handle)res);
            }
        }

        return anchoredChunks;
    }

    void fracture::connectTouchingChunks(chunk_graph_manager& manager, ref<submesh_asset> asset, entity_handle chunk, float jointBreakForce, float touchRadius)
    {
        auto enttScene = globalApp.getCurrentScene();
        eentity entt{ chunk, &enttScene->registry };

        auto& rb = entt.getComponent<physics::px_dynamic_body_component>();
        auto& transform = entt.getComponent<transform_component>();
        auto& mesh = entt.getComponent<mesh_component>().mesh;

        std::unordered_set<entity_handle> overlaps;

        auto& vertices = asset->positions;

        for (size_t i = 0; i < vertices.size(); i++)
        {
            trs worldPosition = transform * vertices[i];

            px_overlap_info overlapResult = physics::physics_holder::physicsRef->
                overlapSphere(worldPosition.position, touchRadius);

            for (size_t j = 0; j < overlapResult.results.size(); j++)
            {
                overlaps.emplace((entity_handle)overlapResult.results[j]);
            }
        }

        for (auto overlap : overlaps)
        {
            if (overlap != chunk)
            {
                {
                    eentity body{ overlap, &enttScene->registry };

                    auto& rbOverlap = body.getComponent<physics::px_dynamic_body_component>();

                    std::vector<PxFilterData> fd1 = getFilterData(rb.getRigidActor());
                    std::vector<PxFilterData> fd2 = getFilterData(rbOverlap.getRigidActor());

                    px_fixed_joint* joint = new px_fixed_joint(px_fixed_joint_desc{ 0.1f, 0.1f, 200.0f, 100.0f }, rb.getRigidActor(), rbOverlap.getRigidActor());

                    joint->joint->setInvInertiaScale0(0.0f);
                    joint->joint->setInvInertiaScale1(0.0f);
                    joint->joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);

                    setFilterData(rb.getRigidActor(), fd1);
                    setFilterData(rbOverlap.getRigidActor(), fd2);

                    if (manager.joints.contains(chunk))
                    {
                        manager.joints[chunk].push_back(joint);
                    }
                    else
                    {
                        std::vector<px_fixed_joint*> jointVec;
                        jointVec.push_back(joint);
                        manager.joints.emplace(chunk, jointVec);
                    }
                }
            }
        }
    }
}