// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "particles/particles.h"

#include "dx/dx_texture.h"

#include "rendering/material.h"

#include "debris_particle_system.hlsli"

namespace era_engine
{
    struct ERA_CORE_API DebrisParticleSystem : ParticleSystem
    {
        static void initialize_pipeline();

        DebrisParticleSystem() {}
        DebrisParticleSystem(uint32 max_num_particles);
        void initialize(uint32 max_num_particles);

        void burst(vec3 position);

        virtual void update(struct DXCommandList* cl, const CommonParticleSimulationData& common, float dt) override;
        void render(TransparentRenderPass* render_pass);

    private:
        static DXPipeline emit_pipeline;
        static DXPipeline simulate_pipeline;
        static DXPipeline render_pipeline;

        struct DebrisMaterial
        {
        };

        struct DebrisPipeline : ParticleRenderPipeline<DebrisMaterial>
        {
            PIPELINE_SETUP_DECL;
            PARTICLE_PIPELINE_RENDER_DECL(DebrisMaterial);
        };

        struct DebrisBurst
        {
            vec3 position;
        };

        std::vector<DebrisBurst> bursts;
    };
}