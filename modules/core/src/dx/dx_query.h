// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "dx/dx.h"

namespace era_engine
{
    struct DXTimestampQueryHeap
    {
        void initialize(uint32 max_count);

        DXQueryHeap heap;
    };
}