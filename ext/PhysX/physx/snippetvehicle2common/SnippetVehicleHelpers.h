#include "PxPhysicsAPI.h"

namespace snippetvehicle2
{

physx::PxFilterFlags VehicleFilterShader(
	physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
	physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
	physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize);


bool parseVehicleDataPath(int argc, const char *const* argv, const char* snippetName,
	const char*& vehicleDataPath);

}//namespace snippetvehicle2
