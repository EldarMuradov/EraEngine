#include "core/ecs/tags_component.h"
#include "ecs/serialization/component_registry.h"
#include "ecs/serialization/serialization_meta.h"

#include <rttr/registration>

namespace era_engine
{

    RTTR_REGISTRATION
    {
        using namespace rttr;
        registration::class_<TagsComponent>("TagsComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
            .constructor<ref<Entity::EcsData>>();
        ComponentRegistry::register_component<TagsComponent>();
    }


    TagsComponent::TagsComponent(ref<Entity::EcsData> _data)
        : Component(_data)
    {
    }

    TagsComponent::~TagsComponent()
    {
    }

    void TagsComponent::add_tag(const std::string& tag)
    {
        container.add_tag(tag);
    }

    bool TagsComponent::remove_tag(const std::string& tag)
    {
        return container.remove_tag(tag);
    }

    bool TagsComponent::has_tag(const std::string& tag) const
    {
        return container.has_tag(tag);
    }

    size_t TagsComponent::size() const
    {
        return container.size();
    }

    void TagsComponent::clear()
    {
        container.clear();
    }

    const TagsContainer& TagsComponent::get_container() const
    {
        return container;
    }


}