#pragma once 

#include "core_api.h"

#include "animation/skeleton.h"

#include "ecs/component.h"

#include <optional>

namespace era_engine::animation
{
	class ERA_CORE_API SkeletonComponent : public Component
	{
	public:
		SkeletonComponent(ref<Entity::EcsData> _data);
		~SkeletonComponent() override;

		ERA_VIRTUAL_REFLECT(Component)

	public:
		ref<Skeleton> skeleton = nullptr;

		std::optional<SkeletonPose> current_pose;

		bool draw_sceleton = false;
	};
}