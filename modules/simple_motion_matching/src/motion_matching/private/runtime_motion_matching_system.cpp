#include "motion_matching/runtime_motion_matching_system.h"
#include "animation/animation.h"
#include "animation/skinning.h"

#include "rendering/debug_visualization.h"

#include "core/cpu_profiling.h"
#include "core/string.h"
#include "core/debug/debug_var.h"

#include "engine/engine.h"

#include "ecs/update_groups.h"
#include "ecs/base_components/transform_component.h"

#include "motion_matching/motion_matching_database.h"
#include "motion_matching/common.h"
#include "motion_matching/motion_matching_component.h"

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
        world->get_registry().on_construct<MotionMatchingComponent>().connect<&RuntimeMotionMatchingSystem::on_controller_created>(this);
	}

	void RuntimeMotionMatchingSystem::update(float dt)
	{
		using namespace animation;

        for (auto [handle, transform_component, controller] : world->group(components_group<TransformComponent, MotionMatchingComponent>).each())
        {

        }
	}

    void RuntimeMotionMatchingSystem::on_controller_created(entt::registry& registry, entt::entity entity)
    {
        MotionMatchingComponent& controller = registry.get<MotionMatchingComponent>(entity);
        TransformComponent& transform = registry.get<TransformComponent>(entity);

        // TODO
    }
}