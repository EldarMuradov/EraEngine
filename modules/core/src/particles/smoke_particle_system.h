// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "particles/particles.h"

#include "dx/dx_texture.h"

#include "rendering/material.h"

#include "smoke_particle_system.hlsli"

namespace era_engine
{
    struct ERA_CORE_API SmokeParticleSystem : ParticleSystem
    {
        static void initialize_pipeline();

        SmokeParticleSystem() {}
        SmokeParticleSystem(uint32 max_num_particles, float emit_rate, const std::string& texture_filename, uint32 cols, uint32 rows, vec3 pos);
        void initialize(uint32 max_num_particles, float emit_rate, const std::string& texture_filename, uint32 cols, uint32 rows, vec3 pos);

        virtual void update(struct DXCommandList* cl, const CommonParticleSimulationData& common, float dt) override;
        void render(TransparentRenderPass* render_pass);

        float emit_rate;
        SmokeParticleSettings settings;

    private:
        TextureAtlasCB atlas_cb;
        DXTextureAtlas atlas;

        vec3 position;

        static DXPipeline emit_pipeline;
        static DXPipeline simulate_pipeline;
        static DXPipeline render_pipeline;

        struct SmokeMaterial
        {
            DXTextureAtlas atlas;
            DXDynamicConstantBuffer cbv;
        };

        struct SmokePipeline : ParticleRenderPipeline<SmokeMaterial>
        {
            PIPELINE_SETUP_DECL;
            PARTICLE_PIPELINE_RENDER_DECL(SmokeMaterial);
        };
    };
}