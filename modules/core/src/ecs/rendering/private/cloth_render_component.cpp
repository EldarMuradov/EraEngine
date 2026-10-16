#include "ecs/rendering/cloth_render_component.h"
#include "ecs/serialization/component_registry.h"
#include "ecs/serialization/serialization_meta.h"
#include "ecs/world.h"

#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<ClothRenderComponent>("ClothRenderComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
			.constructor<ref<Entity::EcsData>>();
		ComponentRegistry::register_component<ClothRenderComponent>();
	}

	ClothRenderComponent::ClothRenderComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	std::tuple<DXVertexBufferGroupView, DXVertexBufferGroupView, DXIndexBufferView, SubmeshInfo> era_engine::ClothRenderComponent::get_render_data()
	{
		if (get_data_internal != nullptr)
		{
			return get_data_internal(this);
		}
		return std::tuple<DXVertexBufferGroupView, DXVertexBufferGroupView, DXIndexBufferView, SubmeshInfo>();
	}
}