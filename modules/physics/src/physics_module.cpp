#include "physics_module.h"

#include "physics/core/physics.h"

#include "engine/engine.h"

#include "ecs/world_system_scheduler.h"

#include <rttr/registration>

namespace era_engine
{

    PhysicsModule::PhysicsModule() noexcept
        : IModule("physics")
    {
    }

    PhysicsModule::~PhysicsModule()
    {
    }

    bool PhysicsModule::initialize(void* engine)
    {
        IModule::initialize(engine);

        physics::PhysicsHolder::physics_ref = make_ref<physics::Physics>();
        physics::PhysicsHolder::physics_ref->start();

        MODULE_REGISTRATION

        return true;
    }

    bool PhysicsModule::terminate()
    {
        physics::PhysicsHolder::physics_ref->release();

        IModule::terminate();
        return true;
    }

    RTTR_PLUGIN_REGISTRATION
    {
        using namespace rttr;
        registration::class_<PhysicsModule>("PhysicsModule")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .method("initialize", &PhysicsModule::initialize)
            .method("terminate", &PhysicsModule::terminate)
            (metadata("module_type", IModule::Type::ENGINE));
    }

}