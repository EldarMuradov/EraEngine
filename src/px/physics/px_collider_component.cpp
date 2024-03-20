#include "pch.h"
#include "px_collider_component.h"
#include <asset/file_registry.h>
#include <extensions/PxTetMakerExt.h>
#include <px/core/px_extensions.h>

namespace physics 
{
	px_collider_component_base::~px_collider_component_base()
	{
	}

	NODISCARD PxTriangleMesh* px_triangle_mesh_collider_builder::buildMesh(mesh_asset* asset, float size)
	{
		size_t submeshesCount = asset->submeshes.size();

		PxArray<PxVec3> vertices;
		PxArray<PxU32> indices;

		PxArray<PxVec3> outputVertices;
		PxArray<PxU32> outputIndices;
		PxReal maximalTriangleEdgeLength = 0.0f;

		size_t totalTrianglesCount = 0;

		for (size_t s = 0; s < submeshesCount; s++)
		{
			submesh_asset root = asset->submeshes[s];

			const auto& triangles = root.triangles;
			const auto& positions = root.positions;

			size_t trianglesCount = triangles.size();
			size_t verticesCount = positions.size();

			totalTrianglesCount += trianglesCount;

			for (size_t i = 0; i < trianglesCount; i += 3)
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

			for (size_t i = 0; i < verticesCount; i++)
			{
				vertices.pushBack(PxVec3(positions[i].x, positions[i].y, positions[i].z) * size);
			}
		}

		PxI32 targetTriangleCount = totalTrianglesCount < 5000 ? totalTrianglesCount : totalTrianglesCount * 0.66f;
		
		PxTetMaker::simplifyTriangleMesh(vertices, indices, targetTriangleCount, maximalTriangleEdgeLength, outputVertices, outputIndices);

		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = outputVertices.size();
		meshDesc.points.stride = sizeof(physx::PxVec3);
		meshDesc.points.data = &outputVertices[0];

		meshDesc.triangles.count = outputIndices.size() / 3;
		meshDesc.triangles.stride = 3 * sizeof(PxU32);
		meshDesc.triangles.data = &outputIndices[0];

		px_triangle_mesh_builder mesh_adapter;

		return mesh_adapter.createTriangleMesh(meshDesc);
	}

	px_box_collider_component::~px_box_collider_component()
	{
	}

	bool px_box_collider_component::createShape()
	{
		const auto& physics = physics_holder::physicsRef->getPhysics();
		material = physics->createMaterial(0.5f, 0.5f, 0.6f);
		shape = physics->createShape(PxBoxGeometry(height, length, width), *material);

		enableShapeInSceneQueryTests(shape);
		enableShapeInContactTests(shape);

		physics_holder::physicsRef->colliders.emplace(this);

		return true;
	}

	px_sphere_collider_component::~px_sphere_collider_component()
	{

	}

	bool px_sphere_collider_component::createShape()
	{
		const auto& physics = physics_holder::physicsRef->getPhysics();
		material = physics->createMaterial(0.5f, 0.5f, 0.6f);
		shape = physics->createShape(PxSphereGeometry(radius), *material);

		enableShapeInSceneQueryTests(shape);
		enableShapeInContactTests(shape);

		physics_holder::physicsRef->colliders.emplace(this);

		return true;
	}

	px_capsule_collider_component::~px_capsule_collider_component()
	{
	}

	bool px_capsule_collider_component::createShape()
	{
		const auto& physics = physics_holder::physicsRef->getPhysics();
		material = physics->createMaterial(0.5f, 0.5f, 0.6f);
		shape = physics->createShape(PxCapsuleGeometry(radius, height / 2.0f), *material);

		enableShapeInSceneQueryTests(shape);
		enableShapeInContactTests(shape);

		physics_holder::physicsRef->colliders.emplace(this);

		return true;
	}

	px_triangle_mesh_collider_component::~px_triangle_mesh_collider_component()
	{

	}

	bool px_triangle_mesh_collider_component::createShape()
	{
		px_triangle_mesh_collider_builder builder;
		PxTriangleMesh* mesh = builder.buildMesh(asset, modelSize);

		const auto& physics = physics_holder::physicsRef->getPhysics();
		material = physics->createMaterial(0.5f, 0.5f, 0.6f);
		shape = physics->createShape(PxTriangleMeshGeometry(mesh, PxMeshScale(modelSize)), *material);

		enableShapeInSceneQueryTests(shape);
		enableShapeInContactTests(shape);

		PX_RELEASE(mesh);

		physics_holder::physicsRef->colliders.emplace(this);

		return true;
	}

	px_bounding_box_collider_component::~px_bounding_box_collider_component()
	{
	}

	bool px_bounding_box_collider_component::createShape()
	{
		submesh_asset root = asset->submeshes[0];

		const auto& positions = root.positions;

		size_t verticesCount = positions.size();

		::std::vector<PxVec3> vertices;
		for (size_t i = 0; i < verticesCount; i++)
		{
			vertices.push_back(PxVec3(positions[i].x, positions[i].y, positions[i].z) * modelSize);
		}

		auto result = calculateBoundingBox(vertices);
		::std::vector<PxVec3> vts;
		::std::vector<uint32_t> inds;

		createMeshFromBoundingBox(result, vts, inds);

		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = vts.size();
		meshDesc.points.stride = sizeof(PxVec3);
		meshDesc.points.data = vts.data();

		meshDesc.triangles.count = inds.size() / 3;
		meshDesc.triangles.stride = 3 * sizeof(PxU32);
		meshDesc.triangles.data = inds.data();

		px_triangle_mesh_builder mesh_adapter;
		auto mesh = mesh_adapter.createTriangleMesh(meshDesc);

		const auto& physics = physics_holder::physicsRef->getPhysics();
		material = physics->createMaterial(0.5f, 0.5f, 0.6f);
		shape = physics->createShape(PxTriangleMeshGeometry(mesh, PxMeshScale(modelSize)), *material);

		PX_RELEASE(mesh);

		physics_holder::physicsRef->colliders.emplace(this);

		return true;
	}

