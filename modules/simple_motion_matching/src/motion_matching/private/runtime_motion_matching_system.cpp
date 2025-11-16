#include "motion_matching/runtime_motion_matching_system.h"
#include "motion_matching/features/motion_matching_feature_set.h"
#include "motion_matching/motion_matching_database.h"
#include "motion_matching/common.h"
#include "motion_matching/motion_matching_component.h"

#include "animation/animation.h"

#include "core/string.h"
#include "core/debug/debug_var.h"

#include "ecs/update_groups.h"
#include "ecs/base_components/transform_component.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<RuntimeMotionMatchingSystem>("RuntimeMotionMatchingSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("motion_matching")))
			.method("update", &RuntimeMotionMatchingSystem::update)(metadata("update_group", update_types::GAMEPLAY_BEFORE_PHYSICS_CONCURRENT));
	}

	RuntimeMotionMatchingSystem::RuntimeMotionMatchingSystem(World* _world)
		: System(_world)
	{
	}

	RuntimeMotionMatchingSystem::~RuntimeMotionMatchingSystem()
	{
	}

	void RuntimeMotionMatchingSystem::init()
	{
	}

	void RuntimeMotionMatchingSystem::update(float dt)
	{
		using namespace animation;

        for (auto [handle, transform_component, mm_controller, animation_component] : world->group(components_group<TransformComponent, MotionMatchingComponent, AnimationComponent>).each())
        {
			mm_controller.search_timer -= dt;
			if (mm_controller.search_timer <= 0.0f)
			{
				mm_controller.search_timer = mm_controller.search_time;

				MotionMatchingFeatureSet feature_set;
				SearchResult search_result = mm_controller.search_animation(feature_set, "LOCOMOTION");
				
				if (search_result.animation != nullptr)
				{
					animation_component.current_animation = search_result.animation;
					animation_component.current_anim_position = search_result.anim_position;
				}
			}
        }
	}
}