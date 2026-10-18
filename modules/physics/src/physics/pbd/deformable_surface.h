#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include <core/math.h>

#include <extensions/PxCudaHelpersExt.h>
#include <extensions/PxDeformableSkinningExt.h>
#include <extensions/PxRemeshingExt.h>

namespace physx
{
    template <typename T>
    class HostAndDeviceBuffer
    {
    public:
        HostAndDeviceBuffer()
        {
        }

        HostAndDeviceBuffer(PxCudaContextManager* contextManager, PxU32 numElements)
            : context_manager(contextManager), device_data(NULL), host_data(NULL), num_elements(0)
        {
            allocate(numElements);
        }

        void initialize(PxCudaContextManager* contextManager, PxU32 numElements)
        {
            context_manager = contextManager;
            allocate(numElements);
        }

        void initialize(PxCudaContextManager* contextManager, const T* dataSource, PxU32 numElements)
        {
            context_manager = contextManager;
            allocate(numElements);
            PxMemCopy(host_data, dataSource, numElements * sizeof(T));
        }

        void allocate(PxU32 numElements)
        {
            release();
            device_data = Ext::PxCudaHelpersExt::allocDeviceBuffer<T>(*context_manager, numElements);
            host_data = Ext::PxCudaHelpersExt::allocPinnedHostBuffer<T>(*context_manager, numElements);
            num_elements = numElements;
        }

        void copy_device_to_host(PxU32 numElementsToCopy = 0xFFFFFFFF)
        {
            Ext::PxCudaHelpersExt::copyDToH(*context_manager, host_data, device_data, PxMin(numElementsToCopy, num_elements));
        }

        void copy_host_to_device(PxU32 numElementsToCopy = 0xFFFFFFFF)
        {
            Ext::PxCudaHelpersExt::copyHToD<T>(*context_manager, device_data, host_data, PxMin(numElementsToCopy, num_elements));
        }

        void copy_device_to_host_async(CUstream stream, PxU32 numElementsToCopy = 0xFFFFFFFF)
        {
            Ext::PxCudaHelpersExt::copyDToHAsync(*context_manager, host_data, device_data, PxMin(numElementsToCopy, num_elements), stream);
        }

        void copy_host_to_device_async(CUstream stream, PxU32 numElementsToCopy = 0xFFFFFFFF)
        {
            Ext::PxCudaHelpersExt::copyHToDAsync(*context_manager, device_data, host_data, PxMin(numElementsToCopy, num_elements), stream);
        }

        void release()
        {
            Ext::PxCudaHelpersExt::freeDeviceBuffer(*context_manager, device_data);
            Ext::PxCudaHelpersExt::freePinnedHostBuffer(*context_manager, host_data);
        }

        PxCudaContextManager* context_manager = nullptr;
        T* device_data = nullptr;
        T* host_data = nullptr;
        PxU32 num_elements = 0;
    };
}

namespace era_engine::physics
{
    struct DeformableSkinnedMesh
    {
        physx::PxArray<physx::PxVec3> vertices;
        physx::PxArray<physx::PxU32> triangles;
    };

    class BasePostSolveCallback : public physx::PxPostSolveCallback
    {
        virtual void synchronize() = 0;
        virtual physx::PxVec3* getSkinnedVertices(physx::PxU32 clothIndex) = 0;
    };

    class ERA_PHYSICS_API DeformableSurface final
    {
    public:
        DeformableSurface() = default;
        DeformableSurface(physx::PxDeformableSurface* _deformable_surface, physx::PxU32 _num_elements, DeformableSkinnedMesh&& _skinned_mesh);
        ~DeformableSurface();

        void release();

        void copy_deformed_vertices_from_gpu_async(CUstream stream);
        void copy_deformed_vertices_from_gpu();

        physx::PxVec4* get_positions_inv_mass() const;
        physx::PxDeformableSurface* get_deformable_surface() const;
        physx::PxTriangleMesh* get_triangle_mesh() const;

        const DeformableSkinnedMesh& get_skinned_mesh() const;
        DeformableSkinnedMesh& get_skinned_mesh_for_write();

        physx::PxU32 get_num_elements() const;

    private:
        physx::PxVec4* positions_inv_mass = nullptr;
        physx::PxDeformableSurface* deformable_surface = nullptr;
        physx::PxTriangleMesh* triangle_mesh = nullptr;

        DeformableSkinnedMesh skinned_mesh;

        physx::PxU32 num_elements = 0;
    };

    physx::PxDeformableSurface* create_deformable_surface(physx::PxPhysics& physics,
        physx::PxTriangleMesh* triangle_mesh,
        physx::PxDeformableSurfaceMaterial** materials,
        const physx::PxU32 nb_materials,
        physx::PxCudaContextManager* cuda_context_manager);

    physx::PxDeformableSurface* create_deformable_surface(physx::PxPhysics& physics,
        const physx::PxCookingParams& ck_params,
        physx::PxArray<physx::PxVec3>& vertices,
        physx::PxArray<physx::PxU32>& triangles,
        physx::PxDeformableSurfaceMaterial** materials,
        const physx::PxU32 nb_materials,
        physx::PxCudaContextManager* cuda_context_manager);

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
        physx::PxReal density);

    class SurfaceSkinningHelper
    {
    public:
        SurfaceSkinningHelper();

        SurfaceSkinningHelper(physx::PxCudaContextManager* context_manager,
            physx::PxDeformableSurface* _deformable_surface,
            physx::PxVec3* skinned_points_rest_position,
            physx::PxU32 nb_skinned_points);

        void package_gpu_data(physx::PxTrimeshSkinningGpuData& target);

        void release();

        physx::PxDeformableSurface* deformable_surface = nullptr;
        physx::HostAndDeviceBuffer<physx::PxU32> surface_triangles;
        physx::HostAndDeviceBuffer<physx::PxVec3> normal_vectors;
        physx::HostAndDeviceBuffer<physx::PxTriangleMeshEmbeddingInfo> skinning_info;
        physx::HostAndDeviceBuffer<physx::PxVec3> skinned_vertices;
        physx::PxU32 num_skinned_vertices = 0;
        physx::PxReal half_thickness = 0.0f;
    };
}