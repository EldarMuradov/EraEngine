#include "pch.h"
#include "px_collider_component.h"
#include <asset/file_registry.h>

px_collider_component_base::~px_collider_component_base()
{
	//PX_RELEASE(shape)
}

PxTriangleMesh* px_triangle_mesh_collider_builder::createMeshShape(mesh_asset* asset, unsigned int size)
{
	submesh_asset root = asset->submeshes[0];

	const auto& triangles = root.triangles;
	const auto& positions = root.positions;

	size_t trianglesCount = triangles.size();
	size_t verticesCount = positions.size();

	std::vector<physx::PxU32> indices;

	for (size_t i = 0; i < trianglesCount; i += 3)
	{
		indices.push_back(triangles[i + 0].a);
		indices.push_back(triangles[i + 0].b);		
		indices.push_back(triangles[i + 0].c);

		indices.push_back(triangles[i + 1].a);
		indices.push_back(triangles[i + 1].b);
		indices.push_back(triangles[i + 1].c);

		indices.push_back(triangles[i + 2].a);
		indices.push_back(triangles[i + 2].b);
		indices.push_back(triangles[i + 2].c);
	}

	std::vector<physx::PxVec3> vertices;
	for (size_t i = 0; i < verticesCount; i++)
	{
		vertices.push_back(PxVec3(positions[i].x, positions[i].y, positions[i].z) * size);
	}

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = vertices.size();
	meshDesc.points.stride = sizeof(physx::PxVec3);
	meshDesc.points.data = vertices.data();

	meshDesc.triangles.count = indices.size() / 3;
	meshDesc.triangles.stride = 3 * sizeof(PxU32);
	meshDesc.triangles.data = indices.data();

	px_triangle_mesh mesh_adapter;

	return mesh_adapter.createTriangleMesh(meshDesc);
}

px_box_collider_component::~px_box_collider_component()
{
}

bool px_box_collider_component::createShape()
{
	auto material = px_physics_engine::getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);
	shape = px_physics_engine::getPhysics()->createShape(physx::PxBoxGeometry(height, length, width), *material);
	enableShapeInSceneQueryTests(shape);
	enableShapeInContactTests(shape);
	return true;
}

px_sphere_collider_component::~px_sphere_collider_component()
{

}

bool px_sphere_collider_component::createShape()
{
	auto material = px_physics_engine::getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);
	shape = px_physics_engine::getPhysics()->createShape(physx::PxSphereGeometry(radius), *material);
	enableShapeInSceneQueryTests(shape);
	enableShapeInContactTests(shape);
	return true;
}

px_capsule_collider_component::~px_capsule_collider_component()
{
}

bool px_capsule_collider_component::createShape()
{
	auto material = px_physics_engine::getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);
	shape = px_physics_engine::getPhysics()->createShape(physx::PxCapsuleGeometry(radius, height / 2.0f), *material);
	enableShapeInSceneQueryTests(shape);
	enableShapeInContactTests(shape);
	return true;
}

px_triangle_mesh_collider_component::~px_triangle_mesh_collider_component()
{
	//if (asset)
	//{
	//	delete asset;
	//	asset = nullptr;
	//}
}

bool px_triangle_mesh_collider_component::createShape()
{
	px_triangle_mesh_collider_builder builder;
	PxTriangleMesh* mesh = builder.createMeshShape(asset, modelSize);

	auto material = px_physics_engine::getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);
	auto physics = px_physics_engine::getPhysics();
	shape = physics->createShape(PxTriangleMeshGeometry(mesh, PxMeshScale(modelSize)), *material);
	enableShapeInSceneQueryTests(shape);
	enableShapeInContactTests(shape);
	PX_RELEASE(mesh);
	return true;
}

px_bounding_box_collider_component::~px_bounding_box_collider_component()
{
	//if (asset)
	//{
	//	delete asset;
	//	asset = nullptr;
	//}
}

bool px_bounding_box_collider_component::createShape()
{
	submesh_asset root = asset->submeshes[0];

	const auto& positions = root.positions;

	size_t verticesCount = positions.size();

	std::vector<physx::PxVec3> vertices;
	for (size_t i = 0; i < verticesCount; i++)
	{
		vertices.push_back(PxVec3(positions[i].x, positions[i].y, positions[i].z) * modelSize);
	}

	auto result = calculateBoundingBox(vertices);
	std::vector<physx::PxVec3> vts;
	std::vector<uint32_t> inds;

	createMeshFromBoundingBox(result, vts, inds);

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = vts.size();
	meshDesc.points.stride = sizeof(physx::PxVec3);
	meshDesc.points.data = vts.data();

	meshDesc.triangles.count = inds.size() / 3;
	meshDesc.triangles.stride = 3 * sizeof(PxU32);
	meshDesc.triangles.data = inds.data();

	px_triangle_mesh mesh_adapter;
	auto mesh = mesh_adapter.createTriangleMesh(meshDesc);

	auto material = px_physics_engine::getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);
	shape = px_physics_engine::getPhysics()->createShape(PxTriangleMeshGeometry(mesh, PxMeshScale(modelSize)), *material);

	PX_RELEASE(mesh);
	return true;
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

px_bounding_box calculateBoundingBox(const std::vector<physx::PxVec3>& positions)
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

void createMeshFromBoundingBox(const px_bounding_box& box, std::vector<physx::PxVec3>& vertices, std::vector<uint32_t>& indices)
{
	physx::PxVec3 p0 = box.minCorner;
	physx::PxVec3 p1 = physx::PxVec3(box.maxCorner.x, box.minCorner.y, box.minCorner.z);
	physx::PxVec3 p2 = physx::PxVec3(box.maxCorner.x, box.minCorner.y, box.maxCorner.z);
	physx::PxVec3 p3 = physx::PxVec3(box.minCorner.x, box.minCorner.y, box.maxCorner.z);
	physx::PxVec3 p4 = physx::PxVec3(box.minCorner.x, box.maxCorner.y, box.minCorner.z);
	physx::PxVec3 p5 = physx::PxVec3(box.maxCorner.x, box.maxCorner.y, box.minCorner.z);
	physx::PxVec3 p6 = box.maxCorner;
	physx::PxVec3 p7 = physx::PxVec3(box.minCorner.x, box.maxCorner.y, box.maxCorner.z);

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
