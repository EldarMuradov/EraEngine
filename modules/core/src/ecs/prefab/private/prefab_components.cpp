#include "ecs/prefab/transient_component.h"
#include "ecs/prefab/prefab_instance_component.h"
#include "ecs/prefab/prefab_asset.h"

#include "ecs/serialization/component_registry.h"
#include "ecs/serialization/serialization_meta.h"

#include <rttr/registration>

namespace era_engine
{
    RTTR_REGISTRATION
    {
        using namespace rttr;

        registration::class_<TransientComponent>("TransientComponent")(metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
            .constructor<ref<Entity::EcsData>>();
        ComponentRegistry::register_component<TransientComponent>();

        registration::class_<PrefabInstanceComponent>("PrefabInstanceComponent")(metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
            .constructor<ref<Entity::EcsData>>()
            .property_readonly("local_id", &PrefabInstanceComponent::local_id)
            .property_readonly("instance_id", &PrefabInstanceComponent::instance_id);
        ComponentRegistry::register_component<PrefabInstanceComponent>();
    }

    TransientComponent::TransientComponent(ref<Entity::EcsData> _data)
        : Component(_data)
    {
    }

    TransientComponent::~TransientComponent()
    {
    }

    PrefabInstanceComponent::PrefabInstanceComponent(ref<Entity::EcsData> _data)
        : Component(_data)
    {
    }

    PrefabInstanceComponent::~PrefabInstanceComponent()
    {
    }
}
