// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "px/core/px_physics_engine.h"

#if PX_BLAST_ENABLE

#include "px/core/px_extensions.h"
#include "px/physics/px_joint.h"
#include "px/blast/px_blast_utils.h"
#include "px/blast/px_nvmesh.h"
#include "px/blast/px_blast_fracture.h"

#include "rendering/main_renderer.h"

#include "scripting/native_scripting_linker.h"

#include "scene/scene.h"

#include "asset/mesh_postprocessing.h"

#include "application.h"

#include <NvBlast.h>

namespace era_engine::physics
{
    eentity buildChunk(const trs& transform,
        ref<pbr_material> insideMaterial,
        ref<pbr_material> outsideMaterial,
        std::pair<ref<submesh_asset>,
        ref<nvmesh>> mesh,
        float mass,
        float& radius,
        uint32 generation);

    std::vector<entity_handle> buildChunks(const trs& transform, ref<pbr_material> insideMaterial,
                                           ref<pbr_material> outsideMaterial, 
                                           std::vector<::std::pair<ref<submesh_asset>,
                                           ref<nvmesh>>> meshes, 
                                           float chunkMass,
                                           std::vector<float>& radiuses,
                                           uint32 generation = 0);

    static inline uint32 id = 0;
    
    static inline uint32 maxSpliteGeneration = 3U;

    static inline spin_lock fractureSyncObj;

    struct chunk_graph_manager
    {
        struct chunk_node
        {
            chunk_node() = default;

            chunk_node(entity_handle handl, uint32 generation) : handle(handl), spliteGeneration(generation) {}

            bool contains(entity_handle chunkNode) const;

            void onJointBreak();

            void update();

            void setup(chunk_graph_manager* manager);

            void addToUnfreezeQueue();

            void remove(entity_handle chunkNode);

            void processDamage(PxVec3 impulse);

            void cleanBrokenLinks();

            void setupRigidbody();

            void unfreeze();

            void freeze();

            std::unordered_set<entity_handle> neighbours;

            std::unordered_map<px_fixed_joint*, entity_handle> jointToChunk;
            std::unordered_map<entity_handle, px_fixed_joint*> chunkToJoint;

            entity_handle* neighboursArray = nullptr;

            entity_handle handle{};

            bool frozen = true;
            bool isKinematic = false;
            bool hasBrokenLinks = false;

            vec3 frozenPos{};
            quat frozenRot{};

            volatile uint32 spliteGeneration = 0;
        };

        void setup(std::vector<entity_handle> bodies, uint32 generation = 0);

        void update();

        void searchGraph(std::vector<entity_handle> objects);

        void remove(entity_handle handle, px_fixed_joint* joint);

        void traverse(chunk_node* o, std::unordered_set<chunk_node*>& search, std::unordered_set<chunk_node*>& visited);

        std::vector<entity_handle> nodes;
        std::unordered_map<entity_handle, std::vector<px_fixed_joint*>> joints;
        size_t nbNodes{};
    };
}

#endif