// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/blast/px_blast_destructions.h"
#include "px/core/px_aggregate.h"

#if PX_BLAST_ENABLE

namespace era_engine::physics
{
    eentity buildChunk(const trs& transform,
        ref<pbr_material> insideMaterial,
        ref<pbr_material> outsideMaterial,
        std::pair<ref<submesh_asset>,
        ref<nvmesh>> mesh,
        float mass,
        float& radius,
        uint32 generation)
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

        float calculatedMass = volumeOfMesh(mesh.first);
        std::cout << (std::to_string(calculatedMass).c_str()) << "\n";

        eentity chunk = enttScene->createEntity(asset.name.c_str())
            .addComponent<transform_component>(transform)
            .addComponent<px_convex_mesh_collider_component>(&asset)
            .addComponent<px_dynamic_body_component>()
            .addComponent<nvmesh_chunk_component>(mesh.second, calculatedMass)
            .addComponent<mesh_component>(mm);

        mm->aabb.grow(aabb.minCorner);
        mm->aabb.grow(aabb.maxCorner);

        radius = mm->aabb.volume();

        mm->mesh = builder.createDXMesh();

        return chunk;
    }

    std::vector<entity_handle> buildChunks(const trs& transform,
                                           ref<pbr_material> insideMaterial,
                                           ref<pbr_material> outsideMaterial,
                                           std::vector<::std::pair<ref<submesh_asset>,
                                           ref<nvmesh>>> meshes,
                                           float chunkMass,
                                           std::vector<float>& radiuses,
                                           uint32 generation)
    {
        std::vector<entity_handle> handles;

        //px_aggregate* aggregate = new px_aggregate(meshes.size(), false);

        for (size_t i = 0; i < meshes.size(); ++i)
        {
            eentity entity = buildChunk(transform, insideMaterial, outsideMaterial, meshes[i], chunkMass, radiuses[i], generation);
            handles.push_back(entity.handle);
            //aggregate->addActor(entity.getComponent<px_dynamic_body_component>().getRigidActor());
        }

        return handles;
    }

    bool chunk_graph_manager::chunk_node::contains(entity_handle chunkNode) const
    {
        return neighbours.contains(chunkNode);
    }

    void chunk_graph_manager::chunk_node::onJointBreak()
    {
        hasBrokenLinks = true;
    }

    void chunk_graph_manager::chunk_node::update()
    {
        if (frozen && !isKinematic)
        {
            auto enttScene = globalApp.getCurrentScene();

            eentity renderEntity{ handle, &enttScene->registry };

            auto& transform = renderEntity.getComponent<transform_component>();
            auto& dynamicBody = renderEntity.getComponent<px_dynamic_body_component>();

            transform.position = frozenPos;
            transform.rotation = frozenRot;

            dynamicBody.setPhysicsPositionAndRotation(frozenPos, frozenRot);
        }
    }

    void chunk_graph_manager::chunk_node::setup(chunk_graph_manager* manager)
    {
        setupRigidbody();

        /*if(!isKinematic)
            freeze();*/

        jointToChunk.clear();
        chunkToJoint.clear();

        auto enttScene = globalApp.getCurrentScene();

        for (auto joint : manager->joints[handle])
        {
            auto chunk = *reinterpret_cast<entity_handle*>(joint->second->userData);
            jointToChunk.emplace(joint, chunk);
            chunkToJoint.emplace(chunk, joint);
        }

        for (auto& chunkNode : chunkToJoint)
        {
            neighbours.emplace(chunkNode.first);

            eentity renderEntity{ chunkNode.first, &enttScene->registry };
            auto& chunk = renderEntity.getComponent<physics::chunk_graph_manager::chunk_node>();
            if (chunk.contains(handle) == false)
            {
                chunk.neighbours.emplace(handle);
            }
        }
    }

    void chunk_graph_manager::chunk_node::addToUnfreezeQueue()
    {
        auto& queue = physics_holder::physicsRef->unfreezeBlastQueue;
        if (queue.contains((uint32)handle))
            return;
        lock lock{ physics_holder::physicsRef->blastSync };
        uint32 value = (uint32)handle;
        queue.emplace(value);
        frozen = false;
    }

    void chunk_graph_manager::chunk_node::remove(entity_handle chunkNode)
    {
        chunkToJoint.erase(chunkNode);
        neighbours.erase(chunkNode);
    }

    void chunk_graph_manager::chunk_node::processDamage(PxVec3 impulse)
    {
        if (spliteGeneration < maxSpliteGeneration)
        {
            if (impulse.magnitude() > 1.0f)
            {
                blastFractureQueue.pushEvent({ (uint32_t)handle });
            }
        }
    }

    void chunk_graph_manager::chunk_node::cleanBrokenLinks()
    {
        std::vector<px_fixed_joint*> brokenLinks;
        auto enttScene = globalApp.getCurrentScene();

        for (auto& joint : jointToChunk)
        {
            if (joint.first->joint && (joint.first->joint->getConstraintFlags() & PxConstraintFlag::eBROKEN))
            {
                brokenLinks.push_back(joint.first);
            }
        }

        for (auto link : brokenLinks)
        {
            auto body = jointToChunk[link];

            jointToChunk.erase(link);
            chunkToJoint.erase(body);

            eentity renderEntity{ body, &enttScene->registry };

            renderEntity.getComponent<chunk_graph_manager::chunk_node>().remove(handle);

            neighbours.erase(body);
        }

        hasBrokenLinks = false;
    }

    void chunk_graph_manager::chunk_node::setupRigidbody()
    {
        auto enttScene = globalApp.getCurrentScene();

        if (!enttScene->registry.size())
            return;

        eentity renderEntity{ handle, &enttScene->registry };

        auto& rb = renderEntity.getComponent<px_dynamic_body_component>();

        rb.setAngularDamping(0.1f);
        rb.setLinearDamping(0.2f);
        rb.setThreshold(0.02f, 0.02f);

        auto dyn = rb.getRigidDynamic();
        dyn->setMaxDepenetrationVelocity(3.0f);
        dyn->setSolverIterationCounts(32, 32);
        rb.updateMassAndInertia(renderEntity.getComponent<nvmesh_chunk_component>().mass);

        if (rb.isKinematicBody())
            isKinematic = true;
    }

    void chunk_graph_manager::chunk_node::unfreeze()
    {
        auto enttScene = globalApp.getCurrentScene();

        eentity renderEntity{ handle, &enttScene->registry };
        auto& rb = renderEntity.getComponent<px_dynamic_body_component>();
        if (rb.isKinematicBody())
            return;
        rb.setConstraints(0);
        rb.setGravity(true);
        rb.setFilterMask(-1, -1);

        frozen = false;
    }

    void chunk_graph_manager::chunk_node::freeze()
    {
        frozen = true;

        auto enttScene = globalApp.getCurrentScene();

        eentity renderEntity{ handle, &enttScene->registry };
        auto& transform = renderEntity.getComponent<transform_component>();

        frozenPos = transform.position;
        frozenRot = transform.rotation;

        auto& rb = renderEntity.getComponent<physics::px_dynamic_body_component>();
        rb.setGravity(false);
        rb.setConstraints(PX_FREEZE_ALL);
        rb.setFilterMask(16, 16);
    }

    void chunk_graph_manager::setup(std::vector<entity_handle> bodies, uint32 generation)
    {
        nbNodes = bodies.size();
        nodes.reserve(nbNodes);

        auto enttScene = globalApp.getCurrentScene();

        for (size_t i = 0; i < bodies.size(); i++)
        {
            if (bodies[i] == null_entity)
                continue;

            eentity renderEntity{ bodies[i], &enttScene->registry };

            if (!renderEntity.hasComponent<chunk_node>())
            {
                renderEntity.addComponent<chunk_node>(bodies[i], generation);
            }
        }

        for (size_t i = 0; i < bodies.size(); i++)
        {
            eentity renderEntity{ bodies[i], &enttScene->registry };

            auto newNode = renderEntity.getComponentIfExists<chunk_node>();
            newNode->setup(this);
            nodes.emplace_back(bodies[i]);
        }
    }

    void chunk_graph_manager::update()
    {
        /*auto enttScene = globalApp.getCurrentScene();

        std::vector<std::pair<entity_handle, px_fixed_joint*>> brokenLinks;

        for (auto& node : joints)
        {
            eentity renderEntity{ node.first, &enttScene->registry };

            for (auto& joint : node.second)
            {
                if (joint->joint->getConstraintFlags() & PxConstraintFlag::eBROKEN)
                {
                    brokenLinks.push_back({node.first, joint});
                    renderEntity.getComponent<chunk_graph_manager::chunk_node>().onJointBreak();
                }

            }
        }

        bool runSearch = false;

        for (size_t i = 0; i < nbNodes; i++)
        {
            auto node = nodes[i];

            eentity renderEntity{ node, &enttScene->registry };
            auto nodeComponent = renderEntity.getComponentIfExists<chunk_node>();
            nodeComponent->update();
            if (nodeComponent->hasBrokenLinks)
            {
                nodeComponent->cleanBrokenLinks();

                runSearch = true;
            }
        }

        for (auto&[h, j] : brokenLinks)
        {
            remove(h, j);
        }

        if (runSearch)
            searchGraph(nodes);*/
    }

    void chunk_graph_manager::searchGraph(std::vector<entity_handle> objects)
    {
        std::vector<chunk_node*> anchors;
        std::unordered_set<chunk_node*> search;

        for (size_t i = 0; i < nbNodes; i++)
        {
            auto enttScene = globalApp.getCurrentScene();

            eentity renderEntity{ objects[i], &enttScene->registry };
            if (auto rb = renderEntity.getComponentIfExists<px_dynamic_body_component>())
            {
                if (renderEntity.getComponentIfExists<chunk_graph_manager::chunk_node>()->isKinematic)
                    anchors.push_back(renderEntity.getComponentIfExists<chunk_graph_manager::chunk_node>());
            }

            search.emplace(renderEntity.getComponentIfExists<chunk_graph_manager::chunk_node>());
        }

        for (auto o : anchors)
        {
            if (search.contains(o))
            {
                std::unordered_set<chunk_node*> subVisited;
                traverse(o, search, subVisited);

                std::unordered_set<chunk_node*> temp;

                for (auto s : search)
                {
                    if (!subVisited.contains(s))
                        temp.emplace(s);
                }
                search = temp;
            }
        }
        for (auto sub : search)
        {
            sub->unfreeze();
        }
    }

    void chunk_graph_manager::remove(entity_handle handle, px_fixed_joint* joint)
    {
        auto& jointsList = joints[handle];
        auto end = jointsList.end();
        for (auto iter = jointsList.begin(); iter != end; ++iter)
        {
            if (*iter == joint)
                jointsList.erase(iter);
        }
    }

    void chunk_graph_manager::traverse(chunk_node* o, std::unordered_set<chunk_node*>& search, std::unordered_set<chunk_node*>& visited)
    {
        if (search.contains(o) && visited.contains(o) == false)
        {
            auto enttScene = globalApp.getCurrentScene();

            visited.emplace(o);

            for (auto n : o->neighbours)
            {
                eentity renderEntity{ n, &enttScene->registry };
                traverse(renderEntity.getComponentIfExists<chunk_graph_manager::chunk_node>(), search, visited);
            }
        }
    }

}
#endif
