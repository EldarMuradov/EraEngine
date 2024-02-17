#include "pch.h"
#include "asset.h"
#include "core/random.h"

static random_number_generator rng = time(0);

NODISCARD asset_handle asset_handle::generate()
{
	return asset_handle(rng.randomUint64());
}