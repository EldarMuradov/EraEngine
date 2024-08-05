// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "px/core/px_physics_engine.h"

#include "core/math.h"

#include "dx/dx_buffer.h"

#include "geometry/mesh.h"

#include "asset/model_asset.h"

#include "scene/scene.h"

namespace era_engine::physics
{
	using namespace physx;

	enum class px_collider_type : uint8
	{
		collider_type_none,
		collider_type_box,
		collider_type_sphere,
		collider_type_capsule,
		collider_type_triangle_mesh,
		collider_type_convex_mesh,
		collider_type_plane,
		collider_type_bounding_box
	};

	struct px_triangle_mesh_collider_builder
	{
		NODISCARD PxTriangleMesh* buildMesh(mesh_asset* asset, const vec3& size);
	};

	struct px_convex_mesh_collider_builder
	{
		NODISCARD PxConvexMesh* buildMesh(mesh_asset* asset, const vec3& size);
	};

	void setupFiltering(PxShape* shape, PxU32 filterGroup, PxU32 filterMask);

	void enableShapeVisualization(PxShape* shape);
	void disableShapeVisualization(PxShape* shape);

	void enableShapeInContactTests(PxShape* shape);
	void disableShapeInContactTests(PxShape* shape);

	void enableShapeInSceneQueryTests(PxShape* shape);
	void disableShapeInSceneQueryTests(PxShape* shape);

	struct px_collider_component_base;

	template<typename T_>
	using IsColliderType = std::enable_if_t<std::is_base_of_v<struct px_collider_component_base, T_>, bool>;

	struct px_collider_component_base : px_physics_component_base
	{
		px_collider_component_base(uint32 handle) : px_physics_component_base(handle) { }
		px_collider_component_base() = default;
		virtual ~px_collider_component_base();

		PxGeometry* getGeometry() const { return geometry; }
		PxShape* getShape() const { return shape; }
		void setShape(PxShape* newShape) { shape = newShape; }

		virtual bool createGeometry() { return false; }

		template<typename T, IsColliderType<T> = true>
		T* is()
		{
			return static_cast<const T*>(this);
		}

		virtual void registerCollider() { physics_holder::physicsRef->collidersMap[entityHandle].push_back(this); }

		virtual void release(bool release = true) override { PX_RELEASE(shape) PX_RELEASE(material) RELEASE_PTR(geometry) }

	protected:
		PxGeometry* geometry = nullptr;
		PxShape* shape = nullptr;
		PxMaterial* material = nullptr;
	};

	struct px_box_collider_component : px_collider_component_base
	{
		px_box_collider_component(uint32 handle, float h, float l, float w)
			: px_collider_component_base(handle)
		{
			height = h;
			length = l;
			width = w;
			registerCollider();
		};

		~px_box_collider_component();

		virtual bool createGeometry() override;

		void registerCollider() override { physics_holder::physicsRef->collidersMap[entityHandle].push_back(this); }

		float height{}, length{}, width{};
	};

	struct px_sphere_collider_component : px_collider_component_base
	{
		px_sphere_collider_component(uint32 handle, float r)
			: px_collider_component_base(handle)
		{
			radius = r;
			registerCollider();
		};

		~px_sphere_collider_component();

		virtual bool createGeometry() override;

		void registerCollider() override { physics_holder::physicsRef->collidersMap[entityHandle].push_back(this); }

		float radius{};
	};

	struct px_capsule_collider_component : px_collider_component_base
	{
		px_capsule_collider_component(uint32 handle, float r, float h)
			: px_collider_component_base(handle)
		{
			radius = r;
			height = h;
			registerCollider();
		};

		~px_capsule_collider_component();

		virtual bool createGeometry() override;

		void registerCollider() override { physics_holder::physicsRef->collidersMap[entityHandle].push_back(this); }

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
		px_bounding_box_collider_component(uint32 handle, vec3 size, mesh_asset* as)
			: px_collider_component_base(handle), asset(as), modelSize(size)
		{
			name = asset->name;
			registerCollider();
		};

		~px_bounding_box_collider_component();

		virtual bool createGeometry() override;

		void release(bool release = true) override { PX_RELEASE(shape) RELEASE_PTR(asset) PX_RELEASE(material) RELEASE_PTR(geometry) }

		void registerCollider() override { physics_holder::physicsRef->collidersMap[entityHandle].push_back(this); }

		mesh_asset* asset = nullptr;
		vec3 modelSize = 1.0f;
		std::string name;
	};

	struct px_triangle_mesh_collider_component : px_collider_component_base
	{
		px_triangle_mesh_collider_component(uint32 handle, mesh_asset* as, vec3 size = vec3(1.0f)) : px_collider_component_base(handle), asset(as), modelSize(size)
		{
			name = asset->name;
			registerCollider();
		};

		~px_triangle_mesh_collider_component();

		virtual bool createGeometry() override;

		virtual void release(bool release = true) override { PX_RELEASE(shape) RELEASE_PTR(asset) PX_RELEASE(material) RELEASE_PTR(geometry) }

		void registerCollider() override { physics_holder::physicsRef->collidersMap[entityHandle].push_back(this); }

		mesh_asset* asset = nullptr;
		vec3 modelSize = 1.0f;
		std::string name;
	};

	struct px_convex_mesh_collider_component : px_collider_component_base
	{
		px_convex_mesh_collider_component(uint32 handle, mesh_asset* as, vec3 size = vec3(1.0f))
			: px_collider_component_base(handle), asset(as), modelSize(size)
		{
			name = asset->name;
			registerCollider();
		};

		~px_convex_mesh_collider_component();

		virtual bool createGeometry() override;

		virtual void release(bool release = true) override { PX_RELEASE(shape) PX_RELEASE(material) RELEASE_PTR(geometry) }

		void registerCollider() override { physics_holder::physicsRef->collidersMap[entityHandle].push_back(this); }

		mesh_asset* asset = nullptr;
		vec3 modelSize = vec3(1.0f);
		std::string name;
	};

	struct px_plane_collider_component : px_physics_component_base
	{
		px_plane_collider_component(uint32 handle, const vec3& pos, const vec3& norm = vec3(0.f, 1.f, 0.f)) : px_physics_component_base(handle), position(pos), normal(norm)
		{
			createGeometry();
		};

		~px_plane_collider_component() {};

		bool createGeometry();

		virtual void release(bool release = true) override { PX_RELEASE(plane) PX_RELEASE(material) }

		vec3 position{};
		vec3 normal{};

	private:
		PxRigidStatic* plane = nullptr;
		PxMaterial* material = nullptr;
	};
}

#include "core/reflect.h"

namespace era_engine
{
	REFLECT_STRUCT(physics::px_box_collider_component,
		(height, "Height"),
		(length, "Length"),
		(width, "Width")
	);

	REFLECT_STRUCT(physics::px_sphere_collider_component,
		(radius, "Radius")
	);

	REFLECT_STRUCT(physics::px_capsule_collider_component,
		(height, "Height"),
		(radius, "Radius")
	);

	REFLECT_STRUCT(physics::px_bounding_box_collider_component,
		(modelSize, "Size"),
		(name, "Name")
	);

	REFLECT_STRUCT(physics::px_triangle_mesh_collider_component,
		(modelSize, "Size"),
		(name, "Name")
	);

	REFLECT_STRUCT(physics::px_convex_mesh_collider_component,
		(modelSize, "Size"),
		(name, "Name")
	);
}