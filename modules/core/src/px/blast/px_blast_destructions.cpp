// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/blast/px_blast_destructions.h"

#if PX_BLAST_ENABLE

namespace era_engine::physics
{
    eentity buildChunk(const trs& transform, ref<pbr_material> insideMaterial, ref<pbr_material> outsideMaterial, std::pair<ref<submesh_asset>, ref<nvmesh>> mesh, float mass, uint32 generation)
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
            .addComponent<physics::px_convex_mesh_collider_component>(&asset)
            .addComponent<physics::px_dynamic_body_component>()
            .addComponent<nvmesh_chunk_component>(mesh.second)
            .addComponent<mesh_component>(mm);

        mm->aabb.grow(aabb.minCorner);
        mm->aabb.grow(aabb.maxCorner);

        mm->mesh = builder.createDXMesh();

        return chunk;
    }

    std::vector<entity_handle> buildChunks(const trs& transform, ref<pbr_material> insideMaterial, ref<pbr_material> outsideMaterial, std::vector<::std::pair<ref<submesh_asset>, ref<nvmesh>>> meshes, float chunkMass, uint32 generation)
    {
        std::vector<entity_handle> handles;

        for (size_t i = 0; i < meshes.size(); ++i)
        {
            handles.push_back(buildChunk(transform, insideMaterial, outsideMaterial, meshes[i], chunkMass, generation).handle);
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
        if (frozen)
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

        //if(!isKinematic)
        //    freeze();

        jointToChunk.clear();
        chunkToJoint.clear();

        if (!manager->joints.contains(handle))
            return;

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

            link->joint->setInvInertiaScale0(0.0f);
            link->joint->setInvInertiaScale1(0.0f);

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

        constexpr float chunkMass = 3.0f;

        rb.setMaxAngularVelosity(100.0f);
        rb.setAngularDamping(0.01f);
        rb.setLinearDamping(0.01f);

        auto dyn = rb.getRigidDynamic();
        dyn->setSolverIterationCounts(4, 4);
        dyn->setMaxDepenetrationVelocity(3.0f);
        dyn->setMaxContactImpulse(1000.0f);

        rb.updateMassAndInertia(chunkMass);
    }

    void chunk_graph_manager::chunk_node::unfreeze()
    {
        frozen = false;

        auto enttScene = globalApp.getCurrentScene();

        eentity renderEntity{ handle, &enttScene->registry };

        renderEntity.getComponent<physics::px_dynamic_body_component>().setConstraints(0);
    }

    void chunk_graph_manager::chunk_node::freeze()
    {
        frozen = true;

        auto enttScene = globalApp.getCurrentScene();

        eentity renderEntity{ handle, &enttScene->registry };

        frozenPos = renderEntity.getComponent<transform_component>().position;
        frozenRot = renderEntity.getComponent<transform_component>().rotation;

        renderEntity.getComponent<physics::px_dynamic_body_component>().setConstraints(PX_FREEZE_ALL);
    }

    void chunk_graph_manager::setup(std::vector<entity_handle> bodies, uint32 generation)
    {
        nbNodes = bodies.size();
        nodes.reserve(nbNodes);

        physics_holder::physicsRef->unfreezeBlastQueue.reserve(nbNodes * 5);

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
        auto enttScene = globalApp.getCurrentScene();

        for (auto& node : joints)
        {
            eentity renderEntity{ node.first, &enttScene->registry };

            for (auto& joint : node.second)
            {
                if (joint->joint->getConstraintFlags() & PxConstraintFlag::eBROKEN)
                    renderEntity.getComponent<chunk_graph_manager::chunk_node>().onJointBreak();
            }
        }

        bool runSearch = false;

        for (size_t i = 0; i < nbNodes; i++)
        {
            auto node = nodes[i];

            eentity renderEntity{ node, &enttScene->registry };
            auto nodeComponent = renderEntity.getComponentIfExists<chunk_node>();

            if (nodeComponent->hasBrokenLinks)
            {
                nodeComponent->cleanBrokenLinks();

                runSearch = true;
            }
        }

        if (runSearch)
            searchGraph(nodes);
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
            sub->addToUnfreezeQueue();
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
                traverse(renderEntity.getComponentIfExists<physics::chunk_graph_manager::chunk_node>(), search, visited);
            }
        }
    }

}
#endif
