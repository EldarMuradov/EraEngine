// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "px/temp/px_mesh_generator.h"

#include "px/core/px_physics_engine.h"

#include "px/physics/px_rigidbody_component.h"

namespace era_engine::physics
{
	using namespace physx;

	struct px_soft_body
	{
		px_soft_body() = default;

		px_soft_body(PxSoftBody* softBody, PxCudaContextManager* cudaContextManager);

		~px_soft_body() {}

		void release();

		void copyDeformedVerticesFromGPUAsync(CUstream stream);

		void copyDeformedVerticesFromGPU();

		uint32 getNbVertices() const
		{
			return softBody->getCollisionMesh()->getNbVertices();
		}

		PxVec4* positionsInvMass = nullptr;
		PxSoftBody* softBody = nullptr;
		PxCudaContextManager* cudaContextManager = nullptr;
	};

	ref<px_soft_body> createSoftBody(const PxCookingParams& params, const PxArray<PxVec3>& triVerts, const PxArray<PxU32>& triIndices, bool useCollisionMeshForSimulation = false);

	PxRigidDynamic* createRigidCube(PxReal halfExtent, const PxVec3& position);

	void connectCubeToSoftBody(PxRigidDynamic* cube, PxReal cubeHalfExtent, const PxVec3& cubePosition, PxSoftBody* softBody, PxU32 pointGridResolution = 10);

	struct px_soft_body_component : px_body_component
	{
		px_soft_body_component(uint32 handle);

		virtual ~px_soft_body_component() {}

		virtual void release(bool release = true) override;

		ref<px_soft_body> body;
	};
}