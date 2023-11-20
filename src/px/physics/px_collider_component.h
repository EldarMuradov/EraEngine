#pragma once
#include "px/core/px_physics_engine.h"
#include <dx/dx_buffer.h>
#include <geometry/mesh.h>
#include <asset/model_asset.h>

enum class px_collider_type 
{
	Box,
	Sphere,
	Capsule,
	TriangleMesh,
	ConvexMesh,
	Plane
};

struct px_triangle_mesh_collider_builder
{
	PxTriangleMesh* createMeshShape(mesh_asset* asset, unsigned int size);
};

class px_collider_component_base
{
public:
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

class px_box_collider : public px_collider_component_base 
{
public:
	px_box_collider(float h, float l, float w) noexcept 
	{
		type = px_collider_type::Box;
		height = h;
		length = l;
		width = w;
	};

	~px_box_collider();

	bool createShape() override;

private:
	float height, length, width;
};

class px_sphere_collider : public px_collider_component_base
{
public:
	px_sphere_collider(float r) noexcept
	{
		radius = r;
		type = px_collider_type::Sphere;
	};

	~px_sphere_collider();

	bool createShape() override;

private:
	float radius;
};

class px_capsule_collider : public px_collider_component_base
{
public:
	px_capsule_collider(float r, float h) noexcept
	{
		type = px_collider_type::Capsule;
		radius = r;
		height = h;
	};

	~px_capsule_collider();

	bool createShape() override;

private:
	float height, radius;
};

class px_triangle_mesh_collider : public px_collider_component_base
{
public:
	px_triangle_mesh_collider(mesh_asset* as, unsigned int size) noexcept : asset(as), model_size(size)
	{
		type = px_collider_type::TriangleMesh;
	};

	~px_triangle_mesh_collider();

	bool createShape() override;

private:
	mesh_asset* asset = nullptr;
	unsigned int model_size = 0;
};