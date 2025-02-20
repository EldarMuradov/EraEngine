// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"
#include "core/math.h"
#include "core/random.h"

#include "dx/dx_texture.h"

#include "rendering/render_pass.h"

#include "light_probe.hlsli"

namespace era_engine
{
	struct ERA_CORE_API light_probe_grid
	{
		void initialize(vec3 minCorner, vec3 dimensions, float cellSize);
		void visualize(opaque_render_pass* renderPass);

		void updateProbes(dx_command_list* cl, const struct raytracing_tlas& lightProbeTlas, const common_render_data& common) const;

		NODISCARD light_probe_grid_cb getCB() const { return { minCorner, cellSize, numNodesX, numNodesY, numNodesZ }; }

		vec3 minCorner;
		float cellSize;

		uint32 numNodesX;
		uint32 numNodesY;
		uint32 numNodesZ;
		uint32 totalNumNodes = 0;

		ref<dx_texture> irradiance;
		ref<dx_texture> depth;

		ref<dx_texture> raytracedRadiance;
		ref<dx_texture> raytracedDirectionAndDistance;

	private:
		bool visualizeProbes = false;
		bool visualizeRays = false;
		bool autoRotateRays = true;

		mutable bool rotateRays = false;

		float irradianceUIScale = 1.f;
		float depthUIScale = 1.f;
		float raytracedRadianceUIScale = 1.f;

		mutable quat rayRotation = quat::identity;
		mutable RandomNumberGenerator rng = { 61913 };

		friend struct light_probe_tracer;
		friend struct eeditor;
	};

	void initializeLightProbePipelines();
}