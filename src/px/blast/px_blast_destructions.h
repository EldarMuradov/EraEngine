#pragma once

#include <map>

#include <px/core/px_physics_engine.h>
#include <rendering/main_renderer.h>
#include <px/core/px_extensions.h>
#include <application.h>
#include <scripting/native_scripting_linker.h>
#include <scene/scene.h>
#include <asset/mesh_postprocessing.h>
#include <random>
#include <iomanip>

#include <NvBlast.h>

#include <NvBlastTk.h>
#include <NvBlastExtTkSerialization.h>

#include "NvBlastGlobals.h"

#include <NvBlastExtDamageShaders.h>
#include <NvBlastExtStressSolver.h>
#include "NvBlastExtImpactDamageManager.h"
#include <NvBlastExtPxManager.h>
#include <NvBlastExtPxAsset.h>
#include <NvBlastExtPxTask.h>
#include <NvBlastExtSerialization.h>
#include <NvBlastExtPxActor.h>
#include <NvBlastExtPxFamily.h>
#include <NvBlastExtPxStressSolver.h>
#include <NvBlastExtPxListener.h>
#include "NvBlastExtAuthoring.h"
#include <NvBlastExtPxAsset.h>
#include "NvBlastExtLlSerialization.h"
#include <NvBlastExtSync.h>
#include <NvBlastExtAuthoringMesh.h>
#include <NvBlastExtAuthoringFractureTool.h>
#include <NvBlastExtAuthoringMeshCleaner.h>
#include <NvBlastExtAuthoringTypes.h>
#include <NvBlastExtAuthoringBondGenerator.h>

namespace physics
{
    struct nvmesh;

    static inline int id = 0;

    enum class anchor
    {
        None = 0,
        Left = 1,
        Right = 2,
        Bottom = 4,
        Top = 8,
        Front = 16,
        Back = 32
    };

    struct bounds
    {
        vec3 center;
        vec3 extents;
    };

    struct nvmesh
    {
        ::std::vector<physx::PxVec3> vertices;
        ::std::vector<physx::PxVec3> normals;
        ::std::vector<physx::PxVec2> uvs;
        ::std::vector<uint32_t> indices;

        Nv::Blast::Mesh* mesh = nullptr;

        nvmesh(::std::vector<physx::PxVec3> verts, ::std::vector<physx::PxVec3> norms,
            ::std::vector<physx::PxVec2> uvis, ::std::vector<uint32_t> inds)
          : vertices(verts),
            normals(norms),
            uvs(uvis),
            indices(inds)
        {
            mesh = NvBlastExtAuthoringCreateMesh((NvcVec3*)vertices.data(), (NvcVec3*)normals.data(), (NvcVec2*)uvs.data(),
                vertices.size(), indices.data(), indices.size());
        }

        nvmesh()
        {
            mesh = NvBlastExtAuthoringCreateMesh((NvcVec3*)vertices.data(), (NvcVec3*)normals.data(), (NvcVec2*)uvs.data(),
                vertices.size(), indices.data(), indices.size());
        }

        nvmesh(Nv::Blast::Mesh* inMesh)
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

        void release()
        {
            vertices.clear();
            normals.clear();
            uvs.clear();
            indices.clear();

            RELEASE_PTR(mesh);
        }

        void cleanMesh()
        {
            Nv::Blast::MeshCleaner* cleaner = NvBlastExtAuthoringCreateMeshCleaner();
            mesh = cleaner->cleanMesh(mesh);
        }
    };

    inline ref<submesh_asset> createRenderMesh(const physics::nvmesh& simpleMesh)
    {
        ref<submesh_asset> asset = make_ref<submesh_asset>();

        auto defaultmat = createPBRMaterialAsync({ "", "" });

        std::vector<vec3> positions; positions.reserve(1 << 16);
        std::vector<vec2> uvs; uvs.reserve(1 << 16);
        std::vector<vec3> normals; normals.reserve(1 << 16);

        std::vector<vec3> positionCache; positionCache.reserve(16);
        std::vector<vec2> uvCache; uvCache.reserve(16);
        std::vector<vec3> normalCache; normalCache.reserve(16);

        size_t nbTriangles = simpleMesh.indices.size() / 3;

        for (auto vert : simpleMesh.vertices)
        {
            asset->positions.push_back(physx::createVec3(vert));
        }
        for (auto uv : simpleMesh.uvs)
        {
            asset->uvs.push_back(physx::createVec2(uv));
        }

        for (uint32_t i = 0; i < simpleMesh.indices.size(); i += 3)
        {
            asset->triangles.push_back(indexed_triangle16{
                (uint16)simpleMesh.indices[i + 0],
                (uint16)simpleMesh.indices[i + 1],
                (uint16)simpleMesh.indices[i + 2] });
        }

        generateNormalsAndTangents(asset, mesh_flag_default);

        return asset;
    }

