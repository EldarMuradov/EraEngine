#include "physics/vehicles/vehicle_system.h"
#include "physics/vehicles/vehicle_simulation.h"
#include "physics/core/physics.h"
#include "physics/collisions_holder_root_component.h"
#include "physics/material.h"
#include "physics/shape_utils.h"

#include <ecs/update_groups.h>
#include <ecs/base_components/name_component.h>

#include <core/log.h>
#include <core/cpu_profiling.h>
#include <core/debug/debug_var.h>

#include <imgui/imgui.h>
#include <core/ecs/input_receiver_component.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::physics
{
    RTTR_REGISTRATION
    {
        using namespace rttr;

        registration::class_<VehicleSystem>("VehicleSystem")
            .constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))

            .method("update", &VehicleSystem::update)(metadata("update_group", update_types::PHYSICS),
                metadata("Before", std::vector<std::string>{"PhysicsSystem::update"}))

            .method("draw_debug_data", &VehicleSystem::draw_debug_data)(metadata("update_group", update_types::BEFORE_RENDER))

            .method("process_door_input", &VehicleSystem::process_door_input)(metadata("update_group", update_types::GAMEPLAY_NORMAL));
    }

    static constexpr float WHEEL_SHAPE_SCALE = 0.95f;

    static DebugVar<bool> show_vehicle_debug_data = DebugVar<bool>("physics.vehicles.show_debug_data", true);

    template <typename ComponentType>
    static ComponentType* get_parent_component(World* world, Entity::Handle entity_handle)
    {
        Entity parent = world->get_entity(world->get_entity(entity_handle).get_parent_handle());
        return parent.is_valid() ? parent.get_component_if_exists<ComponentType>() : nullptr;
    }

    static W4VehicleComponent* get_parent_vehicle(World* world, Entity::Handle entity_handle)
    {
        W4VehicleComponent* vehicle = get_parent_component<W4VehicleComponent>(world, entity_handle);
        return vehicle != nullptr && vehicle->is_created() ? vehicle : nullptr;
    }

    VehicleSystem::VehicleSystem(World* _world)
        : System(_world)
    {
    }

    VehicleSystem::~VehicleSystem()
    {
    }

    void VehicleSystem::init()
    {
        entt::registry& registry = world->get_registry();
        registry.on_destroy<W4VehicleComponent>().connect<&VehicleSystem::on_vehicle_released>(this);
        registry.on_destroy<VehicleDoorComponent>().connect<&VehicleSystem::on_door_released>(this);
        registry.on_destroy<VehicleShapeComponent>().connect<&VehicleSystem::on_shape_released>(this);

        w4_vehicles_group = world->group(components_group<TransformComponent, W4VehicleComponent>);
        wheels_group = world->group(components_group<TransformComponent, VehicleWheelComponent>);
        doors_group = world->group(components_group<TransformComponent, VehicleDoorComponent>);
        steering_wheels_group = world->group(components_group<TransformComponent, VehicleSteeringWheelComponent>);
    }

    void VehicleSystem::update(float dt)
    {
        ZoneScopedN("VehicleSystem::update");

        for (auto [entity_handle, transform_component, vehicle] : w4_vehicles_group.each())
        {
            const bool config_changed = !(vehicle.config == vehicle.built_config);
            if (vehicle.simulation == nullptr ? (!vehicle.build_failed || config_changed) : config_changed)
            {
                create_vehicle(world->get_entity(entity_handle), vehicle);
            }
        }

        PhysicsEngine::execute_write([&]()
            {
                for (auto [entity_handle, transform_component, vehicle] : w4_vehicles_group.each())
                {
                    VehicleSimulation* simulation = vehicle.simulation;
                    if (simulation == nullptr)
                    {
                        continue;
                    }

                    const VehicleDriverInput input = read_driver_input(world->get_entity(entity_handle), vehicle);
                    const float forward_speed = simulation->get_forward_speed();

                    const VehicleCommands commands = vehicle.controller.update(input, vehicle.config.control, vehicle.config.steering, forward_speed, dt);

                    simulation->apply_commands(commands);
                    simulation->simulate(dt);
                    simulation->apply_stability_assist(vehicle.config.control.stability_assist);

                    VehicleDebugData& debug_data = vehicle.debug_data;
                    debug_data.forward_speed = forward_speed;
                    debug_data.engine_rpm = simulation->get_engine_rpm();
                    debug_data.gear = simulation->get_current_gear();
                    debug_data.grounded_wheels = simulation->get_nb_grounded_wheels();
                    debug_data.steer_angle = simulation->get_steer_angle();
                    debug_data.commands = commands;
                }
            });

        update_wheels();
        update_doors(dt);
        update_steering_wheels();
    }

    void VehicleSystem::update_wheels()
    {
        for (auto [entity_handle, transform_component, wheel] : wheels_group.each())
        {
            const W4VehicleComponent* vehicle = get_parent_vehicle(world, entity_handle);
            if (vehicle == nullptr || wheel.wheel_id >= vehicle->simulation->get_nb_wheels())
            {
                continue;
            }

            const physx::PxTransform pose = vehicle->simulation->get_wheel_local_pose(wheel.wheel_id, wheel.should_spin);
            const trs& local_transform = transform_component.get_local_transform();
            transform_component.set_local_transform(trs(create_vec3(pose.p), create_quat(pose.q), local_transform.scale));

            if (wheel.should_spin)
            {
                Entity entity = world->get_entity(entity_handle);
                physx::PxShape* wheel_shape = vehicle->simulation->get_wheel_shape(wheel.wheel_id);
                const VehicleShapeComponent* shape_component = entity.get_component_if_exists<VehicleShapeComponent>();
                if (shape_component == nullptr || shape_component->get_shape() != wheel_shape)
                {
                    bind_shape(entity, wheel_shape, vehicle->collision_type);
                }
            }
        }
    }

    void VehicleSystem::process_door_input(float dt)
    {
        ZoneScopedN("VehicleSystem::process_door_input");

        for (auto [entity_handle, transform_component, door] : doors_group.each())
        {
            const InputReceiverComponent* input_receiver = get_parent_component<InputReceiverComponent>(world, entity_handle);
            if (input_receiver != nullptr && input_receiver->get_frame_input().keyboard['F'].press_event)
            {
                door.toggle();
            }
        }
    }

    void VehicleSystem::update_doors(float dt)
    {
        using namespace physx;

        const ref<Physics>& physics = PhysicsEngine::get_physics_core();

        for (auto [entity_handle, transform_component, door] : doors_group.each())
        {
            const float target = std::clamp(door.open, 0.0f, 1.0f);
            const float step = door.speed * dt;
            door.current += std::clamp(target - door.current, -step, step);

            const trs& local_transform = transform_component.get_local_transform();
            const quat rotation = quat(vec3(0.0f, 1.0f, 0.0f), deg2rad(door.max_angle * door.current));
            const bool moved = !(rotation == local_transform.rotation);
            if (moved)
            {
                transform_component.set_local_transform(trs(local_transform.position, rotation, local_transform.scale));
            }

            W4VehicleComponent* vehicle = get_parent_vehicle(world, entity_handle);
            if (vehicle == nullptr || door.collision_hull.size() < 4)
            {
                continue;
            }

            const PxTransform local_pose(create_PxVec3(local_transform.position), create_PxQuat(rotation));
            const bool needs_shape = door.shape == nullptr || door.shape_generation != vehicle->generation;

            if (!needs_shape && !moved)
            {
                continue;
            }

            PhysicsEngine::execute_write([&]()
                {
                    if (needs_shape)
                    {
                        std::vector<PxVec3> points;
                        for (const vec3& point : door.collision_hull)
                        {
                            points.push_back(create_PxVec3(point));
                        }

                        door.shape = VehicleSimulation::create_convex_shape(*physics->get_physics(),
                            physics->get_cooking_params(),
                            *physics->get_default_material()->get_native_material(),
                            points);
                        door.shape_generation = vehicle->generation;

                        if (door.shape == nullptr)
                        {
                            return;
                        }

                        door.shape->setFlags(PxShapeFlag::eSIMULATION_SHAPE | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eVISUALIZATION);
                        ShapeUtils::setup_filtering(world, door.shape, static_cast<uint32>(vehicle->collision_type), std::nullopt);
                        vehicle->simulation->get_rigid_body()->attachShape(*door.shape);
                        door.shape->release();

                        bind_shape(world->get_entity(entity_handle), door.shape, vehicle->collision_type);
                    }

                    if (door.shape != nullptr)
                    {
                        door.shape->setLocalPose(local_pose);
                    }
                });
        }
    }

    void VehicleSystem::update_steering_wheels()
    {
        for (auto [entity_handle, transform_component, steering_wheel] : steering_wheels_group.each())
        {
            const W4VehicleComponent* vehicle = get_parent_vehicle(world, entity_handle);
            if (vehicle == nullptr)
            {
                continue;
            }

            const trs& local_transform = transform_component.get_local_transform();
            const quat rotation = quat(normalize(steering_wheel.axis), vehicle->get_debug_data().steer_angle * steering_wheel.ratio);
            transform_component.set_local_transform(trs(local_transform.position, rotation, local_transform.scale));
        }
    }

    void VehicleSystem::on_door_released(entt::registry& registry, entt::entity entity_handle)
    {
        VehicleDoorComponent* door = registry.try_get<VehicleDoorComponent>(entity_handle);
        if (door == nullptr || door->shape == nullptr)
        {
            return;
        }

        const W4VehicleComponent* vehicle = get_parent_vehicle(world, static_cast<Entity::Handle>(entity_handle));
        if (vehicle != nullptr && door->shape_generation == vehicle->generation)
        {
            VehicleShapeComponent* shape_component = registry.try_get<VehicleShapeComponent>(entity_handle);
            if (shape_component != nullptr && shape_component->shape == door->shape)
            {
                shape_component->shape = nullptr;
            }

            PhysicsEngine::execute_write([&]()
                {
                    vehicle->simulation->get_rigid_body()->detachShape(*door->shape);
                });
        }
        door->shape = nullptr;
    }

    void VehicleSystem::on_shape_released(entt::registry& registry, entt::entity entity_handle)
    {
        using namespace physx;

        VehicleShapeComponent* shape_component = registry.try_get<VehicleShapeComponent>(entity_handle);
        if (shape_component == nullptr || shape_component->shape == nullptr)
        {
            return;
        }

        PhysicsEngine::execute_write([&]()
            {
                PxRigidActor* actor = shape_component->shape->getActor();
                if (actor == nullptr)
                {
                    return;
                }

                // Shapes of a removed part fall back to the chassis owner on the vehicle root.
                Component* fallback = nullptr;
                if (actor->userData != nullptr)
                {
                    Entity root = static_cast<Component*>(actor->userData)->get_entity();
                    VehicleShapeComponent* root_shape = root.is_valid() ? root.get_component_if_exists<VehicleShapeComponent>() : nullptr;
                    if (root_shape != nullptr && root_shape != shape_component && root_shape->shape != nullptr)
                    {
                        fallback = root_shape;
                    }
                }

                std::vector<PxShape*> shapes(actor->getNbShapes());
                actor->getShapes(shapes.data(), static_cast<PxU32>(shapes.size()));
                for (PxShape* shape : shapes)
                {
                    if (shape->userData == static_cast<Component*>(shape_component))
                    {
                        shape->userData = fallback;
                    }
                }
            });

        // The actor owns the shape, ~ShapeComponent must not release it.
        shape_component->shape = nullptr;
    }

    VehicleShapeComponent* VehicleSystem::bind_shape(Entity entity, physx::PxShape* shape, CollisionType collision_type)
    {
        VehicleShapeComponent* shape_component = entity.get_component_if_exists<VehicleShapeComponent>();
        if (shape_component == nullptr)
        {
            shape_component = entity.add_component<VehicleShapeComponent>();
        }

        shape_component->shape = shape;
        shape_component->collision_type.get_silent_for_write() = collision_type;
        shape->userData = static_cast<Component*>(shape_component);
        return shape_component;
    }

    void VehicleSystem::unbind_shapes(const physx::PxRigidActor* actor)
    {
        for (auto [entity_handle, shape_component] : world->get_registry().view<VehicleShapeComponent>().each())
        {
            if (shape_component.shape != nullptr && shape_component.shape->getActor() == actor)
            {
                shape_component.shape->userData = nullptr;
                shape_component.shape = nullptr;
            }
        }
    }

    void VehicleSystem::draw_debug_data(float dt)
    {
        if (!show_vehicle_debug_data.get() || w4_vehicles_group.empty())
        {
            return;
        }

        if (!ImGui::Begin("Vehicle Debug Data"))
        {
            ImGui::End();
            return;
        }

        for (auto [entity_handle, transform_component, vehicle] : w4_vehicles_group.each())
        {
            const VehicleDebugData& debug_data = vehicle.debug_data;
            const std::string name = world->get_entity(entity_handle).get_component<NameComponent>()->get_name();

            if (!vehicle.is_created())
            {
                ImGui::Text("%s: not created%s", name.c_str(), vehicle.build_failed ? " (invalid config, see log)" : "");
                continue;
            }

            const char* gear = debug_data.gear < 0 ? "R" : (debug_data.gear == 0 ? "N" : nullptr);

            ImGui::Text("%s", name.c_str());
            ImGui::Text("  %.1f km/h  %.0f rpm  gear %s", debug_data.forward_speed * 3.6f, debug_data.engine_rpm, gear ? gear : std::to_string(debug_data.gear).c_str());
            ImGui::Text("  throttle %.2f  brake %.2f  handbrake %.0f  steer %+.2f%s",
                debug_data.commands.throttle,
                debug_data.commands.brake,
                debug_data.commands.handbrake,
                debug_data.commands.steer,
                debug_data.commands.reverse ? "  [reverse]" : "");
            ImGui::Text("  grounded wheels %u", debug_data.grounded_wheels);
        }

        ImGui::End();
    }

    void VehicleSystem::create_vehicle(Entity entity, W4VehicleComponent& vehicle)
    {
        using namespace physx;

        const std::string name_string = entity.get_component<NameComponent>()->get_name();
        const char* name = name_string.c_str();

        auto simulation = std::make_unique<VehicleSimulation>();
        simulation->name = name;

        std::string error;
        if (!simulation->build_params(vehicle.config, error))
        {
            if (vehicle.simulation != nullptr)
            {
                LOG_ERROR("Vehicle '%s': %s The config change is reverted.", name, error.c_str());
                vehicle.config = vehicle.built_config;
                return;
            }

            LOG_ERROR("Vehicle '%s': %s", name, error.c_str());
            vehicle.built_config = vehicle.config;
            vehicle.build_failed = true;
            return;
        }

        vehicle.built_config = vehicle.config;
        vehicle.build_failed = true;

        const ref<Physics>& physics = PhysicsEngine::get_physics_core();
        const CollisionsHolderRootComponent* collisions_holder_rc = world->get_root_component<CollisionsHolderRootComponent>();
        const uint32 collision_type = static_cast<uint32>(vehicle.collision_type);

        PxTransform pose = create_PxTransform(entity.get_component<TransformComponent>()->get_world_transform());
        PxVec3 linear_velocity(0.0f);
        PxVec3 angular_velocity(0.0f);

        bool created = false;
        PxShape* chassis_shape = nullptr;

        PhysicsEngine::execute_write([&]()
            {
                if (vehicle.simulation != nullptr)
                {
                    PxRigidBody* old_body = vehicle.simulation->get_rigid_body();
                    pose = old_body->getGlobalPose();
                    linear_velocity = old_body->getLinearVelocity();
                    angular_velocity = old_body->getAngularVelocity();
                }

                release_vehicle(vehicle);

                if (!simulation->create(*physics->get_physics(),
                        physics->get_cooking_params(),
                        *physics->get_default_material()->get_native_material(),
                        *physics->get_scene()))
                {
                    return;
                }

                PxRigidBody* body = simulation->get_rigid_body();
                body->setGlobalPose(pose);
                body->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
                body->setActorFlag(PxActorFlag::eVISUALIZATION, true);

                const PxVehiclePhysXActor& actor = simulation->mPhysXState.physxActor;

                PxShape* shapes[PxVehicleLimits::eMAX_NB_WHEELS + 1];
                const PxU32 nb_shapes = body->getShapes(shapes, PxVehicleLimits::eMAX_NB_WHEELS + 1);
                for (PxU32 i = 0; i < nb_shapes; ++i)
                {
                    const bool is_wheel = std::find(std::begin(actor.wheelShapes), std::end(actor.wheelShapes), shapes[i]) != std::end(actor.wheelShapes);

                    shapes[i]->setFlags(PxShapeFlag::eSIMULATION_SHAPE | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eVISUALIZATION);
                    ShapeUtils::setup_filtering(world, shapes[i], collision_type, std::nullopt);

                    if (!is_wheel)
                    {
                        chassis_shape = shapes[i];
                    }
                    else
                    {
                        // Ground support comes from the suspension raycasts.
                        PxConvexMeshGeometry geometry = static_cast<const PxConvexMeshGeometry&>(shapes[i]->getGeometry());
                        geometry.scale = PxMeshScale(WHEEL_SHAPE_SCALE);
                        shapes[i]->setGeometry(geometry);

                        PxFilterData filter_data = shapes[i]->getSimulationFilterData();
                        filter_data.word2 |= static_cast<uint32>(FilterData::NON_STATIC_ONLY_FLAG);
                        shapes[i]->setSimulationFilterData(filter_data);
                    }
                }

                simulation->road_query_filter.collision_mask = collisions_holder_rc != nullptr ? collisions_holder_rc->get_collision_filter(collision_type) : 0;

                ASSERT(chassis_shape != nullptr);
                Component* chassis_owner = bind_shape(entity, chassis_shape, vehicle.collision_type);
                for (PxU32 i = 0; i < nb_shapes; ++i)
                {
                    if (shapes[i] != chassis_shape)
                    {
                        shapes[i]->userData = chassis_owner;
                    }
                }

                body->setName(simulation->name.c_str());
                body->userData = static_cast<Component*>(&vehicle);

                physics->get_scene()->addActor(*body);

                PxRigidDynamic* dynamic_body = body->is<PxRigidDynamic>();
                dynamic_body->setLinearVelocity(linear_velocity);
                dynamic_body->setAngularVelocity(angular_velocity);

                created = true;
            });

        if (!created)
        {
            LOG_ERROR("Vehicle '%s': failed to create PhysX vehicle.", name);
            if (simulation->get_rigid_body() != nullptr)
            {
                PhysicsEngine::execute_write([&]()
                    {
                        simulation->destroy();
                    });
            }
            return;
        }

        vehicle.simulation = simulation.release();
        ++vehicle.generation;

        physics->register_actor_locked(vehicle.simulation->get_rigid_body(), &vehicle);
        vehicle.controller.reset();
        vehicle.debug_data = VehicleDebugData();
        vehicle.build_failed = false;
    }

    void VehicleSystem::release_vehicle(VehicleBaseComponent& vehicle)
    {
        if (vehicle.simulation == nullptr)
        {
            return;
        }

        PhysicsEngine::get_physics_core()->unregister_actor_locked(vehicle.simulation->get_rigid_body());

        PhysicsEngine::execute_write([&]()
            {
                unbind_shapes(vehicle.simulation->get_rigid_body());
                vehicle.simulation->destroy();
                RELEASE_PTR(vehicle.simulation)
            });
    }

    VehicleDriverInput VehicleSystem::read_driver_input(Entity entity, const W4VehicleComponent& vehicle) const
    {
        const InputReceiverComponent* input_receiver_component = entity.get_component_if_exists<InputReceiverComponent>();
        if (input_receiver_component == nullptr)
        {
            return vehicle.driver_input;
        }

        const UserInput& input = input_receiver_component->get_frame_input();

        VehicleDriverInput result;
        result.throttle = (input.keyboard['W'].down || input.keyboard[KEY_UP].down) ? 1.0f : 0.0f;
        result.brake = (input.keyboard['S'].down || input.keyboard[KEY_DOWN].down) ? 1.0f : 0.0f;

        const bool left = input.keyboard['A'].down || input.keyboard[KEY_LEFT].down;
        const bool right = input.keyboard['D'].down || input.keyboard[KEY_RIGHT].down;
        result.steer = (left ? 1.0f : 0.0f) - (right ? 1.0f : 0.0f);

        result.handbrake = input.keyboard[KEY_SPACE].down;
        return result;
    }

    void VehicleSystem::on_vehicle_released(entt::registry& registry, entt::entity entity_handle)
    {
        if (W4VehicleComponent* vehicle = registry.try_get<W4VehicleComponent>(entity_handle))
        {
            release_vehicle(*vehicle);
        }
    }
}
