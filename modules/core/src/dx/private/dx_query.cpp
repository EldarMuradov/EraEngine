// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "dx/dx_query.h"
#include "dx/dx_context.h"

namespace era_engine
{
	void DXTimestampQueryHeap::initialize(uint32 max_count)
	{
		D3D12_QUERY_HEAP_DESC desc;
		desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		desc.Count = max_count;
		desc.NodeMask = 0;

		checkResult(dx_context.device->CreateQueryHeap(&desc, IID_PPV_ARGS(&heap)));
	}
}