    struct fracture_tool
    {
        Nv::Blast::FractureTool* fractureTool = nullptr;

        fracture_tool()
        {
            fractureTool = NvBlastExtAuthoringCreateFractureTool();
        }
    };

    struct randomGenerator : Nv::Blast::RandomGeneratorBase
    {
        ::std::random_device rd;
        //static inline ::std::mt19937::result_type seedResult;
        static inline int seedResult;

        virtual float getRandomValue()
        {
            ::std::mt19937 eng(rd());
            eng.seed(seedResult);
            ::std::uniform_real_distribution<float> distr(0.0f, 1.0f);

            return distr(eng);
        }

        virtual void seed(int32_t seed)
        {
            seedResult = seed;
        }
    };

    struct voronoi_sites_generator
    {
        randomGenerator* rndGen = nullptr;
        Nv::Blast::VoronoiSitesGenerator* generator = nullptr;

        voronoi_sites_generator(nvmesh* mesh)
        {
            rndGen = new randomGenerator();
            generator = NvBlastExtAuthoringCreateVoronoiSitesGenerator(mesh->mesh, rndGen);
        }

        void release()
        {
            RELEASE_PTR(rndGen)
            RELEASE_PTR(generator)
        }
    };

    struct chunk_node
    {
        ::std::unordered_set<chunk_node*> neighbours;
        chunk_node** neighboursArray = nullptr;

        entity_handle handle;

        bool frozen;

        vec3 frozenPos;
        quat forzenRot;

        chunk_node() = default;

        chunk_node(entity_handle handl) : handle(handl) {}

        bool contains(chunk_node* chunkNode) const
        {
            return neighbours.contains(chunkNode);
        }

        void update()
        {
            if (frozen)
            {
                auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

                eentity renderEntity{ handle, &enttScene->registry };

                renderEntity.getComponent<transform_component>().position = frozenPos;
                renderEntity.getComponent<transform_component>().rotation = forzenRot;
            }
        }

        void setup()
        {
            freeze();
        }

        void unfreeze()
        {
            frozen = false;
        }

        void remove(chunk_node* chunkNode)
        {
            neighbours.erase(chunkNode);
        }

        void freeze()
        {
            frozen = true;

            auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

            eentity renderEntity{ handle, &enttScene->registry };

            frozenPos = renderEntity.getComponent<transform_component>().position;
            forzenRot = renderEntity.getComponent<transform_component>().rotation;
        }
    };

    struct chunk_graph_manager
    {
        chunk_node* nodes = nullptr;
        size_t nbNodes{};

        void setup(::std::vector<entity_handle> bodies)
        {
            nbNodes = bodies.size();
            nodes = new chunk_node[nbNodes];
            for (int i = 0; i < bodies.size(); i++)
            {
                auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

                eentity renderEntity{ bodies[i], &enttScene->registry };

                if (auto node = renderEntity.getComponentIfExists<chunk_node>())
                {
                    node->setup();
                    nodes[i] = *node;
                }
                else
                {
                    renderEntity.addComponent<chunk_node>(bodies[i]);

                    auto newNode = renderEntity.getComponentIfExists<chunk_node>();
                    newNode->setup();
                    nodes[i] = *newNode;
                }
            }
        }

        void update()
        {
            bool runSearch = false;

            if (runSearch)
                searchGraph(nodes);
        }

        void searchGraph(chunk_node* objects)
        {
            ::std::vector<chunk_node*> anchors;
            ::std::unordered_set<chunk_node*> search;

            for (size_t i = 0; i < nbNodes; i++)
            {
                auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

                eentity renderEntity{ objects[i].handle, &enttScene->registry };
                if (auto rb = renderEntity.getComponentIfExists<px_rigidbody_component>())
                {
                    if (rb->type == px_rigidbody_type::Static)
                        anchors.push_back(&objects[i]);
                }

                search.emplace(&objects[i]);
            }

            auto index = 0;
            for (auto o : anchors)
            {
                if (search.contains(o))
                {
                    ::std::unordered_set<chunk_node*> subVisited;
                    traverse(o, search, subVisited);

                    ::std::unordered_set<chunk_node*> temp;

                    for (auto s : search)
                    {
                        if (!subVisited.contains(s))
                            temp.emplace(s);
                    }
                    search.clear();
                    search = temp;
                }
            }
            for (auto sub : search)
            {
                sub->unfreeze();
            }
        }

