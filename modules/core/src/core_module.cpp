#include "core_module.h"

#include "engine/engine.h"
#include "ecs/world_system_scheduler.h"

#include <rttr/registration>

namespace era_engine
{
    CoreModule::CoreModule() noexcept
        : IModule("core")
    {
    }

    CoreModule::~CoreModule()
    {
    }

    bool CoreModule::initialize(void* engine)
    {
        IModule::initialize(engine);

        Engine* engine_ptr = static_cast<Engine*>(engine);

        World* world = get_world_by_name("GameWorld");
        WorldSystemScheduler* scheduler = world->get_system_scheduler();

        scheduler->initialize_systems(rttr::type::get_types());

        return true;
    }

    bool CoreModule::terminate()
    {
        IModule::terminate();
        return true;
    }

    RTTR_PLUGIN_REGISTRATION
    {
        using namespace rttr;
        registration::class_<CoreModule>("CoreModule")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .method("initialize", &CoreModule::initialize)
            .method("terminate", &CoreModule::terminate)
            (metadata("module_type", IModule::Type::ENGINE));
    }

}