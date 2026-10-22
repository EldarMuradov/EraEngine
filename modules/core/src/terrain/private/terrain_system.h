#pragma once

#include "ecs/system.h"

namespace era_engine
{
    class TerrainSystem final : public System
    {
    public:
        TerrainSystem(World* _world);
        ~TerrainSystem() override;

        void init() override;
        void update(float dt) override;

        ERA_VIRTUAL_REFLECT(System)
    };
}
