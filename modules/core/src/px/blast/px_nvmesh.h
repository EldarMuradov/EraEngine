#pragma once

#include "px/core/px_physics_engine.h"

#if PX_BLAST_ENABLE

#include <NvBlast.h>

#include <NvBlastExtAuthoring.h>
#include <NvBlastExtAuthoringMesh.h>
#include <NvBlastExtAuthoringMeshCleaner.h>

namespace era_engine::physics
{
    using namespace physx;

    struct nvmesh
    {
        nvmesh(std::vector<PxVec3> verts, std::vector<PxVec3> norms,
            std::vector<PxVec2> uvis, std::vector<uint32_t> inds);

        nvmesh();

        nvmesh(Nv::Blast::Mesh* inMesh);

        ~nvmesh();

        void release();

        void cleanMesh();

        std::vector<PxVec3> vertices;
        std::vector<PxVec3> normals;
        std::vector<PxVec2> uvs;
        std::vector<uint32_t> indices;

        Nv::Blast::Mesh* mesh = nullptr;
    };

    struct nvmesh_chunk_component : px_physics_component_base
    {
        nvmesh_chunk_component() = default;
        nvmesh_chunk_component(uint32 handle, ref<nvmesh> inputMesh, float chunkMass) : px_physics_component_base(handle), mesh(inputMesh), mass(chunkMass) {}
        ~nvmesh_chunk_component() {}

        virtual void release(bool release = false) override { PX_RELEASE(mesh) }

        ref<nvmesh> mesh = nullptr;
        float mass = 1.0f;
    };
}

#endif