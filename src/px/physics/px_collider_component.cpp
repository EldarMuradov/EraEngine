#include "pch.h"
#include "px_collider_component.h"
#include <asset/file_registry.h>

px_collider_component_base::~px_collider_component_base()
{
	PX_RELEASE(shape)
}

PxTriangleMesh* px_triangle_mesh_collider_builder::createMeshShape(mesh_asset* asset, unsigned int size)
{
	submesh_asset root = asset->submeshes[0];

	const auto& triangles = root.triangles;
	const auto& positions = root.positions;

	size_t trianglesCount = triangles.size();
	size_t verticesCount = positions.size();

	std::vector<physx::PxU32> indices(trianglesCount * 3);

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

	std::vector<physx::PxVec3> vertices(verticesCount);
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

px_box_collider::~px_box_collider()
{
}

bool px_box_collider::createShape()
{
	auto material = px_physics_engine::getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);
	shape = px_physics_engine::getPhysics()->createShape(physx::PxBoxGeometry(height, length, width), *material);

	return true;
}

px_sphere_collider::~px_sphere_collider()
{

}

bool px_sphere_collider::createShape()
{
	auto material = px_physics_engine::getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);
	shape = px_physics_engine::getPhysics()->createShape(physx::PxSphereGeometry(radius), *material);

	return true;
}

px_capsule_collider::~px_capsule_collider()
{
}

bool px_capsule_collider::createShape()
{
	auto material = px_physics_engine::getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);
	shape = px_physics_engine::getPhysics()->createShape(physx::PxCapsuleGeometry(radius, height / 2.0), *material);

	return true;
}

px_triangle_mesh_collider::~px_triangle_mesh_collider()
{
	if (asset)
	{
		delete asset;
		asset = nullptr;
	}
}

bool px_triangle_mesh_collider::createShape()
{
	px_triangle_mesh_collider_builder builder;
	PxTriangleMesh* mesh = builder.createMeshShape(asset, model_size);

	auto material = px_physics_engine::getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);
	shape = px_physics_engine::getPhysics()->createShape(PxTriangleMeshGeometry(mesh, PxMeshScale(model_size)), &material, PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE);

	PX_RELEASE(mesh);
	return true;
}