        void traverse(chunk_node* o, ::std::unordered_set<chunk_node*> search, ::std::unordered_set<chunk_node*> visited)
        {
            if (search.contains(o) && visited.contains(o) == false)
            {
                visited.emplace(o);

                for (auto n : o->neighbours)
                {
                    traverse(n, search, visited);
                }
            }
        }
    };

    inline bounds toBounds(::std::vector<vec3> vertices)
    {
        auto min = vec3(INFINITE);
        auto max = vec3(-INFINITE);

        for (int i = 0; i < vertices.size(); i++)
        {
            min = ::min(vertices[i], min);
            max = ::min(vertices[i], max);
        }

        return bounds((max - min) / 2 + min, max - min);
    }

    inline ::std::vector<physx::PxVec3> createStdVectorPxVec3(const ::std::vector<vec3> vec)
    {
        ::std::vector<physx::PxVec3> v;
        v.reserve(vec.size());
        for (size_t i = 0; i < vec.size(); i++)
        {
            v.push_back(physx::createPxVec3(vec[i]));
        }

        return v;
    }

    inline ::std::vector<physx::PxVec2> createStdVectorPxVec2(const ::std::vector<vec2> vec)
    {
        ::std::vector<physx::PxVec2> v;
        v.reserve(vec.size());
        for (size_t i = 0; i < vec.size(); i++)
        {
            v.push_back(physx::createPxVec2(vec[i]));
        }

        return v;
    }

    inline float signedVolumeOfTriangle(const vec3& p1, const vec3& p2, const vec3& p3)
    {
        float v321 = p3.x * p2.y * p1.z;
        float v231 = p2.x * p3.y * p1.z;
        float v312 = p3.x * p1.y * p2.z;
        float v132 = p1.x * p3.y * p2.z;
        float v213 = p2.x * p1.y * p3.z;
        float v123 = p1.x * p2.y * p3.z;
        return (1.0f / 6.0f) * (-v321 + v231 + v312 - v132 - v213 + v123);
    }

    inline float volumeOfMesh(ref<submesh_asset> mesh)
    {
        float volume = 0.0f;

        auto vertices = mesh->positions;
        auto triangles = mesh->triangles;

        for (int i = 0; i < mesh->triangles.size(); i ++)
        {
            vec3 p1 = vertices[triangles[i].a];
            vec3 p2 = vertices[triangles[i].b];
            vec3 p3 = vertices[triangles[i].c];

            volume += signedVolumeOfTriangle(p1, p2, p3);
        }

        return ::abs(volume);
    }

    inline ::std::vector<uint32_t> generateIndices(Nv::Blast::Triangle* triangles, size_t nbTriangles)
    {
        struct vertex
        {
            vec3 position;
            vec3 normal;
            vec2 uv;

            bool operator==(const vertex& other) const
            {
                return position.x == other.position.x && position.y == other.position.y && position.z == other.position.z
                    && normal.x == other.normal.x && normal.y == other.normal.y && normal.z == other.normal.z
                    && uv.x == other.uv.x && uv.y == other.uv.y;
            }
        };

        ::std::vector<vertex> vertices;
        ::std::vector<uint32_t> indices;

        for (size_t i = 0; i < nbTriangles; i++)
        {
            auto& triangle = triangles[i];

            {
                vertex vertex = { 
                    vec3(triangle.a.p.x, triangle.a.p.y, triangle.a.p.z),
                    vec3(triangle.a.n.x, triangle.a.n.y, triangle.a.n.z),
                    vec2(triangle.a.uv->x, triangle.a.uv->y) };

                auto it = std::find(vertices.begin(), vertices.end(), vertex);
                if (it != vertices.end())
                {
                    // Vertex already exists in the vertices vector, so we just need to add the index
                    indices.push_back(std::distance(vertices.begin(), it));
                }
                else
                {
                    // New vertex, so we need to add it to the vertices vector and add the index
                    vertices.push_back(vertex);
                    indices.push_back(vertices.size() - 1);
                }
            }

            {
                vertex vertex = {
                    vec3(triangle.b.p.x, triangle.b.p.y, triangle.b.p.z),
                    vec3(triangle.b.n.x, triangle.b.n.y, triangle.b.n.z),
                    vec2(triangle.b.uv->x, triangle.b.uv->y) };

                auto it = std::find(vertices.begin(), vertices.end(), vertex);
                if (it != vertices.end())
                {
                    // Vertex already exists in the vertices vector, so we just need to add the index
                    indices.push_back(std::distance(vertices.begin(), it));
                }
                else
                {
                    // New vertex, so we need to add it to the vertices vector and add the index
                    vertices.push_back(vertex);
                    indices.push_back(vertices.size() - 1);
                }
            }

            {
                vertex vertex = {
                    vec3(triangle.c.p.x, triangle.c.p.y, triangle.c.p.z),
                    vec3(triangle.c.n.x, triangle.c.n.y, triangle.c.n.z),
                    vec2(triangle.c.uv->x, triangle.c.uv->y) };

                auto it = std::find(vertices.begin(), vertices.end(), vertex);
                if (it != vertices.end())
                {
                    // Vertex already exists in the vertices vector, so we just need to add the index
                    indices.push_back(std::distance(vertices.begin(), it));
                }
                else
                {
                    // New vertex, so we need to add it to the vertices vector and add the index
                    vertices.push_back(vertex);
                    indices.push_back(vertices.size() - 1);
                }
            }
        }

        return indices;
    }

