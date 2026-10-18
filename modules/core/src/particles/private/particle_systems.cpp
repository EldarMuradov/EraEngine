// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "particles/particle_systems.h"
#include "particles/fire_particle_system.h"
#include "particles/smoke_particle_system.h"
#include "particles/boid_particle_system.h"
#include "particles/debris_particle_system.h"

namespace era_engine
{
    void load_all_particle_system_pipelines()
    {
        FireParticleSystem::initialize_pipeline();
        SmokeParticleSystem::initialize_pipeline();
        BoidParticleSystem::initialize_pipeline();
        DebrisParticleSystem::initialize_pipeline();
    }
}