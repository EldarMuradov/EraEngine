#pragma once

#include "core_api.h"
#include "ecs/component.h"

#include "geometry/mesh.h"

namespace era_engine
{
    class ERA_CORE_API ClothRenderComponent : public Component
    {
    public:
        ClothRenderComponent(ref<Entity::EcsData> _data);

        std::tuple<DXVertexBufferGroupView, DXVertexBufferGroupView, DXIndexBufferView, SubmeshInfo> get_render_data();

        ERA_VIRTUAL_REFLECT(Component)
    public:
        std::function<std::tuple<DXVertexBufferGroupView, DXVertexBufferGroupView, DXIndexBufferView, SubmeshInfo>(ClothRenderComponent*)> get_data_internal;

        ref<DXIndexBuffer> index_buffer;
        DXVertexBufferGroupView prev_frame_vb;
    };
}