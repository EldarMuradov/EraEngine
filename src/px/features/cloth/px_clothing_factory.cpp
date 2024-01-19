#include "pch.h"

#if PX_CLOTH

#include <px/core/px_physics_engine.h>
#include "px/features/cloth/px_clothing_factory.h"
#include <NvCloth/DxContextManagerCallback.h>
#include <NvCloth/Range.h>
#include <NvCloth/PhaseConfig.h>

px_clothing_factory::px_clothing_factory()
{
	CUresult result = cuDeviceGetCount(&deviceCount);
	ASSERT(CUDA_SUCCESS == result);
	ASSERT(deviceCount >= 1);

	result = cuCtxCreate(&cudaContext, 0, 0);
	ASSERT(CUDA_SUCCESS == result);

	nv::cloth::Factory* factory = NvClothCreateFactoryCUDA(cudaContext);

	solver = factory->createSolver();
}

bool px_clothing_factory::update(float dt)
{
	float deltaTime = 1.0f / 60.0f;
	solver->beginSimulation(deltaTime);
	for (int i = 0; i < solver->getSimulationChunkCount(); i++)
	{
		solver->simulateChunk(i);
	}
	solver->endSimulation();

	return true;
}

nv::cloth::Fabric* px_clothing_factory::createFabric(std::vector<vec3>& positions, uint32 vertexCount)
{
	nv::cloth::ClothMeshDesc meshDesc;

	std::vector<physx::PxVec3> vertices;
	for (size_t i = 0; i < vertexCount; i++)
	{
		vertices.push_back(PxVec3(positions[i].x, positions[i].y, positions[i].z));
	}

	meshDesc.setToDefault();
	meshDesc.points.data = vertices.data();
	meshDesc.points.stride = sizeof(vertices[0]);
	meshDesc.points.count = vertexCount;

	nv::cloth::Vector<int32_t>::Type phaseTypeInfo;
	float grav[3] = {0, -9.81f, 0};
	nv::cloth::Fabric* fabric = NvClothCookFabricFromMesh(factory, meshDesc, &grav[0], &phaseTypeInfo);
	return fabric;
}

void px_clothing_factory::releaseFabric(nv::cloth::Fabric* fabric)
{
	fabric->decRefCount();
}

nv::cloth::Cloth* px_clothing_factory::createCloth(std::vector<physx::PxVec4> particlePositions, uint32 particleCount, nv::cloth::Fabric* fabric)
{
	nv::cloth::Cloth* cloth = factory->createCloth(nv::cloth::Range<physx::PxVec4>(particlePositions.data(), &particlePositions[0] + particleCount), *fabric);
	
	nv::cloth::PhaseConfig* phases = new nv::cloth::PhaseConfig[fabric->getNumPhases()];
	for (int i = 0; i < fabric->getNumPhases(); i++)
	{
		phases[i].mPhaseIndex = i;

		phases[i].mStiffness = 1.0f;
		phases[i].mStiffnessMultiplier = 1.0f;
		phases[i].mCompressionLimit = 1.0f;
		phases[i].mStretchLimit = 1.0f;
	}
	cloth->setPhaseConfig(nv::cloth::Range<nv::cloth::PhaseConfig>(phases, phases + fabric->getNumPhases()));
	delete[] phases;

	cloth->setGravity(gravity);
	cloth->setDamping(physx::PxVec3(0.5f));
	
	solver->addCloth(cloth);

	return cloth;
}

void px_clothing_factory::releaseCloth(nv::cloth::Cloth* cloth)
{
	solver->removeCloth(cloth);
	NV_CLOTH_DELETE(cloth);
}

px_clothing_factory::~px_clothing_factory()
{
	NV_CLOTH_DELETE(solver);
	cuCtxDestroy(cudaContext);
	NvClothDestroyFactory(factory);
}

#endif