	void enableShapeVisualization(PxShape* shape) noexcept
	{
		shape->setFlag(PxShapeFlag::eVISUALIZATION, true);
	}

	void disableShapeVisualization(PxShape* shape) noexcept
	{
		shape->setFlag(PxShapeFlag::eVISUALIZATION, false);
	}

	void enableShapeInContactTests(PxShape* shape) noexcept
	{
		shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
	}

	void disableShapeInContactTests(PxShape* shape) noexcept
	{
		shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
	}

	void enableShapeInSceneQueryTests(PxShape* shape) noexcept
	{
		shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
	}

	void disableShapeInSceneQueryTests(PxShape* shape) noexcept
	{
		shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
	}

	NODISCARD px_bounding_box calculateBoundingBox(const ::std::vector<physx::PxVec3>& positions)
	{
		px_bounding_box box;
		if (positions.empty())
			return box;

		box.minCorner = positions[0];
		box.maxCorner = positions[0];

		for (const auto& position : positions)
		{
			box.minCorner = physx::min(box.minCorner, position);
			box.maxCorner = physx::max(box.maxCorner, position);
		}

		return box;
	}

	void createMeshFromBoundingBox(const px_bounding_box& box, ::std::vector<physx::PxVec3>&vertices, ::std::vector<uint32_t>& indices)
	{
		PxVec3 p0 = box.minCorner;
		PxVec3 p1 = PxVec3(box.maxCorner.x, box.minCorner.y, box.minCorner.z);
		PxVec3 p2 = PxVec3(box.maxCorner.x, box.minCorner.y, box.maxCorner.z);
		PxVec3 p3 = PxVec3(box.minCorner.x, box.minCorner.y, box.maxCorner.z);
		PxVec3 p4 = PxVec3(box.minCorner.x, box.maxCorner.y, box.minCorner.z);
		PxVec3 p5 = PxVec3(box.maxCorner.x, box.maxCorner.y, box.minCorner.z);
		PxVec3 p6 = box.maxCorner;
		PxVec3 p7 = PxVec3(box.minCorner.x, box.maxCorner.y, box.maxCorner.z);

		vertices.push_back({ p0 });
		vertices.push_back({ p1 });
		vertices.push_back({ p2 });
		vertices.push_back({ p3 });
		vertices.push_back({ p4 });
		vertices.push_back({ p5 });
		vertices.push_back({ p6 });
		vertices.push_back({ p7 });

		uint32_t boxIndices[36] =
		{
			0, 1, 2,
			2, 3, 0,
			4, 5, 6,
			6, 7, 4,
			0, 1, 5,
			5, 4, 0,
			2, 3, 7,
			7, 6, 2,
			0, 4, 7,
			7, 3, 0,
			1, 5, 6,
			6, 2, 1
		};

		for (int i = 0; i < 36; i++)
			indices.push_back(boxIndices[i]);
	}

	px_convex_mesh_collider_component::~px_convex_mesh_collider_component()
	{
	}

	bool px_convex_mesh_collider_component::createShape()
	{
		px_convex_mesh_collider_builder builder;
		PxConvexMesh* mesh = builder.buildMesh(asset, modelSize);

		const auto& physics = physics_holder::physicsRef->getPhysics();
		material = physics->createMaterial(0.5f, 0.5f, 0.6f);
		shape = physics->createShape(PxConvexMeshGeometry(mesh, PxMeshScale(modelSize)), *material);

		enableShapeInSceneQueryTests(shape);
		enableShapeInContactTests(shape);

		PX_RELEASE(mesh);

		physics_holder::physicsRef->colliders.emplace(this);

		return true;
	}

	px_plane_collider_component::~px_plane_collider_component()
	{
	}

	bool px_plane_collider_component::createShape()
	{
		const auto& physics = physics_holder::physicsRef->getPhysics();
		material = physics->createMaterial(0.5f, 0.5f, 0.6f);
		plane = PxCreatePlane(*physics, PxPlane(createPxVec3(position), createPxVec3(normal)), *material);

		physics_holder::physicsRef->getScene()->addActor(*plane);

		return true;
	}

	NODISCARD PxConvexMesh* px_convex_mesh_collider_builder::buildMesh(mesh_asset* asset, float size)
	{
		submesh_asset root = asset->submeshes[0];

		const auto& positions = root.positions;

		size_t verticesCount = positions.size();

		PxArray<PxVec3> vertices;
		for (size_t i = 0; i < verticesCount; i++)
			vertices.pushBack(PxVec3(positions[i].x, positions[i].y, positions[i].z) * size);

		PxConvexMeshDesc meshDesc;
		meshDesc.points.count = vertices.size();
		meshDesc.points.stride = sizeof(PxVec3);
		meshDesc.points.data = &vertices[0];
		meshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eQUANTIZE_INPUT;

#if PX_GPU_BROAD_PHASE
		meshDesc.flags |= PxConvexFlag::eGPU_COMPATIBLE;
#endif

		px_convex_mesh_builder mesh_adapter;

		return mesh_adapter.createConvexMesh(meshDesc);
	}

}