#include "physics/pbd/deformable_surface.h"
#include "physics/core/physics.h"

namespace era_engine::physics
{
    static PX_FORCE_INLINE physx::PxU32 id(physx::PxU32 x, physx::PxU32 y, physx::PxU32 numY)
    {
        return x * numY + y;
    }

    static PX_FORCE_INLINE physx::PxReal compute_triangle_mass(const physx::PxU32* triangle, const physx::PxVec3* vertices, physx::PxReal thickness, physx::PxReal density)
    {
        physx::PxReal area = 0.5f * (vertices[triangle[1]] - vertices[triangle[0]]).cross(vertices[triangle[2]] - vertices[triangle[0]]).magnitude();
        return area * thickness * density;
    }

    DeformableSurface::DeformableSurface(physx::PxDeformableSurface* _deformable_surface, physx::PxU32 _num_elements, DeformableSkinnedMesh&& _skinned_mesh)
        : deformable_surface(_deformable_surface), num_elements(_num_elements), skinned_mesh(std::move(_skinned_mesh))
    {
        physx::PxShape* shape = deformable_surface->getShape();

        const physx::PxTriangleMeshGeometry& triangle_mesh_geom = static_cast<const physx::PxTriangleMeshGeometry&>(shape->getGeometry());
        triangle_mesh = triangle_mesh_geom.triangleMesh;

        physx::PxCudaContextManager* context_manager = PhysicsEngine::get_physics_core()->get_cuda_context_manager();

        positions_inv_mass = PX_EXT_PINNED_MEMORY_ALLOC(physx::PxVec4, *context_manager, triangle_mesh->getNbVertices());
    }

    DeformableSurface::~DeformableSurface()
    {
    }

    void DeformableSurface::release()
    {
        PhysicsEngine::execute_write([&]()
            {
                PhysicsEngine::get_physics_core()->get_scene()->addActor(*deformable_surface);

                PX_RELEASE(deformable_surface)
            });

        PX_EXT_PINNED_MEMORY_FREE(*PhysicsEngine::get_physics_core()->get_cuda_context_manager(), positions_inv_mass);

        positions_inv_mass = nullptr;
    }

    void DeformableSurface::copy_deformed_vertices_from_gpu_async(CUstream stream)
    {
        physx::PxCudaContextManager* context_manager = PhysicsEngine::get_physics_core()->get_cuda_context_manager();

        physx::PxScopedCudaLock _lock(*context_manager);
        context_manager->getCudaContext()->memcpyDtoHAsync(positions_inv_mass,
            reinterpret_cast<CUdeviceptr>(deformable_surface->getPositionInvMassBufferD()), triangle_mesh->getNbVertices() * sizeof(physx::PxVec4), stream);
    }

    void DeformableSurface::copy_deformed_vertices_from_gpu()
    {
        physx::PxCudaContextManager* context_manager = PhysicsEngine::get_physics_core()->get_cuda_context_manager();

        physx::PxScopedCudaLock _lock(*context_manager);
        context_manager->getCudaContext()->memcpyDtoH(positions_inv_mass,
            reinterpret_cast<CUdeviceptr>(deformable_surface->getPositionInvMassBufferD()), triangle_mesh->getNbVertices() * sizeof(physx::PxVec4));
    }

    physx::PxVec4* DeformableSurface::get_positions_inv_mass() const
    {
        return positions_inv_mass;
    }

    physx::PxDeformableSurface* DeformableSurface::get_deformable_surface() const
    {
        return deformable_surface;
    }

    physx::PxTriangleMesh* DeformableSurface::get_triangle_mesh() const
    {
        return triangle_mesh;
    }

    const DeformableSkinnedMesh& DeformableSurface::get_skinned_mesh() const
    {
        return skinned_mesh;
    }

    DeformableSkinnedMesh& DeformableSurface::get_skinned_mesh_for_write()
    {
        return skinned_mesh;
    }

    physx::PxU32 DeformableSurface::get_num_elements() const
    {
        return num_elements;
    }

    SurfaceSkinningHelper::SurfaceSkinningHelper()
    {
    }

