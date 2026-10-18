// Copyright (c) 2023-present Eldar Muradov. All rights reserved.


#include "dx/dx_barrier_batcher.h"
#include "dx/dx_command_list.h"
#include "dx/dx_context.h"
#include "dx/dx_descriptor_allocation.h"

namespace era_engine
{
    BarrierBatcher::BarrierBatcher(DXCommandList* cl)
    {
        this->cl = cl;
    }

    BarrierBatcher& BarrierBatcher::transition(const DXResource& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource)
    {
        if (num_barriers == arraysize(barriers))
        {
            submit();
        }

        if (from != to)
        {
            barriers[num_barriers++] = CD3DX12_RESOURCE_BARRIER::Transition(res.Get(), from, to, subresource);
        }
        return *this;
    }

    BarrierBatcher& BarrierBatcher::transition(const ref<DXTexture>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource)
    {
        if (!res)
            return *this;

        return transition(res->resource, from, to, subresource);
    }

    BarrierBatcher& BarrierBatcher::transition(const ref<DXBuffer>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
    {
        if (!res)
            return *this;

        return transition(res->resource, from, to);
    }

    BarrierBatcher& BarrierBatcher::transition_begin(const DXResource& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource)
    {
        if (num_barriers == arraysize(barriers))
        {
            submit();
        }

        if (from != to)
        {
            barriers[num_barriers++] = CD3DX12_RESOURCE_BARRIER::Transition(res.Get(), from, to, subresource, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
        }
        return *this;
    }

    BarrierBatcher& BarrierBatcher::transition_begin(const ref<DXTexture>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource)
    {
        if (!res)
            return *this;

        return transition_begin(res->resource, from, to, subresource);
    }

    BarrierBatcher& BarrierBatcher::transition_begin(const ref<DXBuffer>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
    {
        if (!res)
            return *this;

        return transition_begin(res->resource, from, to);
    }

    BarrierBatcher& BarrierBatcher::transition_end(const DXResource& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource)
    {
        if (num_barriers == arraysize(barriers))
        {
            submit();
        }

        if (from != to)
        {
            barriers[num_barriers++] = CD3DX12_RESOURCE_BARRIER::Transition(res.Get(), from, to, subresource, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);
        }
        return *this;
    }

    BarrierBatcher& BarrierBatcher::transition_end(const ref<DXTexture>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource)
    {
        if (!res)
            return *this;

        return transition_end(res->resource, from, to, subresource);
    }

    BarrierBatcher& BarrierBatcher::transition_end(const ref<DXBuffer>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
    {
        if (!res)
            return *this;

        return transition_end(res->resource, from, to);
    }

    BarrierBatcher& BarrierBatcher::uav(const DXResource& resource)
    {
        if (num_barriers == arraysize(barriers))
        {
            submit();
        }

        barriers[num_barriers++] = CD3DX12_RESOURCE_BARRIER::UAV(resource.Get());
        return *this;
    }

    BarrierBatcher& BarrierBatcher::uav(const ref<DXTexture>& res)
    {
        if (!res)
            return *this;

        return uav(res->resource);
    }

    BarrierBatcher& BarrierBatcher::uav(const ref<DXBuffer>& res)
    {
        if (!res)
            return *this;

        return uav(res->resource);
    }

    BarrierBatcher& BarrierBatcher::aliasing(const DXResource& before, const DXResource& after)
    {
        if (num_barriers == arraysize(barriers))
        {
            submit();
        }

        barriers[num_barriers++] = CD3DX12_RESOURCE_BARRIER::Aliasing(before ? before.Get() : 0, after ? after.Get() : 0);
        return *this;
    }

    BarrierBatcher& BarrierBatcher::aliasing(const ref<DXTexture>& before, const ref<DXTexture>& after)
    {
        return aliasing(before ? before->resource : 0, after ? after->resource : 0);
    }

    BarrierBatcher& BarrierBatcher::aliasing(const ref<DXBuffer>& before, const ref<DXBuffer>& after)
    {
        return aliasing(before ? before->resource : 0, after ? after->resource : 0);
    }

    void BarrierBatcher::submit()
    {
        if (num_barriers)
        {
            cl->barriers(barriers, num_barriers);
            num_barriers = 0;
        }
    }

}