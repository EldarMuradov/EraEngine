// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "pch.h"
#include "px/physics/px_soft_body.h"

namespace era_engine
{
	physics::px_soft_body_component::px_soft_body_component(uint32 handle) : px_body_component(handle)
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

	void physics::px_soft_body_component::release(bool release) noexcept
	{
		body->release();
		body.reset();
		physics_holder::physicsRef->removeActor(this);

		PX_RELEASE(actor)
	}
}