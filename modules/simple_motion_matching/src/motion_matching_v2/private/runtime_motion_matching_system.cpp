#include "motion_matching_v2/runtime_motion_matching_system.h"
#include "animation/animation.h"
#include "animation/skinning.h"

#include "rendering/ecs/renderer_holder_root_component.h"
#include "rendering/debug_visualization.h"

#include "core/cpu_profiling.h"
#include "core/memory.h"
#include "core/string.h"
#include "core/ecs/input_reciever_component.h"
#include "core/debug/debug_var.h"

#include "engine/engine.h"

#include "ecs/update_groups.h"
#include "ecs/rendering/mesh_component.h"
#include "ecs/base_components/transform_component.h"

#include "motion_matching_v2/motion_matching_database.h"
#include "motion_matching/common.h"
#include "motion_matching_v2/motion_matching_component.h"

#include <hnswlib/hnsw_all.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<RuntimeMotionMatchingSystem>("RuntimeMotionMatchingSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("render")))
			.method("update", &RuntimeMotionMatchingSystem::update)(metadata("update_group", update_types::RENDER));
	}

	RuntimeMotionMatchingSystem::RuntimeMotionMatchingSystem(World* _world)
		: System(_world)
	{
		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);
	}

	RuntimeMotionMatchingSystem::~RuntimeMotionMatchingSystem()
	{
		allocator->reset();
	}

	void RuntimeMotionMatchingSystem::init()
	{
		allocator = get_transient_object<Allocator>();

        world->get_registry().on_construct<MotionMatchingComponent>().connect<&RuntimeMotionMatchingSystem::on_controller_created>(this);
	}

	void RuntimeMotionMatchingSystem::update(float dt)
	{
		using namespace animation;

        for (auto [handle, transform_component, reciever_component, controller] : world->group(components_group<TransformComponent, InputRecieverComponent, MotionMatchingComponent>).each())
        {

        }
	}

    void RuntimeMotionMatchingSystem::on_controller_created(entt::registry& registry, entt::entity entity)
    {
        MotionMatchingComponent& controller = registry.get<MotionMatchingComponent>(entity);
        TransformComponent& transform = registry.get<TransformComponent>(entity);

        // TODO
    }

    void RuntimeMotionMatchingSystem::draw_trajectory(const slice1d<vec3>& trajectory_positions, const slice1d<quat>& trajectory_rotations, const vec4& color)
    {
        for (int i = 1; i < trajectory_positions.size; i++)
        {
            vec3 point = vec3(trajectory_positions(i).x, trajectory_positions(i).y, trajectory_positions(i).z);
            renderWireSphere(point, 0.05f, color, renderer_holder_rc->ldrRenderPass);

            vec3 dir = trajectory_rotations(i) * vec3(0.0f, 0.0f, 1.0f);
            renderLine(point, point + 0.6f * dir, color, renderer_holder_rc->ldrRenderPass);
            renderLine(vec3(trajectory_positions(i - 1).x, trajectory_positions(i - 1).y, trajectory_positions(i - 1).z), point, color, renderer_holder_rc->ldrRenderPass);
        }
    }

}