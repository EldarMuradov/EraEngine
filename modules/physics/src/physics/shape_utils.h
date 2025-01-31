#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include <core/math.h>

#include <ecs/entity.h>

namespace era_engine
{
	struct MeshAsset;
}

namespace era_engine::physics
{

	class ERA_PHYSICS_API ShapeUtils final
	{
		ShapeUtils() = delete;
	public:
		static physx::PxConvexMesh* build_convex_mesh(const MeshAsset* asset, const vec3& size);
		static physx::PxTriangleMesh* build_triangle_mesh(const MeshAsset* asset, const vec3& size);

		static void get_filter_data(physx::PxRigidActor* actor, std::vector<physx::PxFilterData>& out_filter_data);

		static bool is_trigger(const physx::PxFilterData& data);

		static void set_filter_data(physx::PxRigidActor* actor, const std::vector<physx::PxFilterData>& filter_data);

		static void setup_filtering(physx::PxShape* shape, physx::PxU32 filter_group, physx::PxU32 filter_mask);

		static void enable_shape_visualization(physx::PxShape* shape, const bool enable);
		static void enable_shape_in_contact_tests(physx::PxShape* shape, const bool enable);
		static void enable_shape_in_scene_query_tests(physx::PxShape* shape, const bool enable);
	};

}