#include "animation/animation.h"
#include "animation/skinning.h"

#include "core/memory.h"
#include "core/random.h"
#include "core/imgui.h"
#include "core/string.h"

#include "geometry/mesh.h"

#include "dx/dx_context.h"

#include "rendering/debug_visualization.h"

#include "ecs/serialization/component_registry.h"

#include <rttr/registration>

#include <algorithm>

namespace era_engine::animation
{
    const char* limb_type_names[] =
        {
            "Unknown",

            "Torso",
            "Head",

            "Upper arm right",
            "Lower arm right",
            "Hand right",

            "Upper arm left",
            "Lower arm left",
            "Hand left",

            "Upper leg right",
            "Lower leg right",
            "Foot right",

            "Upper leg left",
            "Lower leg left",
            "Foot left",
    };

    RTTR_REGISTRATION
    {
        using namespace rttr;
        registration::class_<AnimationComponent>("AnimationComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("current_animation", &AnimationComponent::current_animation)
            .property("current_anim_position", &AnimationComponent::current_anim_position)
            .property("play", &AnimationComponent::play)
            .property("update_skeleton", &AnimationComponent::update_skeleton)
            .property("loop", &AnimationComponent::loop)
            .property("enable_root_motion", &AnimationComponent::enable_root_motion)
            .property("blend_time", &AnimationComponent::blend_time)
            .property("inertial_blend", &AnimationComponent::inertial_blend_inited);
        ComponentRegistry::register_component<AnimationComponent>();
    }

    AnimationComponent::AnimationComponent(ref<Entity::EcsData> _data)
        : Component(_data)
    {
    }

    AnimationComponent::~AnimationComponent()
    {
    }

    void AnimationComponent::on_deserialized()
    {
        if (inertial_blend_inited && inertial_sampler == nullptr)
        {
            const SkeletonComponent* skeleton_component = get_entity().get_component_if_exists<SkeletonComponent>();
            if (skeleton_component != nullptr && skeleton_component->skeleton != nullptr)
            {
                activate_inertial_blend();
            }
            else
            {
                inertial_blend_inited = false;
            }
        }
    }

    void AnimationComponent::activate_inertial_blend()
    {
        const SkeletonComponent* skeleton_component = get_entity().get_component<SkeletonComponent>();

        inertial_sampler = make_ref<InertialBlendSampler>(skeleton_component->skeleton.get());
        inertial_blend_inited = true;
    }

    void AnimationComponent::trigger_reset_inertial_blend(const SkeletonPose& reset_pose, float _blend_time)
    {
        blend_time = _blend_time;
        inertial_sampler->reset_blend_time(blend_time);
        inertial_sampler->init_inertial_blend(reset_pose);
    }
}