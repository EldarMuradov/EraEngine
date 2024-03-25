#pragma once

#include <px/core/px_physics_engine.h>

namespace physics
{
	using namespace physx;

	struct px_soft_body
	{
		px_soft_body(PxSoftBody* softBody, PxCudaContextManager* cudaContextManager) :
			softBody(softBody),
			cudaContextManager(cudaContextManager)
		{
			positionsInvMass = PX_PINNED_HOST_ALLOC_T(PxVec4, cudaContextManager, softBody->getCollisionMesh()->getNbVertices());
		}

		~px_soft_body() {}

		void release()
		{
			if (softBody)
				softBody->release();
			if (positionsInvMass)
				PX_PINNED_HOST_FREE(cudaContextManager, positionsInvMass);
		}

		void copyDeformedVerticesFromGPUAsync(CUstream stream)
		{
			PxTetrahedronMesh* tetMesh = softBody->getCollisionMesh();

			PxScopedCudaLock _lock(*cudaContextManager);
			cudaContextManager->getCudaContext()->memcpyDtoHAsync(positionsInvMass, reinterpret_cast<CUdeviceptr>(softBody->getPositionInvMassBufferD()), tetMesh->getNbVertices() * sizeof(PxVec4), stream);
		}

		void copyDeformedVerticesFromGPU()
		{
			PxTetrahedronMesh* tetMesh = softBody->getCollisionMesh();

			PxScopedCudaLock _lock(*cudaContextManager);
			cudaContextManager->getCudaContext()->memcpyDtoH(positionsInvMass, reinterpret_cast<CUdeviceptr>(softBody->getPositionInvMassBufferD()), tetMesh->getNbVertices() * sizeof(PxVec4));
		}


		PxVec4* positionsInvMass = nullptr;
		PxSoftBody* softBody = nullptr;
		PxCudaContextManager* cudaContextManager = nullptr;
	};

	inline PxSoftBody* createSoftBody(const PxCookingParams& params, const PxArray<PxVec3>& triVerts, const PxArray<PxU32>& triIndices, bool useCollisionMeshForSimulation = false)
	{
		const auto physics = physics_holder::physicsRef->getPhysics();

		PxSoftBodyMesh* softBodyMesh;

		PxU32 numVoxelsAlongLongestAABBAxis = 8;

		PxSimpleTriangleMesh surfaceMesh;
		surfaceMesh.points.count = triVerts.size();
		surfaceMesh.points.data = triVerts.begin();
		surfaceMesh.triangles.count = triIndices.size() / 3;
		surfaceMesh.triangles.data = triIndices.begin();

		if (useCollisionMeshForSimulation)
		{
			softBodyMesh = PxSoftBodyExt::createSoftBodyMeshNoVoxels(params, surfaceMesh, physics->getPhysicsInsertionCallback());
		}
		else
		{
			softBodyMesh = PxSoftBodyExt::createSoftBodyMesh(params, surfaceMesh, numVoxelsAlongLongestAABBAxis, physics->getPhysicsInsertionCallback());
		}

		//Alternatively one can cook a softbody mesh in a single step
		//tetMesh = cooking.createSoftBodyMesh(simulationMeshDesc, collisionMeshDesc, softbodyDesc, physics.getPhysicsInsertionCallback());
		PX_ASSERT(softBodyMesh);

		if (!physics_holder::physicsRef->getCudaContextManager())
			return NULL;
		PxSoftBody* softBody = physics->createSoftBody(*physics_holder::physicsRef->getCudaContextManager());
		if (softBody)
		{
			PxShapeFlags shapeFlags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE;

			PxFEMSoftBodyMaterial* materialPtr = PxGetPhysics().createFEMSoftBodyMaterial(1e+6f, 0.45f, 0.5f);
			PxTetrahedronMeshGeometry geometry(softBodyMesh->getCollisionMesh());
			PxShape* shape = physics->createShape(geometry, &materialPtr, 1, true, shapeFlags);
			if (shape)
			{
				softBody->attachShape(*shape);
				shape->setSimulationFilterData(PxFilterData(0, 0, 2, 0));
			}
			softBody->attachSimulationMesh(*softBodyMesh->getSimulationMesh(), *softBodyMesh->getSoftBodyAuxData());

			physics_holder::physicsRef->getScene()->addActor(*softBody);

			PxFEMParameters femParams;
			physics_holder::physicsRef->addSoftBody(softBody, femParams, PxTransform(PxVec3(0.f, 0.f, 0.f), PxQuat(PxIdentity)), 100.f, 1.0f, 30);
			softBody->setSoftBodyFlag(PxSoftBodyFlag::eDISABLE_SELF_COLLISION, true);
		}
		return softBody;
	}
}