#pragma once

#if PX_CLOTH

#pragma comment(lib, "NvCloth_x64.lib")

#include <NvCloth/Cloth.h>
#include <NvClothExt/ClothFabricCooker.h>
#include <NvCloth/Solver.h>
#include <NvCloth/Factory.h>

struct px_clothing_factory
{
	px_clothing_factory();
	px_clothing_factory(px_clothing_factory&) = default;
	px_clothing_factory(px_clothing_factory&&) = default;

	bool update(float dt);

	nv::cloth::Fabric* createFabric(std::vector<vec3>& positions, uint32 vertexCount);
	void releaseFabric(nv::cloth::Fabric* fabric);
	nv::cloth::Cloth* createCloth(std::vector<physx::PxVec4> particlePositions, uint32 particleCount, nv::cloth::Fabric* fabric);
	void releaseCloth(nv::cloth::Cloth* cloth);

	~px_clothing_factory();

	CUcontext cudaContext = nullptr;
	nv::cloth::Factory* factory = nullptr;
	nv::cloth::Solver* solver = nullptr;

	int deviceCount = 0;
};

#endif