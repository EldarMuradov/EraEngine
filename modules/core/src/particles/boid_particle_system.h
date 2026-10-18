// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "particles/particles.h"

#include "dx/dx_texture.h"

#include "rendering/material.h"

#include "boid_particle_system.hlsli"

namespace era_engine
{
    struct ERA_CORE_API BoidParticleSystem : ParticleSystem
    {
        static void initialize_pipeline();

        BoidParticleSystem() {}
        BoidParticleSystem(uint32 max_num_particles, float emit_rate);
        void initialize(uint32 max_num_particles, float emit_rate);

        virtual void update(struct DXCommandList* cl, const CommonParticleSimulationData& common, float dt) override;
        void render(TransparentRenderPass* render_pass);

        float emit_rate;
        BoidParticleSettings settings;

    private:
        static DXPipeline emit_pipeline;
        static DXPipeline simulate_pipeline;
        static DXPipeline render_pipeline;

        ref<MultiMesh> cartoon_mesh;
        DXVertexBufferGroupView skinned_vertex_buffer;
        float time = 0.f;

        struct BoidMaterial
        {
        };

        struct BoidPipeline : ParticleRenderPipeline<BoidMaterial>
        {
            PIPELINE_SETUP_DECL;
            PARTICLE_PIPELINE_RENDER_DECL(BoidMaterial);
        };
    };
}