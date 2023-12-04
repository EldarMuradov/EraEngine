#pragma once
#include "px/core/px_physics_engine.h"
#include <dx/dx_buffer.h>
#include <geometry/mesh.h>
#include <asset/model_asset.h>

enum class px_collider_type : uint8
{
	Box,
	Sphere,
	Capsule,
	TriangleMesh,
	ConvexMesh,
	Plane,
	BoundingBox
};

struct px_triangle_mesh_collider_builder
{
	PxTriangleMesh* createMeshShape(mesh_asset* asset, unsigned int size);
};

void enableShapeInContactTests(PxShape* shape) noexcept;
void disableShapeInContactTests(PxShape* shape) noexcept;
void enableShapeInSceneQueryTests(PxShape* shape) noexcept;
void disableShapeInSceneQueryTests(PxShape* shape) noexcept;

struct px_collider_component_base
{
	px_collider_component_base(px_collider_type collType) noexcept : type(collType) {};
	px_collider_component_base() = default;
	virtual ~px_collider_component_base();

	physx::PxShape* getShape() const noexcept { return shape; }
	void setShape(physx::PxShape* newShape) noexcept { shape = newShape; }

	virtual bool createShape() { return false; }

	px_collider_type getType() const noexcept { return type; }

protected:
	px_collider_type type = px_collider_type::Box;

	physx::PxShape* shape = nullptr;
};

struct px_box_collider_component : px_collider_component_base 
{
	px_box_collider_component(float h, float l, float w) noexcept 
	{
		type = px_collider_type::Box;
		height = h;
		length = l;
		width = w;
	};

	~px_box_collider_component();

	bool createShape() override;

private:
	float height, length, width;
};

struct px_sphere_collider_component : px_collider_component_base
{
	px_sphere_collider_component(float r) noexcept
	{
		radius = r;
		type = px_collider_type::Sphere;
	};

	~px_sphere_collider_component();

	bool createShape() override;

private:
	float radius;
};

struct px_capsule_collider_component : px_collider_component_base
{
	px_capsule_collider_component(float r, float h) noexcept
	{
		type = px_collider_type::Capsule;
		radius = r;
		height = h;
	};

	~px_capsule_collider_component();

	bool createShape() override;

private:
	float height, radius;
};

struct px_bounding_box 
{
	physx::PxVec3 minCorner;
	physx::PxVec3 maxCorner;
};

px_bounding_box calculateBoundingBox(const std::vector<physx::PxVec3>& positions);

void createMeshFromBoundingBox(const px_bounding_box& box, std::vector<physx::PxVec3>& vertices, std::vector<uint32_t>& indices);

struct px_bounding_box_collider_component : px_collider_component_base
{
	px_bounding_box_collider_component(unsigned int size, mesh_asset* as) noexcept : asset(as), model_size(size)
	{
		type = px_collider_type::BoundingBox;
	};

	~px_bounding_box_collider_component();

	bool createShape() override;

private:
	mesh_asset* asset = nullptr;
	unsigned int model_size = 0;
};

struct px_triangle_mesh_collider_component : px_collider_component_base
{
	px_triangle_mesh_collider_component(unsigned int size, mesh_asset* as) noexcept : asset(as), model_size(size)
	{
		type = px_collider_type::TriangleMesh;
	};

	~px_triangle_mesh_collider_component();

	bool createShape() override;

private:
	mesh_asset* asset = nullptr;
	unsigned int model_size = 0;
};