#include "game/camera/gameplay_tpp_camera_component.h"

#include <ecs/entity.h>
#include <ecs/serialization/component_registry.h>

#include <rttr/registration>

namespace era_engine
{
    RTTR_REGISTRATION
    {
        using namespace rttr;
        registration::class_<GameplayTppCameraComponent::OrbitState>("GameplayTppCameraOrbitState")
            .property("yaw", &GameplayTppCameraComponent::OrbitState::yaw)
            .property("pitch", &GameplayTppCameraComponent::OrbitState::pitch)
            .property("distance", &GameplayTppCameraComponent::OrbitState::distance)
            .property("height", &GameplayTppCameraComponent::OrbitState::height);

        registration::class_<GameplayTppCameraComponent>("GameplayTppCameraComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("camera_sensitivity", &GameplayTppCameraComponent::camera_sensitivity)
            .property("camera_min_pitch", &GameplayTppCameraComponent::camera_min_pitch)
            .property("camera_max_pitch", &GameplayTppCameraComponent::camera_max_pitch)
            .property("camera_smooth_speed", &GameplayTppCameraComponent::camera_smooth_speed)
            .property("orbit", &GameplayTppCameraComponent::orbit);
        ComponentRegistry::register_component<GameplayTppCameraComponent>();
    }

    GameplayTppCameraComponent::GameplayTppCameraComponent(ref<Entity::EcsData> _data)
        : Component(_data)
    {
    }


}