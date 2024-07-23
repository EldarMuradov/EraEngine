// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "EraScriptingCPPDecls/EraScriptingCPPDecls.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    era_engine::enative_scripting_factory* factory = new era_engine::enative_scripting_factory();

    return TRUE;
}

