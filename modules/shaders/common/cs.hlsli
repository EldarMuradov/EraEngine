#ifndef CS_HLSLI
#define CS_HLSLI

struct CSInput
{
    uint3 group_id           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 group_thread_id     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 dispatch_thread_id  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  group_index        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

#endif
