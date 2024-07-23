// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "core/math.h"
#include "core/memory.h"

#include "scene/scene.h"

#include "px/core/px_physics_engine.h"

#include "rendering/debug_visualization.h"

#include "geometry/mesh_builder.h"

#include "dx/dx_buffer.h"
#include "dx/dx_context.h"

namespace era_engine::physics
{
	using namespace physx;

	struct px_cloth_component;

	struct px_cloth_render_component
	{
		px_cloth_render_component() = default;

		NODISCARD::std::tuple<dx_vertex_buffer_group_view, dx_vertex_buffer_group_view, dx_index_buffer_view, submesh_info> getRenderData(const px_cloth_component& cloth);

		ref<dx_index_buffer> indexBuffer;
		dx_vertex_buffer_group_view prevFrameVB;
	};

	struct px_cloth_system
	{
		px_cloth_system(PxU32 numX, PxU32 numZ, const PxVec3& position = PxVec3(0, 0, 0), PxReal particleSpacing = 0.2f, PxReal totalClothMass = 10.f);

		~px_cloth_system();

		void setWind(const PxVec3& wind);

		PxVec3 getWind() const;

		void syncPositionBuffer();

		void translate(const PxVec4& position);

		void translate(const vec3& position);

		void update(bool visualize = false, ldr_render_pass* ldrRenderPass = nullptr);

		void debugVisualize(ldr_render_pass* ldrRenderPass) const;

		PxVec4* posBuffer = nullptr;
		PxU32 numParticles{};

	private:
#if PX_PARTICLE_USE_ALLOCATOR

		eallocator allocator;

#endif

		PxPBDMaterial* material = nullptr;
		PxPBDParticleSystem* particleSystem = nullptr;
		PxParticleClothBuffer* clothBuffer = nullptr;
	};

	struct px_cloth_component : px_physics_component_base
	{
		px_cloth_component() = default;
		px_cloth_component(uint32 handle, int nX, int nZ, const vec3& position = vec3(0, 0, 0));

		virtual ~px_cloth_component() {}

		vec3* getPositions() const;

		virtual void release(bool release = true) override;

		void update(bool visualize = false, ldr_render_pass* ldrRenderPass = nullptr) const;

		void translate(const vec3& position);

		uint32 numX{};
		uint32 numZ{};

	private:
		ref<px_cloth_system> clothSystem;
		vec3* positions = nullptr;
	};
}

#include "core/reflect.h"

namespace era_engine
{
	REFLECT_STRUCT(physics::px_cloth_component,
		(numX, "NumX"),
		(numZ, "numZ")
	);
}