    struct fracture
    {
        entity_handle fractureGameObject(ref<submesh_asset> meshAsset, const eentity& gameObject, anchor anchor, int seed, int totalChunks, ref<pbr_material> insideMaterial, ref<pbr_material> outsideMaterial, float jointBreakForce, float density)
        {
            // Translate all meshes to one world mesh
            //auto mesh = getWorldMesh(gameObject);

            randomGenerator::seedResult = seed;

            ::std::vector<uint32_t> indices;
            for (int i = 0; i < meshAsset->triangles.size(); i++)
            {
                indices.push_back(meshAsset->triangles[i].a);
                indices.push_back(meshAsset->triangles[i].b);
                indices.push_back(meshAsset->triangles[i].c);
            }

            auto nvMesh = new nvmesh(
                createStdVectorPxVec3(meshAsset->positions),
                createStdVectorPxVec3(meshAsset->normals),
                createStdVectorPxVec2(meshAsset->uvs),
                indices
            );

            auto meshes = fractureMeshesInNvblast(totalChunks, nvMesh);

            // Build chunks gameobjects
            float chunkMass = volumeOfMesh(meshAsset) * density / totalChunks;
            auto chunks = buildChunks(insideMaterial, outsideMaterial, meshes, chunkMass);

            //// Connect blocks that are touching with fixed joints
            //for(auto chunk : chunks)
            //{
            //    connectTouchingChunks(chunk, jointBreakForce);
            //}

            // Set anchored chunks as kinematic
            anchorChunks(gameObject, anchor);

            auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

            eentity fractureGameObject = enttScene->createEntity("Fracture")
                .addComponent<transform_component>(vec3(0.0f), quat::identity, vec3(1.f));

            for(auto chunk : chunks)
            {
                eentity renderEntity{ chunk, &enttScene->registry };
                renderEntity.setParent(fractureGameObject);
            }

            // Graph manager freezes/unfreezes blocks depending on whether they are connected to the graph or not
            auto graphManager = fractureGameObject.addComponent<chunk_graph_manager>().getComponent<chunk_graph_manager>();
            graphManager.setup(chunks);

            return fractureGameObject.handle;
        }

        void anchorChunks(const eentity& gameObject, anchor anchor)
        {

        }

        ::std::vector<entity_handle> buildChunks(ref<pbr_material> insideMaterial, ref<pbr_material> outsideMaterial, ::std::vector<ref<submesh_asset>> meshes, float chunkMass)
        {
            ::std::vector<entity_handle> handles;

            for (size_t i = 0; i < meshes.size(); i++)
            {
                handles.push_back(buildChunk(insideMaterial, outsideMaterial, meshes[i], chunkMass).handle);
            }

            return handles;
        }

