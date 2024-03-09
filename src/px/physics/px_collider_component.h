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
	NODISCARD PxTriangleMesh* buildMesh(mesh_asset* asset, unsigned int size);
};

struct px_convex_mesh_collider_builder
{
	NODISCARD PxConvexMesh* buildMesh(mesh_asset* asset, unsigned int size);
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

	NODISCARD physx::PxShape* getShape() const noexcept { return shape; }
	void setShape(physx::PxShape* newShape) noexcept { shape = newShape; }

	virtual bool createShape() { return false; }

	NODISCARD px_collider_type getType() const noexcept { return type; }

	virtual void release() { PX_RELEASE(shape) PX_RELEASE(material) }

protected:
	px_collider_type type = px_collider_type::Box;

	physx::PxShape* shape = nullptr;
	physx::PxMaterial* material = nullptr;
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

NODISCARD px_bounding_box calculateBoundingBox(const std::vector<physx::PxVec3>& positions);

void createMeshFromBoundingBox(const px_bounding_box& box, std::vector<physx::PxVec3>& vertices, std::vector<uint32_t>& indices);

struct px_bounding_box_collider_component : px_collider_component_base
{
	px_bounding_box_collider_component(unsigned int size, mesh_asset* as) noexcept : asset(as), modelSize(size)
	{
		type = px_collider_type::BoundingBox;
	};

	~px_bounding_box_collider_component();

	bool createShape() override;

	void release() override { PX_RELEASE(shape) RELEASE_PTR(asset) PX_RELEASE(material) }

private:
	mesh_asset* asset = nullptr;
	unsigned int modelSize = 0;
};

struct px_triangle_mesh_collider_component : px_collider_component_base
{
	px_triangle_mesh_collider_component(unsigned int size, mesh_asset* as) noexcept : asset(as), modelSize(size)
	{
		type = px_collider_type::TriangleMesh;
	};

	~px_triangle_mesh_collider_component();

	bool createShape() override;

	void release() override { PX_RELEASE(shape) RELEASE_PTR(asset) PX_RELEASE(material) }

private:
	mesh_asset* asset = nullptr;
	unsigned int modelSize = 0;
};

struct px_plane_collider_component : px_collider_component_base
{
	px_plane_collider_component(const vec3& pos, const vec3& norm = vec3(0.f, 1.f, 0.f)) noexcept : position(pos), normal(norm)
	{
		type = px_collider_type::Plane;
		createShape();
	};

	~px_plane_collider_component();

	bool createShape() override;

	void release() override { PX_RELEASE(shape) PX_RELEASE(plane) PX_RELEASE(material) }

private:
	PxRigidStatic* plane = nullptr;
	vec3 position{};
	vec3 normal{};
};

struct px_convex_mesh_collider_component : px_collider_component_base
{
	px_convex_mesh_collider_component(unsigned int size, mesh_asset* as) noexcept : asset(as), modelSize(size)
	{
		type = px_collider_type::ConvexMesh;
	};

	~px_convex_mesh_collider_component();

	bool createShape() override;

	void release() override { PX_RELEASE(shape) RELEASE_PTR(asset) PX_RELEASE(material) }

private:
	mesh_asset* asset = nullptr;
	unsigned int modelSize = 0;
};