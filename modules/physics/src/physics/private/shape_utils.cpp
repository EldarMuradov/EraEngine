#include "physics/shape_utils.h"
#include "physics/core/physics.h"
#include "physics/collisions_holder_root_component.h"

#include "core/log.h"
#include "core/traits.h"

#include "geometry/mesh.h"

#include "asset/model_asset.h"
#include "asset/file_registry.h"

#include <extensions/PxTetMakerExt.h>

namespace era_engine::physics
{
	physx::PxConvexMesh* ShapeUtils::build_convex_mesh(const MeshAsset* asset, const vec3& size)
	{
		using namespace physx;

		SubmeshAsset root = asset->submeshes[0];

		const auto& positions = root.positions;

		size_t vertices_count = positions.size();

		PxArray<PxVec3> vertices;
		for (size_t i = 0; i < vertices_count; i++)
		{
			vertices.pushBack(PxVec3(positions[i].x, positions[i].y, positions[i].z) * create_PxVec3(size));
		}

		PxConvexMeshDesc mesh_desc;
		mesh_desc.points.count = vertices.size();
		mesh_desc.points.stride = sizeof(PxVec3);
		mesh_desc.points.data = &vertices[0];
		mesh_desc.flags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eDISABLE_MESH_VALIDATION | PxConvexFlag::eFAST_INERTIA_COMPUTATION;

		try
		{
			if (mesh_desc.isValid())
			{
				PxCookingParams cookingParams = PxCookingParams(PhysicsHolder::physics_ref->get_tolerance_scale());
#if PX_GPU_BROAD_PHASE
				cookingParams.buildGPUData = true;
#endif
				//cookingParams.meshWeldTolerance = 0.15f;
				cookingParams.convexMeshCookingType = PxConvexMeshCookingType::eQUICKHULL;
				cookingParams.meshPreprocessParams = PxMeshPreprocessingFlag::eENABLE_INERTIA;
				return PxCreateConvexMesh(cookingParams, mesh_desc, PhysicsHolder::physics_ref->get_physics()->getPhysicsInsertionCallback());
			}
		}
		catch (...)
		{
			//LOG_ERROR("Physics> Failed to create physics triangle mesh");
		}
		return nullptr;
	}

	physx::PxTriangleMesh* ShapeUtils::build_triangle_mesh(const MeshAsset* asset, const vec3& size)
	{
		using namespace physx;
		size_t submeshes_count = asset->submeshes.size();

		PxArray<PxVec3> vertices;
		PxArray<PxU32> indices;

		PxArray<PxVec3> output_vertices;
		PxArray<PxU32> output_indices;
		PxReal maximal_triangle_edge_length = 0.0f;

		PxVec3 physics_model_size = create_PxVec3(size);

		size_t total_triangles_count = 0;

		for (size_t s = 0; s < submeshes_count; s++)
		{
			SubmeshAsset root = asset->submeshes[s];

			const auto& triangles = root.triangles;
			const auto& positions = root.positions;

			size_t triangles_count = triangles.size();
			size_t vertices_count = positions.size();

			total_triangles_count += triangles_count;

			for (size_t i = 0; i < triangles_count; i += 3)
			{
				indices.pushBack(triangles[i + 0].a);
				indices.pushBack(triangles[i + 0].b);
				indices.pushBack(triangles[i + 0].c);

				indices.pushBack(triangles[i + 1].a);
				indices.pushBack(triangles[i + 1].b);
				indices.pushBack(triangles[i + 1].c);

				indices.pushBack(triangles[i + 2].a);
				indices.pushBack(triangles[i + 2].b);
				indices.pushBack(triangles[i + 2].c);
			}

			for (size_t i = 0; i < vertices_count; i++)
			{
				vertices.pushBack(PxVec3(positions[i].x, positions[i].y, positions[i].z) * physics_model_size);
			}
		}

		PxI32 target_triangle_count = total_triangles_count < 5000 ? total_triangles_count : total_triangles_count * 0.66f;

		PxTriangleMeshDesc mesh_desc;

		if (target_triangle_count != total_triangles_count)
		{
			PxTetMaker::simplifyTriangleMesh(vertices, indices, target_triangle_count, maximal_triangle_edge_length, output_vertices, output_indices);

			mesh_desc.points.count = output_vertices.size();
			mesh_desc.points.stride = sizeof(physx::PxVec3);
			mesh_desc.points.data = &output_vertices[0];

			mesh_desc.triangles.count = output_indices.size() / 3;
			mesh_desc.triangles.stride = 3 * sizeof(PxU32);
			mesh_desc.triangles.data = &output_indices[0];
		}
		else
		{
			mesh_desc.points.count = vertices.size();
			mesh_desc.points.stride = sizeof(PxVec3);
			mesh_desc.points.data = &vertices[0];

			mesh_desc.triangles.count = indices.size() / 3;
			mesh_desc.triangles.stride = 3 * sizeof(PxU32);
			mesh_desc.triangles.data = &indices[0];
		}

		try
		{
			if (mesh_desc.triangles.count > 0 && mesh_desc.isValid())
			{
				PxCookingParams cookingParams = PxCookingParams(PhysicsHolder::physics_ref->get_tolerance_scale());
#if PX_GPU_BROAD_PHASE
				cookingParams.buildGPUData = true;
#endif
				cookingParams.suppressTriangleMeshRemapTable = false;
				cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
				return PxCreateTriangleMesh(cookingParams, mesh_desc);
			}
		}
		catch (...)
		{
			LOG_ERROR("Physics> Failed to create physics triangle mesh");
			throw;
		}
		return nullptr;
	}

