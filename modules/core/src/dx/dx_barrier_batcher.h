// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "dx/dx.h"

namespace era_engine
{
    struct DXTexture;
    struct DXBuffer;
    struct DXCommandList;

    struct ERA_CORE_API BarrierBatcher
    {
        BarrierBatcher(DXCommandList* cl);
        ~BarrierBatcher() { submit(); }

        BarrierBatcher& transition(const DXResource& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        BarrierBatcher& transition(const ref<DXTexture>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        BarrierBatcher& transition(const ref<DXBuffer>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);

        // Split barriers.
        BarrierBatcher& transition_begin(const DXResource& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        BarrierBatcher& transition_begin(const ref<DXTexture>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        BarrierBatcher& transition_begin(const ref<DXBuffer>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);

        BarrierBatcher& transition_end(const DXResource& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        BarrierBatcher& transition_end(const ref<DXTexture>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        BarrierBatcher& transition_end(const ref<DXBuffer>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);

        BarrierBatcher& uav(const DXResource& res);
        BarrierBatcher& uav(const ref<DXTexture>& res);
        BarrierBatcher& uav(const ref<DXBuffer>& res);

        BarrierBatcher& aliasing(const DXResource& before, const DXResource& after);
        BarrierBatcher& aliasing(const ref<DXTexture>& before, const ref<DXTexture>& after);
        BarrierBatcher& aliasing(const ref<DXBuffer>& before, const ref<DXBuffer>& after);

        void submit();

        DXCommandList* cl;
        CD3DX12_RESOURCE_BARRIER barriers[16];
        uint32 num_barriers = 0;
    };
}