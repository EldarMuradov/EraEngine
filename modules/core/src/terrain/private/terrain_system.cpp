#include "terrain/private/terrain_system.h"
#include "terrain/terrain.h"

#include "core/cpu_profiling.h"

#include "ecs/update_groups.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
    RTTR_REGISTRATION
    {
        using namespace rttr;

        registration::class_<TerrainSystem>("TerrainSystem")
            .constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("render")))
            .method("update", &TerrainSystem::update)(metadata("update_group", update_types::BEFORE_RENDER));
    }

    TerrainSystem::TerrainSystem(World* _world)
        : System(_world)
    {
    }

    TerrainSystem::~TerrainSystem()
    {
    }

    void TerrainSystem::init()
    {
    }

    void TerrainSystem::update(float dt)
    {
        ZoneScopedN("TerrainSystem::update");

        for (auto [entity_handle, terrain] : world->view<TerrainComponent>().each())
        {
            terrain.update();
        }
    }
}
