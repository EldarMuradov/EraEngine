// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/physics/px_soft_body.h"

namespace era_engine::physics
{
	px_soft_body_component::px_soft_body_component(uint32 handle) : px_body_component(handle)
	{
		auto& physicsRef = physics_holder::physicsRef;
		// Test
		{
			PxArray<PxVec3> triVerts;
			PxArray<PxU32> triIndices;

			PxReal maxEdgeLength = 1;

			PxCookingParams params(physicsRef->getTolerancesScale());
			params.meshWeldTolerance = 0.001f;
			params.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES);
			params.buildTriangleAdjacencies = false;
			params.buildGPUData = true;

			meshgenerator::createCube(triVerts, triIndices, PxVec3(0, 0, 0), 1.0f);
			PxRemeshingExt::limitMaxEdgeLength(triIndices, triVerts, maxEdgeLength);
			ref<px_soft_body> softBodyCube = createSoftBody(params, triVerts, triIndices);

			PxReal halfExtent = 1;
			PxVec3 cubePosA(0, 20, 0);
			PxRigidDynamic* body = createRigidCube(halfExtent, cubePosA);

			connectCubeToSoftBody(body, 2 * halfExtent, cubePosA, softBodyCube->softBody);

			actor = body;

			physicsRef->addActor(this, actor, false);
		}
	}

	void px_soft_body_component::release(bool release)
	{
		body->release();
		body.reset();
		physics_holder::physicsRef->removeActor(this);

		PX_RELEASE(actor)
	}

	px_soft_body::px_soft_body(PxSoftBody* softBody, PxCudaContextManager* cudaContextManager)
		:
		softBody(softBody),
		cudaContextManager(cudaContextManager)
	{
		positionsInvMass = PX_PINNED_HOST_ALLOC_T(PxVec4, cudaContextManager, softBody->getCollisionMesh()->getNbVertices());
	}

	void px_soft_body::release()
	{
		if (softBody)
			softBody->release();
		if (positionsInvMass)
			PX_PINNED_HOST_FREE(cudaContextManager, positionsInvMass);
	}

	void px_soft_body::copyDeformedVerticesFromGPUAsync(CUstream stream)
	{
		PxTetrahedronMesh* tetMesh = softBody->getCollisionMesh();

		PxScopedCudaLock _lock(*cudaContextManager);
		cudaContextManager->getCudaContext()->memcpyDtoHAsync(positionsInvMass, reinterpret_cast<CUdeviceptr>(softBody->getPositionInvMassBufferD()), tetMesh->getNbVertices() * sizeof(PxVec4), stream);
	}

	void px_soft_body::copyDeformedVerticesFromGPU()
	{
		PxTetrahedronMesh* tetMesh = softBody->getCollisionMesh();

		PxScopedCudaLock _lock(*cudaContextManager);
		cudaContextManager->getCudaContext()->memcpyDtoH(positionsInvMass, reinterpret_cast<CUdeviceptr>(softBody->getPositionInvMassBufferD()), tetMesh->getNbVertices() * sizeof(PxVec4));
	}

	ref<px_soft_body> createSoftBody(const PxCookingParams& params, const PxArray<PxVec3>& triVerts, const PxArray<PxU32>& triIndices, bool useCollisionMeshForSimulation)
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
			auto sb = physics_holder::physicsRef->addSoftBody(softBody, femParams, PxTransform(PxVec3(0.f, 0.f, 0.f), PxQuat(PxIdentity)), 100.f, 1.0f, 30);
			softBody->setSoftBodyFlag(PxSoftBodyFlag::eDISABLE_SELF_COLLISION, false);
			softBody->setSoftBodyFlag(PxSoftBodyFlag::eENABLE_CCD, true);

			return sb;
		}

		return nullptr;
	}

	PxRigidDynamic* createRigidCube(PxReal halfExtent, const PxVec3& position)
	{
		const auto physics = physics_holder::physicsRef->getPhysics();

		PxMaterial* material = physics->createMaterial(0.8f, 0.8f, 0.6f);

		PxShape* shape = physics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *material);

		shape->setSimulationFilterData(PxFilterData(0, 0, 1, 0));

		PxTransform localTm(position);
		PxRigidDynamic* body = physics->createRigidDynamic(localTm);
		body->attachShape(*shape);
		PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
		physics_holder::physicsRef->getScene()->addActor(*body);

		shape->release();

		return body;
	}

	void connectCubeToSoftBody(PxRigidDynamic* cube, PxReal cubeHalfExtent, const PxVec3& cubePosition, PxSoftBody* softBody, PxU32 pointGridResolution)
	{
		float f = 2.0f * cubeHalfExtent / (pointGridResolution - 1);
		for (PxU32 ix = 0; ix < pointGridResolution; ++ix)
		{
			PxReal x = ix * f - cubeHalfExtent;
			for (PxU32 iy = 0; iy < pointGridResolution; ++iy)
			{
				PxReal y = iy * f - cubeHalfExtent;
				for (PxU32 iz = 0; iz < pointGridResolution; ++iz)
				{
					PxReal z = iz * f - cubeHalfExtent;
					PxVec3 p(x, y, z);
					PxVec4 bary;
					PxI32 tet = PxTetrahedronMeshExt::findTetrahedronContainingPoint(softBody->getCollisionMesh(), p + cubePosition, bary);
					if (tet >= 0)
						softBody->addTetRigidAttachment(cube, tet, bary, p);
				}
			}
		}
	}
}