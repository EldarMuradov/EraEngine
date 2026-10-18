// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "core/system_info.h"
#include "core/string.h"

#include "dx/dx_context.h"

#include <intrin.h>

namespace era_engine
{
    ERA_CORE_API SystemInfo get_system_info()
    {
        SystemInfo system;
        char cpu_name[64] = "";

        int cpu_info[4] = {-1};
        __cpuid(cpu_info, 0x80000000);
        uint32 n_ex_ids = cpu_info[0];
        // Get the information associated with each extended ID
        for (uint32 i = 0x80000000; i <= n_ex_ids; ++i)
        {
            __cpuid(cpu_info, i);
            // Interpret CPU brand string and cache information
            if (i == 0x80000002)
            {
                memcpy(cpu_name,
                    cpu_info,
                    sizeof(cpu_info));
            }
            else if (i == 0x80000003)
            {
                memcpy(cpu_name + 16,
                    cpu_info,
                    sizeof(cpu_info));
            }
            else if (i == 0x80000004)
            {
                memcpy(cpu_name + 32, cpu_info, sizeof(cpu_info));
            }
        }

        MEMORYSTATUSEX memory_state;
        memory_state.dwLength = sizeof(memory_state);

        GlobalMemoryStatusEx(&memory_state);

        system.cpu_name = cpu_name;
        system.main_memory = memory_state.ullTotalPhys;

        DXGI_ADAPTER_DESC1 gpu_desc;
        dx_context.adapter->GetDesc1(&gpu_desc);

        system.gpu_name = wstring_to_string(gpu_desc.Description);
        system.video_memory = gpu_desc.DedicatedVideoMemory;

        return system;
    }

}