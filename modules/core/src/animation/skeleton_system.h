#pragma once

#include "ecs/system.h"

namespace era_engine
{
    class RendererHolderRootComponent;
}

namespace era_engine::animation
{
    class SkeletonSystem final : public System
    {
    public:
        SkeletonSystem(World* _world);
        ~SkeletonSystem() override;

        void init() override;
        void update(float dt) override;

        void draw_skeletons(float dt);

        ERA_VIRTUAL_REFLECT(System)
    private:
        RendererHolderRootComponent* renderer_holder_rc = nullptr;
    };
}