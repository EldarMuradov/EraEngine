#include "core/ecs/camera_holder_component.h"
#include "ecs/serialization/component_registry.h"
#include "ecs/serialization/serialization_meta.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<CameraHolderComponent>("CameraHolderComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
			.constructor<ref<Entity::EcsData>>();
		ComponentRegistry::register_component<CameraHolderComponent>();
	}


	CameraHolderComponent::CameraHolderComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		type = ATTACHED_TO_TRS;
	}

	CameraHolderComponent::~CameraHolderComponent()
	{
	}

	void CameraHolderComponent::set_render_camera(RenderCamera* _camera)
	{
		camera = _camera;
	}

	RenderCamera* CameraHolderComponent::get_render_camera() const
	{
		return camera;
	}

	CameraHolderComponent::CameraType CameraHolderComponent::get_camera_type() const
	{
		return type;
	}

	void CameraHolderComponent::set_camera_type(CameraType _type)
	{
		type = _type;
	}

	
}