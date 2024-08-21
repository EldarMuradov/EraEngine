// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "scene/scene.h"

#include "px/blast/px_blast_utils.h"

#if PX_BLAST_ENABLE

#include <NvBlastExtAuthoringFractureTool.h>

namespace era_engine
{
    struct submesh_asset;
}

namespace era_engine::physics
{
    struct nvmesh;

    struct fracture_tool
    {
        fracture_tool();

        ~fracture_tool();

        Nv::Blast::FractureTool* fractureTool = nullptr;
    };

    struct randomGenerator : Nv::Blast::RandomGeneratorBase
    {
        virtual float getRandomValue();

        virtual void seed(int32 seed); //unused

        static inline unsigned int seedResult = 5489U;
    };

    struct voronoi_sites_generator
    {
        voronoi_sites_generator(ref<nvmesh> mesh);

        void release();

        ref<randomGenerator> rndGen = nullptr;
        Nv::Blast::VoronoiSitesGenerator* generator = nullptr;
    };

    std::vector<std::pair<ref<submesh_asset>, ref<nvmesh>>> fractureMeshesInNvblast(int totalChunks, ref<nvmesh> nvMesh, bool replace = false);

    struct chunk_graph_manager;

    struct fracture
    {
        entity_handle fractureGameObject(ref<submesh_asset> meshAsset, const eentity& gameObject, anchor anchor, unsigned int seed, int totalChunks, 
            ref<pbr_material> insideMaterial, ref<pbr_material> outsideMaterial, float jointBreakForce, float density);

        void anchorChunks(entity_handle gameObject, anchor anchor);

        std::unordered_set<entity_handle> getAnchoredColliders(anchor anchor, const trs& meshTransform, const bounds& bounds);

        void connectTouchingChunks(chunk_graph_manager& manager, ref<submesh_asset> asset, entity_handle chunk, float jointBreakForce, std::vector<float>& radiuses, int index, float touchRadius = 0.01f);

        std::unordered_set<chunkPair> jointPairs;
    };
}

#endif