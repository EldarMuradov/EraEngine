#pragma once

#include "core_api.h"

#include "ecs/component.h"

namespace era_engine
{
    class PrefabAsset;

    // Added to every entity spawned from a prefab.
    class ERA_CORE_API PrefabInstanceComponent final : public Component
    {
    public:
        PrefabInstanceComponent() = default;
        PrefabInstanceComponent(ref<Entity::EcsData> _data);
        ~PrefabInstanceComponent() override;

        ref<PrefabAsset> prefab;
        uint32 local_id = 0;
        uint64 instance_id = 0;

        ERA_VIRTUAL_REFLECT(Component)
    };
}