    SurfaceSkinningHelper::SurfaceSkinningHelper(physx::PxCudaContextManager* context_manager,
        physx::PxDeformableSurface* _deformable_surface,
        physx::PxVec3* skinned_points_rest_position,
        physx::PxU32 nb_skinned_points)
        : deformable_surface(_deformable_surface)
    {
        using namespace physx;

        const PxTriangleMeshGeometry& triangle_mesh_geom =
            static_cast<const PxTriangleMeshGeometry&>(deformable_surface->getShape()->getGeometry());

        PxU32 nb_triangles = triangle_mesh_geom.triangleMesh->getNbTriangles();

        bool uses16bit = triangle_mesh_geom.triangleMesh->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES;
        surface_triangles.initialize(context_manager, 3 * nb_triangles);

        if (uses16bit)
        {
            const PxU16* triangleIndices = reinterpret_cast<const PxU16*>(triangle_mesh_geom.triangleMesh->getTriangles());
            for (PxU32 i = 0; i < surface_triangles.num_elements; ++i)
                surface_triangles.host_data[i] = triangleIndices[i];
        }
        else
        {
            const PxU32* triangleIndices = reinterpret_cast<const PxU32*>(triangle_mesh_geom.triangleMesh->getTriangles());
            for (PxU32 i = 0; i < surface_triangles.num_elements; ++i)
                surface_triangles.host_data[i] = triangleIndices[i];
        }

        normal_vectors.initialize(context_manager, triangle_mesh_geom.triangleMesh->getNbVertices());
        skinned_vertices.initialize(context_manager, skinned_points_rest_position, nb_skinned_points);
        num_skinned_vertices = nb_skinned_points;
        skinning_info.initialize(context_manager, nb_skinned_points);

        PxDeformableSkinningExt::initializeInterpolatedVertices(
            skinning_info.host_data, triangle_mesh_geom.triangleMesh->getVertices(), NULL, surface_triangles.host_data,
            nb_triangles, skinned_points_rest_position, nb_skinned_points);

        surface_triangles.copy_host_to_device();
        skinned_vertices.copy_host_to_device();
        skinning_info.copy_host_to_device();

        PxShape* surface_shape = deformable_surface->getShape();
        half_thickness = surface_shape->getRestOffset();
    }

    void SurfaceSkinningHelper::package_gpu_data(physx::PxTrimeshSkinningGpuData& target)
    {
        using namespace physx;

        target.guideVerticesD.data = reinterpret_cast<PxVec3*>(deformable_surface->getPositionInvMassBufferD());
        target.guideVerticesD.stride = sizeof(PxVec4);
        target.guideVerticesD.count = normal_vectors.num_elements;

        target.guideNormalsD = normal_vectors.device_data;
        target.guideTrianglesD = surface_triangles.device_data;
        target.skinningInfoPerVertexD = skinning_info.device_data;
        target.skinnedVerticesD.data = skinned_vertices.host_data; // Works because host data is pinned memory.
        target.skinnedVerticesD.stride = sizeof(PxVec3);
        target.skinnedVerticesD.count = num_skinned_vertices;

        target.halfSurfaceThickness = half_thickness;
        target.nbGuideTriangles = surface_triangles.num_elements / 3;
    }

    void SurfaceSkinningHelper::release()
    {
        surface_triangles.release();
        normal_vectors.release();
        skinned_vertices.release();
        skinning_info.release();
    }

    physx::PxDeformableSurface* create_deformable_surface(physx::PxPhysics& physics,
        physx::PxTriangleMesh* triangle_mesh,
        physx::PxDeformableSurfaceMaterial** materials,
        const physx::PxU32 nb_materials,
        physx::PxCudaContextManager* cuda_context_manager)
    {
        using namespace physx;

        if (triangle_mesh == nullptr)
        {
            return nullptr;
        }

        PxDeformableSurface* deformable_surface = physics.createDeformableSurface(*cuda_context_manager);
        if (deformable_surface)
        {
            PxShapeFlags shape_flags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE;

            PxTriangleMeshGeometry geometry(triangle_mesh);
            PxShape* shape = physics.createShape(geometry, materials, PxU16(nb_materials), true, shape_flags);
            if (shape)
            {
                deformable_surface->attachShape(*shape);
            }
        }

        return deformable_surface;
    }

    physx::PxDeformableSurface* create_deformable_surface(physx::PxPhysics& physics,
        const physx::PxCookingParams& ck_params,
        physx::PxArray<physx::PxVec3>& vertices,
        physx::PxArray<physx::PxU32>& triangles,
        physx::PxDeformableSurfaceMaterial** materials,
        const physx::PxU32 nb_materials,
        physx::PxCudaContextManager* cuda_context_manager)
    {
        using namespace physx;

        PxTriangleMeshDesc meshDesc;
        meshDesc.points.count = vertices.size();
        meshDesc.triangles.count = triangles.size() / 3;
        meshDesc.points.stride = sizeof(float) * 3;
        meshDesc.triangles.stride = sizeof(int) * 3;
        meshDesc.points.data = vertices.begin();
        meshDesc.triangles.data = triangles.begin();

        PxDeformableMaterialTableIndex* material_indices = nullptr;
        if (nb_materials > 1)
        {
            const PxU32 total_triangles = meshDesc.triangles.count;
            material_indices = new PxDeformableMaterialTableIndex[total_triangles];
            const PxU32 average_triangle_per_materials = total_triangles / nb_materials;

            PxU32 accumulated_triangle = average_triangle_per_materials;

            PxU32 index = 0;
            for (PxU32 i = 0; i < total_triangles; ++i)
            {
                material_indices[i] = PxDeformableMaterialTableIndex(index);
                if (i == accumulated_triangle)
                {
                    index = index < (nb_materials - 1) ? index + 1 : index;
                    accumulated_triangle += average_triangle_per_materials;
                }
            }
            meshDesc.materialIndices.stride = sizeof(PxDeformableMaterialTableIndex);
            meshDesc.materialIndices.data = material_indices;
        }

        PxTriangleMesh* triangle_mesh = PxCreateTriangleMesh(ck_params, meshDesc, physics.getPhysicsInsertionCallback());

        return create_deformable_surface(physics, triangle_mesh, materials, nb_materials, cuda_context_manager);
    }