	bool ShapeUtils::is_trigger(const physx::PxFilterData& data)
	{
		if (data.word0 != 0xffffffff)
		{
			return false;
		}
		if (data.word1 != 0xffffffff)
		{
			return false;
		}
		if (data.word2 != 0xffffffff)
		{
			return false;
		}
		if (data.word3 != 0xffffffff)
		{
			return false;
		}
		return true;
	}

	void ShapeUtils::setup_filtering(World* world, physx::PxShape* shape, uint32 collision_type, std::optional<uint32> collision_filter_data)
	{
		// - word0 is collision types mask
		// - word1 is mask with types to collide with
		// - word2 is for additional flags
		// - word3 is for user data

		ASSERT(world != nullptr);
		ASSERT(shape != nullptr);

		using namespace physx;

		const CollisionsHolderRootComponent* collisions_holder_rc = world->get_root_component<CollisionsHolderRootComponent>();
		ASSERT(collisions_holder_rc != nullptr);

		PxFilterData physx_sim_filter_data = shape->getSimulationFilterData();
		PxFilterData physx_query_filter_data = shape->getQueryFilterData();

		physx_sim_filter_data.word0 = physx_query_filter_data.word0 = collision_type;
		physx_sim_filter_data.word1 = physx_query_filter_data.word1 = collisions_holder_rc->get_collision_filter(collision_type);

		uint32 additional_flags = (1u << 31);
		set_flag(additional_flags, FilterData::HAS_FILTER_DATA_FLAG, collision_filter_data.has_value());

		physx_sim_filter_data.word2 = physx_query_filter_data.word2 = additional_flags;
		physx_sim_filter_data.word3 = physx_query_filter_data.word3 = collision_filter_data.value_or(0);

		shape->setSimulationFilterData(physx_sim_filter_data);
		shape->setQueryFilterData(physx_query_filter_data);
	}

	void ShapeUtils::enable_shape_visualization(physx::PxShape* shape, const bool enable)
	{
		ASSERT(shape != nullptr);
		shape->setFlag(physx::PxShapeFlag::eVISUALIZATION, enable);
	}

	void ShapeUtils::enable_shape_in_contact_tests(physx::PxShape* shape, const bool enable)
	{
		ASSERT(shape != nullptr);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, enable);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, enable);
	}

	void ShapeUtils::enable_shape_in_scene_query_tests(physx::PxShape* shape, const bool enable)
	{
		ASSERT(shape != nullptr);
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, enable);
	}
}