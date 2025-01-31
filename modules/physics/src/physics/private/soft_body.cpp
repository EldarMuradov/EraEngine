#include "physics/soft_body.h"
#include "physics/core/physics.h"

#include <rttr/registration>

namespace era_engine::physics
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<SoftBodyComponent>("SoftBodyComponent")
			.constructor<>();
	}

	void connect_cube_to_soft_body(physx::PxRigidDynamic* cube, physx::PxReal cube_half_extent, const physx::PxVec3& cube_position, physx::PxSoftBody* soft_body, physx::PxU32 point_grid_resolution)
	{
		using namespace physx;

		float f = 2.0f * cube_half_extent / (point_grid_resolution - 1);
		for (PxU32 ix = 0; ix < point_grid_resolution; ++ix)
		{
			PxReal x = ix * f - cube_half_extent;
			for (PxU32 iy = 0; iy < point_grid_resolution; ++iy)
			{
				PxReal y = iy * f - cube_half_extent;
				for (PxU32 iz = 0; iz < point_grid_resolution; ++iz)
				{
					PxReal z = iz * f - cube_half_extent;
					PxVec3 p(x, y, z);
					PxVec4 bary;
					PxI32 tet = PxTetrahedronMeshExt::findTetrahedronContainingPoint(soft_body->getCollisionMesh(), p + cube_position, bary);
					if (tet >= 0)
					{
						soft_body->addTetRigidAttachment(cube, tet, bary, p);
					}
				}
			}
		}
	}

	SoftBodyComponent::SoftBodyComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		using namespace physx;
		auto& physics_ref = PhysicsHolder::physics_ref;
		// Test
		{
			PxArray<PxVec3> triVerts;
			PxArray<PxU32> triIndices;

			PxReal maxEdgeLength = 1;

			PxCookingParams params(physics_ref->get_tolerance_scale());
			params.meshWeldTolerance = 0.001f;
			params.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES);
			params.buildTriangleAdjacencies = false;
			params.buildGPUData = true;

			create_cube(triVerts, triIndices, PxVec3(0, 0, 0), 1.0f);
			PxRemeshingExt::limitMaxEdgeLength(triIndices, triVerts, maxEdgeLength);
			create_soft_body(params, triVerts, triIndices);

			PxReal halfExtent = 1;
			PxVec3 cubePosA(0, 20, 0);
			PxRigidDynamic* body = create_rigid_cube(halfExtent, cubePosA);

			connect_cube_to_soft_body(body, 2 * halfExtent, cubePosA, soft_body);

			physics_ref->get_scene()->addActor(*soft_body);
		}

		positions_inv_mass = PX_PINNED_HOST_ALLOC_T(physx::PxVec4, PhysicsHolder::physics_ref->get_cuda_context_manager(), soft_body->getCollisionMesh()->getNbVertices());
	}

	SoftBodyComponent::~SoftBodyComponent()
	{

	}

	void SoftBodyComponent::release()
	{
		PhysicsHolder::physics_ref->get_scene()->addActor(*soft_body);

		if (positions_inv_mass)
		{
			PX_PINNED_HOST_FREE(PhysicsHolder::physics_ref->get_cuda_context_manager(), positions_inv_mass);
		}

		Component::release();
		PX_RELEASE(soft_body)
	}

	uint32 SoftBodyComponent::get_nb_vertices() const
	{
		return soft_body->getCollisionMesh()->getNbVertices();
	}

	void SoftBodyComponent::create_soft_body(const physx::PxCookingParams& params, const physx::PxArray<physx::PxVec3>& tri_verts, const physx::PxArray<physx::PxU32>& tri_indices, bool use_collision_mesh_for_simulation)
	{

	}

	void SoftBodyComponent::copy_deformed_vertices_from_gpu_async(CUstream stream)
	{

	}

	void SoftBodyComponent::copy_deformed_vertices_from_gpu()
	{

	}

}