    DeformableSurface add_quad_deformable_surface(physx::PxPhysics& physics,
        const physx::PxCookingParams& ck_params,
        const physx::PxTransform& transform,
        physx::PxU32 num_x,
        physx::PxU32 num_z,
        physx::PxReal size_x,
        physx::PxReal size_z,
        physx::PxDeformableSurfaceMaterial** materials,
        const physx::PxU32 nb_materials,
        physx::PxCudaContextManager* cuda_context_manager,
        physx::PxReal thickness,
        physx::PxReal density)
    {
        using namespace physx;

        if (cuda_context_manager == nullptr)
        {
            ASSERT(false);
            return {};
        }

        PxArray<PxVec3> vertices;
        PxArray<PxU32> triangles;
        PxArray<PxVec3> velocity;
        PxArray<PxReal> triangle_masses;

        const PxU32 total_num = num_x * num_z;

        vertices.reserve(total_num);
        velocity.reserve(total_num);
        triangles.reserve(3 * 2 * (num_x - 1) * (num_z - 1));
        triangle_masses.reserve(2 * (num_x - 1) * (num_z - 1));

        PxReal scaling_x = size_x / (num_x - 1);
        PxReal scaling_z = size_z / (num_z - 1);

        for (PxU32 i = 0; i < num_x; ++i)
        {
            for (PxU32 j = 0; j < num_z; ++j)
            {
                PxVec3 pos(i * scaling_x, 0.0f, j * scaling_z);
                vertices.pushBack(pos);
                velocity.pushBack(PxVec3(0.0f));
            }
        }

        for (PxU32 i = 1; i < num_x; ++i)
        {
            for (PxU32 j = 1; j < num_z; ++j)
            {
                triangles.pushBack(id(i - 1, j - 1, num_z));
                triangles.pushBack(id(i, j - 1, num_z));
                triangles.pushBack(id(i - 1, j, num_z));
                triangle_masses.pushBack(compute_triangle_mass(&triangles[triangles.size() - 3], vertices.begin(), thickness, density));

                triangles.pushBack(id(i - 1, j, num_z));
                triangles.pushBack(id(i, j - 1, num_z));
                triangles.pushBack(id(i, j, num_z));
                triangle_masses.pushBack(compute_triangle_mass(&triangles[triangles.size() - 3], vertices.begin(), thickness, density));
            }
        }

        PxArray<PxU32> subdivided_triangles = triangles;
        PxArray<PxVec3> subdivided_vertices = vertices;
        PxRemeshingExt::limitMaxEdgeLength(subdivided_triangles, subdivided_vertices, 0.0001f, 3);

        DeformableSkinnedMesh mesh;
        for (PxU32 i = 0; i < subdivided_triangles.size(); ++i)
        {
            mesh.triangles.pushBack(subdivided_triangles[i]);
        }
        for (PxU32 i = 0; i < subdivided_vertices.size(); ++i)
        {
            mesh.vertices.pushBack(subdivided_vertices[i]);
        }

        PxDeformableSurface* deformable_surface = create_deformable_surface(physics, ck_params, vertices, triangles, materials, nb_materials, cuda_context_manager);
        PhysicsEngine::get_physics_core()->get_scene()->addActor(*deformable_surface);

        PxVec4* pos_inv_mass_pinned;
        PxVec4* velocity_pinned;
        PxVec4* rest_position_pinned;
        PxDeformableSurfaceExt::allocateAndInitializeHostMirror(*deformable_surface, vertices.begin(), velocity.begin(), vertices.begin(), 0.5f,
            transform, cuda_context_manager, pos_inv_mass_pinned, velocity_pinned, rest_position_pinned);

        PxDeformableSurfaceExt::distributeTriangleMassToVertices(*deformable_surface, triangle_masses.begin(), pos_inv_mass_pinned);

        PxShape* surface_shape = deformable_surface->getShape();
        surface_shape->setContactOffset(2.0f * thickness);
        surface_shape->setRestOffset(thickness);
        surface_shape->getDeformableSurfaceMaterials(materials, PxU16(nb_materials));

        PxDeformableSurfaceExt::copyToDevice(*deformable_surface, PxDeformableSurfaceDataFlag::eALL, vertices.size(), pos_inv_mass_pinned, velocity_pinned, rest_position_pinned);

        DeformableSurface cloth(deformable_surface, total_num, std::move(mesh));

        PX_EXT_PINNED_MEMORY_FREE(*cuda_context_manager, pos_inv_mass_pinned);
        PX_EXT_PINNED_MEMORY_FREE(*cuda_context_manager, velocity_pinned);
        PX_EXT_PINNED_MEMORY_FREE(*cuda_context_manager, rest_position_pinned);

        return cloth;
    }
}