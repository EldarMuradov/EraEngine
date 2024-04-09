// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "px/core/px_physics_engine.h"
#include <dx/dx_buffer.h>
#include <geometry/mesh.h>
#include <asset/model_asset.h>

namespace physics
{
	using namespace physx;

	enum class px_collider_type : uint8
	{
		None,
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
		NODISCARD PxTriangleMesh* buildMesh(mesh_asset* asset, float size);
	};

	struct px_convex_mesh_collider_builder
	{
		NODISCARD PxConvexMesh* buildMesh(mesh_asset* asset, vec3 size);
	};

	void enableShapeVisualization(PxShape* shape) noexcept;
	void disableShapeVisualization(PxShape* shape) noexcept;

	void enableShapeInContactTests(PxShape* shape) noexcept;
	void disableShapeInContactTests(PxShape* shape) noexcept;

	void enableShapeInSceneQueryTests(PxShape* shape) noexcept;
	void disableShapeInSceneQueryTests(PxShape* shape) noexcept;

	struct px_collider_component_base : px_physics_component_base
	{
		px_collider_component_base(px_collider_type collType) noexcept : type(collType) {};
		px_collider_component_base() = default;
		virtual ~px_collider_component_base();

		NODISCARD PxShape* getShape() const noexcept { return shape; }
		void setShape(PxShape* newShape) noexcept { shape = newShape; }

		virtual bool createShape() { return false; }

		virtual void release(bool release = true) noexcept override { PX_RELEASE(shape) PX_RELEASE(material) }

		px_collider_type type = px_collider_type::None;

	protected:
		PxShape* shape = nullptr;
		PxMaterial* material = nullptr;
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

		float height{}, length{}, width{};
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

		float radius{};
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

		float height{}, radius{};
	};

	struct px_bounding_box
	{
		PxVec3 minCorner;
		PxVec3 maxCorner;
	};

	NODISCARD px_bounding_box calculateBoundingBox(const ::std::vector<PxVec3>& positions);

	void createMeshFromBoundingBox(const px_bounding_box& box, ::std::vector<PxVec3>& vertices, ::std::vector<uint32_t>& indices);

	struct px_bounding_box_collider_component : px_collider_component_base
	{
		px_bounding_box_collider_component(float size, mesh_asset* as) noexcept : asset(as), modelSize(size)
		{
			type = px_collider_type::BoundingBox;
			name = asset->name;
		};

		~px_bounding_box_collider_component();

		bool createShape() override;

		void release(bool release = true) noexcept override { PX_RELEASE(shape) RELEASE_PTR(asset) PX_RELEASE(material) }

		mesh_asset* asset = nullptr;
		float modelSize = 1.0f;
		::std::string name;
	};

	struct px_triangle_mesh_collider_component : px_collider_component_base
	{
		px_triangle_mesh_collider_component(float size, mesh_asset* as) noexcept : asset(as), modelSize(size)
		{
			type = px_collider_type::TriangleMesh;
			name = asset->name;
		};

		~px_triangle_mesh_collider_component();

		bool createShape() override;

		virtual void release(bool release = true) noexcept override { PX_RELEASE(shape) RELEASE_PTR(asset) PX_RELEASE(material) }

		mesh_asset* asset = nullptr;
		float modelSize = 1.0f;
		::std::string name;
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

		virtual void release(bool release = true) noexcept override { PX_RELEASE(shape) PX_RELEASE(plane) PX_RELEASE(material) }

		vec3 position{};
		vec3 normal{};

	private:
		PxRigidStatic* plane = nullptr;
	};

	struct px_convex_mesh_collider_component : px_collider_component_base
	{
		px_convex_mesh_collider_component(vec3 size, mesh_asset* as) noexcept : asset(as), modelSize(size)
		{
			type = px_collider_type::ConvexMesh;
			name = asset->name;
		};

		~px_convex_mesh_collider_component();

		bool createShape() override;

		virtual void release(bool release = true) noexcept override { PX_RELEASE(shape) RELEASE_PTR(asset) PX_RELEASE(material) }

		mesh_asset* asset = nullptr;
		vec3 modelSize = vec3(1.0f);
		::std::string name;
	};

	inline bool isTrigger(const PxFilterData& data)
	{
		if(data.word0 != 0xffffffff)
			return false;
		if(data.word1 != 0xffffffff)
			return false;
		if(data.word2 != 0xffffffff)
			return false;
		if(data.word3 != 0xffffffff)
			return false;
		return true;
	}
}

#include "core/reflect.h"

REFLECT_STRUCT(physics::px_collider_component_base,
	(type, "Type")
);

REFLECT_STRUCT(physics::px_box_collider_component,
	(type, "Type"),
	(height, "Height"),
	(length, "Length"),
	(width, "Width")
);

REFLECT_STRUCT(physics::px_sphere_collider_component,
	(type, "Type"),
	(radius, "Radius")
);

REFLECT_STRUCT(physics::px_capsule_collider_component,
	(type, "Type"),
	(height, "Height"),
	(radius, "Radius")
);

REFLECT_STRUCT(physics::px_bounding_box_collider_component,
	(type, "Type"),
	(modelSize, "Size"),
	(name, "Name")
);

REFLECT_STRUCT(physics::px_triangle_mesh_collider_component,
	(type, "Type"),
	(modelSize, "Size"),
	(name, "Name")
);

REFLECT_STRUCT(physics::px_plane_collider_component,
	(type, "Type")
);

REFLECT_STRUCT(physics::px_convex_mesh_collider_component,
	(type, "Type"),
	(modelSize, "Size"),
	(name, "Name")
);