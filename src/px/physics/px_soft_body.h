// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include <px/temp/px_mesh_generator.h>
#include <px/core/px_physics_engine.h>

namespace physics
{
	using namespace physx;

	struct px_soft_body
	{
		px_soft_body() = default;

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

		uint32 getNbVertices() const noexcept
		{
			return softBody->getCollisionMesh()->getNbVertices();
		}

		PxVec4* positionsInvMass = nullptr;
		PxSoftBody* softBody = nullptr;
		PxCudaContextManager* cudaContextManager = nullptr;
	};

	inline ref<px_soft_body> createSoftBody(const PxCookingParams& params, const PxArray<PxVec3>& triVerts, const PxArray<PxU32>& triIndices, bool useCollisionMeshForSimulation = false)
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

	inline PxRigidDynamic* createRigidCube(PxReal halfExtent, const PxVec3& position)
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

	inline void connectCubeToSoftBody(PxRigidDynamic* cube, PxReal cubeHalfExtent, const PxVec3& cubePosition, PxSoftBody* softBody, PxU32 pointGridResolution = 10)
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

	struct px_soft_body_component : px_physics_component_base
	{
		px_soft_body_component(uint32 handle) noexcept : px_physics_component_base(handle)
		{
			// Test
			{
				PxArray<PxVec3> triVerts;
				PxArray<PxU32> triIndices;

				PxReal maxEdgeLength = 1;

				PxCookingParams params(physics_holder::physicsRef->getTolerancesScale());
				params.meshWeldTolerance = 0.001f;
				params.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES);
				params.buildTriangleAdjacencies = false;
				params.buildGPUData = true;

				meshgenerator::createCube(triVerts, triIndices, PxVec3(0, 0, 0), 1.0f);
				PxRemeshingExt::limitMaxEdgeLength(triIndices, triVerts, maxEdgeLength);
				ref<px_soft_body> softBodyCube = createSoftBody(params, triVerts, triIndices);

				PxReal halfExtent = 1;
				PxVec3 cubePosA(0, 20, 0);
				PxRigidDynamic* rigidCubeA = createRigidCube(halfExtent, cubePosA);

				connectCubeToSoftBody(rigidCubeA, 2 * halfExtent, cubePosA, softBodyCube->softBody);
			}
		}

		virtual ~px_soft_body_component() {}

		virtual void release(bool release = true) noexcept override { body->release(); body.reset(); }

		ref<px_soft_body> body;
	};
}