        ::std::vector<ref<submesh_asset>> fractureMeshesInNvblast(int totalChunks, nvmesh* nvMesh)
        {
            fracture_tool* fractureTool = new fracture_tool();
            fractureTool->fractureTool->setRemoveIslands(false);
            fractureTool->fractureTool->setSourceMeshes(&nvMesh->mesh, 1);
            voronoi_sites_generator* generator = new voronoi_sites_generator(nvMesh);

            generator->generator->uniformlyGenerateSitesInMesh(totalChunks);

            const NvcVec3* sites;
            size_t nbSites = generator->generator->getVoronoiSites(sites);
            for (size_t i = 0; i < fractureTool->fractureTool->getChunkCount(); i++)
            {
                int result = fractureTool->fractureTool->voronoiFracturing(0, nbSites, sites, false);
            }

            fractureTool->fractureTool->finalizeFracturing();

            // Extract meshes
            size_t meshCount = fractureTool->fractureTool->getChunkCount();
            ::std::vector<ref<submesh_asset>> meshes;
            meshes.reserve(meshCount);
            for (size_t i = 0; i < meshCount; i++)
            {
                meshes.push_back(extractChunkMesh(fractureTool, i));
            }

            return { meshes };
        }

        ::std::unordered_set<entity_handle> getAnchoredColliders(anchor anchor, trs meshTransform, bounds bounds)
        {
            ::std::unordered_set<entity_handle> anchoredChunks;
            float frameWidth = 0.01f;
          
            return anchoredChunks;
        }

        ref<submesh_asset> extractChunkMesh(fracture_tool* fractureTool, int index)
        {
            Nv::Blast::Triangle* trigs;
            size_t nbTrigs = fractureTool->fractureTool->getBaseMesh(index, trigs);
            nvmesh* mesh;

            ::std::vector<physx::PxVec3> poss;
            ::std::vector<physx::PxVec3> norms;
            ::std::vector<physx::PxVec2> uvs;

            for (size_t i = 0; i < nbTrigs; i++)
            {
                poss.push_back(PxVec3(trigs[i].a.p.x, trigs[i].a.p.y, trigs[i].a.p.z));
                poss.push_back(PxVec3(trigs[i].b.p.x, trigs[i].b.p.y, trigs[i].b.p.z));
                poss.push_back(PxVec3(trigs[i].c.p.x, trigs[i].c.p.y, trigs[i].c.p.z));

                norms.push_back(PxVec3(trigs[i].a.n.x, trigs[i].a.n.y, trigs[i].a.n.z));
                norms.push_back(PxVec3(trigs[i].b.n.x, trigs[i].b.n.y, trigs[i].b.n.z));
                norms.push_back(PxVec3(trigs[i].c.n.x, trigs[i].c.n.y, trigs[i].c.n.z));

                uvs.push_back(PxVec2(trigs[i].a.uv->x, trigs[i].a.uv->y));
                uvs.push_back(PxVec2(trigs[i].b.uv->x, trigs[i].b.uv->y));
                uvs.push_back(PxVec2(trigs[i].c.uv->x, trigs[i].c.uv->y));
            }

            mesh = new nvmesh(poss, norms, uvs, generateIndices(trigs, nbTrigs));

            auto chunkMesh = createRenderMesh(*mesh);

            return chunkMesh;
        }

        bool validateMesh(ref<submesh_asset> mesh)
        {
            if (!mesh->positions.size())
            {
                LOG_ERROR("Blast> Mesh does not have any vertices.");
                return false;
            }

            if (!mesh->uvs.size())
            {
                LOG_ERROR("Blast> Mesh does not have any vertices.");
                return false;
            }

            return true;
        }

        eentity buildChunk(ref<pbr_material> insideMaterial, ref<pbr_material> outsideMaterial, ref<submesh_asset> mesh, float mass)
        {
            auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

            mesh_builder builder;

            auto mm = make_ref<multi_mesh>();

            builder.pushMesh(*mesh, 1.0f);

            mesh_asset asset;
            asset.name = ("Chunk" + ::std::to_string(id));
            asset.submeshes.push_back(*mesh);

            mm->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, outsideMaterial });

            eentity chunk = enttScene->createEntity(asset.name.c_str())
                .addComponent<transform_component>(vec3(0.0f), quat::identity, vec3(1.f))
                .addComponent<physics::px_convex_mesh_collider_component>(&asset)
                .addComponent<physics::px_rigidbody_component>(physics::px_rigidbody_type::Dynamic)
                .addComponent<mesh_component>(mm);

            mm->mesh = builder.createDXMesh();

            return chunk;
        }

        void connectTouchingChunks(const eentity& chunk, float jointBreakForce, float touchRadius = .01f)
        {
            
        }
    };
}