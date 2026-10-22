#pragma once

#include "physics_api.h"

#include "physics/vehicles/w4_vehicle_component.h"
#include "physics/vehicles/tank_vehicle_component.h"

#include <ecs/system.h>
#include <ecs/base_components/transform_component.h>

#include <entt/entt.hpp>

namespace era_engine::physics
{

    class VehicleSystem : public System
    {
    public:
        VehicleSystem(World* _world);
        ~VehicleSystem() override;

        void init() override;
        void update(float dt) override;
        void draw_debug_data(float dt);
        void process_door_input(float dt);

        void on_vehicle_released(entt::registry& registry, entt::entity entity_handle);

        ERA_VIRTUAL_REFLECT(System)

    private:
        void update_wheels();
        void update_doors(float dt);
        void update_steering_wheels();
        void on_door_released(entt::registry& registry, entt::entity entity_handle);
        void on_shape_released(entt::registry& registry, entt::entity entity_handle);
        void create_vehicle(Entity entity, W4VehicleComponent& vehicle);
        void release_vehicle(VehicleBaseComponent& vehicle);

        VehicleShapeComponent* bind_shape(Entity entity, physx::PxShape* shape, CollisionType collision_type);
        void unbind_shapes(const physx::PxRigidActor* actor);

        VehicleDriverInput read_driver_input(Entity entity, const W4VehicleComponent& vehicle) const;

    private:
        entt::group<entt::owned_t<>, entt::get_t<TransformComponent,
                                         W4VehicleComponent>>
            w4_vehicles_group;

        entt::group<entt::owned_t<>, entt::get_t<TransformComponent,
                                         VehicleWheelComponent>>
            wheels_group;

        entt::group<entt::owned_t<>, entt::get_t<TransformComponent,
                                         VehicleDoorComponent>>
            doors_group;

        entt::group<entt::owned_t<>, entt::get_t<TransformComponent,
                                         VehicleSteeringWheelComponent>>
            steering_wheels_group;
    };
}
