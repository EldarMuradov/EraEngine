#include <pch.h>
#ifdef setBit
#undef setBit
#endif // setBit
#include <ctype.h>

#include "../snippetvehicle2common/SnippetVehicleHelpers.h"

using namespace physx;

namespace snippetvehicle2
{

PxFilterFlags VehicleFilterShader(
PxFilterObjectAttributes attributes0, PxFilterData filterData0,
PxFilterObjectAttributes attributes1, PxFilterData filterData1,
PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	PX_UNUSED(attributes0);
	PX_UNUSED(filterData0);
	PX_UNUSED(attributes1);
	PX_UNUSED(filterData1);
	PX_UNUSED(pairFlags);
	PX_UNUSED(constantBlock);
	PX_UNUSED(constantBlockSize);
	return PxFilterFlag::eSUPPRESS;
}


bool parseVehicleDataPath(int argc, const char *const* argv, const char* snippetName,
	const char*& vehicleDataPath)
{
	if (argc != 2 || 0 != strncmp(argv[1], "--vehicleDataPath", strlen("--vehicleDataPath")))
	{
		printf("%s usage:\n"
			"%s "
			"--vehicleDataPath=<path to the [PHYSX_ROOT]/snippets/media/vehicledata folder containing the vehiclejson files to be loaded> \n",
			snippetName, snippetName);
		return false;
	}
	vehicleDataPath = argv[1] + strlen("--vehicleDataPath=");
	return true;
}

}//namespace snippetvehicle2
