#include "core/ecs/camera_holder_component.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<CameraHolderComponent>("CameraHolderComponent")
			.constructor<ref<Entity::EcsData>>();
	}


	CameraHolderComponent::CameraHolderComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		type = ATTACHED_TO_TRS;
	}

	CameraHolderComponent::~CameraHolderComponent()
	{
	}

	void CameraHolderComponent::set_render_camera(render_camera* _camera)
	{
		camera = _camera;
	}

	render_camera* CameraHolderComponent::get_render_camera() const
	{
		return camera;
	}

	CameraHolderComponent::CameraType CameraHolderComponent::get_camera_type() const
	{
		return type;
	}

	
}