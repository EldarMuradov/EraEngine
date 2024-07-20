#include "pch.h"

#include "px/blast/px_nvmesh.h"

namespace era_engine::physics
{
    nvmesh::nvmesh(std::vector<physx::PxVec3> verts, std::vector<physx::PxVec3> norms, std::vector<physx::PxVec2> uvis, std::vector<uint32_t> inds)
        : vertices(verts),
        normals(norms),
        uvs(uvis),
        indices(inds)
    {
        mesh = NvBlastExtAuthoringCreateMesh((NvcVec3*)vertices.data(), (NvcVec3*)normals.data(), (NvcVec2*)uvs.data(),
            vertices.size(), indices.data(), indices.size());
    }

    nvmesh::nvmesh()
    {
        mesh = NvBlastExtAuthoringCreateMesh((NvcVec3*)vertices.data(), (NvcVec3*)normals.data(), (NvcVec2*)uvs.data(),
            vertices.size(), indices.data(), indices.size());
    }

    nvmesh::nvmesh(Nv::Blast::Mesh* inMesh)
    {
        mesh = inMesh;

        size_t nbVerts = mesh->getVerticesCount();
        auto verts = mesh->getVertices();

        for (size_t i = 0; i < nbVerts; i++)
        {
            vertices.push_back(PxVec3(verts[i].p.x, verts[i].p.y, verts[i].p.z));
            uvs.push_back(PxVec2(verts[i].uv->x, verts[i].uv->y));

            normals.push_back(PxVec3(verts[i].n.x, verts[i].n.y, verts[i].n.z));
        }
    }

    nvmesh::~nvmesh()
    {
        release();
    }

    void nvmesh::release()
    {
        vertices.clear();
        normals.clear();
        uvs.clear();
        indices.clear();

        if (mesh)
            mesh->release();
    }

    void nvmesh::cleanMesh()
    {
        Nv::Blast::MeshCleaner* cleaner = NvBlastExtAuthoringCreateMeshCleaner();
        if (cleaner)
        {
            mesh = cleaner->cleanMesh(mesh);

            cleaner->release();
        }
    }

}