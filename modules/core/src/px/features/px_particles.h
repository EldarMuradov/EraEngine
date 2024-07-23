// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "core/math.h"
#include "core/memory.h"

#include "rendering/debug_visualization.h"

#include "geometry/mesh_builder.h"

#include "dx/dx_buffer.h"
#include "dx/dx_context.h"

#include "scene/scene.h"

#include "px/core/px_physics_engine.h"

namespace era_engine::physics
{
	using namespace physx;

	struct px_particles_component;

	struct px_particles_render_component
	{
		px_particles_render_component() = default;
		~px_particles_render_component() {}

		ref<dx_index_buffer> indexBuffer;
		dx_vertex_buffer_group_view prevFrameVB;
	};

	struct px_particle_system
	{
		px_particle_system() = default;
		px_particle_system(PxU32 numX, PxU32 numY, PxU32 numZ, bool fluid, const PxVec3& position = PxVec3(0, 0, 0), PxU32 maxVols = 1, PxU32 maxNh = 96);

		~px_particle_system();

		void setWind(const PxVec3& wind);

		PxVec3 getWind() const;

		void translate(const vec3& position);

		void translate(const PxVec4& position);
		
		void syncPositionBuffer();

		void update(bool visualize = false, ldr_render_pass* ldrRenderPass = nullptr)
		{
			syncPositionBuffer();
			if (visualize && ldrRenderPass)
				debugVisualize(ldrRenderPass);
		}

		void debugVisualize(ldr_render_pass* ldrRenderPass);

		constexpr uint32 getMaxVolumes() const { return maxVolumes; }
		constexpr uint32 getMaxNeighborhood() const { return maxNeighborhood; }
		constexpr uint32 getMaxParticles() const { return maxParticles; }

		PxVec4* diffuseLifeBuffer = nullptr;
		PxVec4* posBuffer = nullptr;

	private:
#if PX_PARTICLE_USE_ALLOCATOR

		eallocator allocator;

#endif

		PxPBDParticleSystem* particleSystem = nullptr;
		PxParticleAndDiffuseBuffer* particleBuffer = nullptr;
		PxPBDMaterial* material = nullptr;

		PxU32 maxNeighborhood{};
		PxU32 maxVolumes{};
		PxU32 maxParticles{};
	};

	struct px_particles_component : px_physics_component_base
	{
		px_particles_component() = default;
		px_particles_component(uint32 handle, const vec3& position, int nX, int nY, int nZ, bool fluid = false);

		virtual ~px_particles_component() {}

		virtual void release(bool release = true) override;

		void update(bool visualize = false, ldr_render_pass* ldrRenderPass = nullptr) const;

		uint32 numX{};
		uint32 numY{};
		uint32 numZ{};

		ref<px_particle_system> particleSystem;
	};
}

#include "core/reflect.h"

namespace era_engine
{
	REFLECT_STRUCT(physics::px_particles_component,
		(numX, "NumX"),
		(numY, "numY"),
		(numZ, "numZ")
	);
}