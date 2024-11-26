#pragma once

#include "physics/physx_api.h"

#include "ecs/component.h"

namespace era_engine
{
	struct mesh_asset;
}

namespace era_engine::physics
{

	template<typename T_>
	using IsShapeType = std::enable_if_t<std::is_base_of_v<class ShapeComponent, T_>, bool>;

	class ShapeComponent : public Component
	{
	public:
		ShapeComponent() = default;
		ShapeComponent(ref<Entity::EcsData> _data);
		virtual ~ShapeComponent();

		template<class T>
		T* is()
		{
			if constexpr (std::is_base_of_v<ShapeComponent, T>)
			{
				return static_cast<T*>(this);
			}
			return nullptr;
		}

		physx::PxShape* get_shape() const;
		const physx::PxGeometry& get_geometry() const;

		virtual void release() override;

		ERA_VIRTUAL_REFLECT(Component)

	protected:
		void register_shape();

		virtual physx::PxShape* create_shape();

	protected:
		physx::PxShape* shape = nullptr;

		friend class DynamicBodyComponent;
		friend class StaticBodyComponent;
	};

	class BoxShapeComponent final : public ShapeComponent
	{
	public:
		BoxShapeComponent() = default;
		BoxShapeComponent(ref<Entity::EcsData> _data, const vec3& _extents);
		~BoxShapeComponent() override;

		ERA_VIRTUAL_REFLECT(ShapeComponent)

	protected:
		physx::PxShape* create_shape() override;

	private:
		vec3 extents = vec3(1.0f);
	};

	class SphereShapeComponent final : public ShapeComponent
	{
	public:
		SphereShapeComponent() = default;
		SphereShapeComponent(ref<Entity::EcsData> _data, const float _radius);
		~SphereShapeComponent() override;

		ERA_VIRTUAL_REFLECT(ShapeComponent)

	protected:
		physx::PxShape* create_shape() override;

	private:
		float radius = 1.0f;
	};

	class CapsuleShapeComponent final : public ShapeComponent
	{
	public:
		CapsuleShapeComponent() = default;
		CapsuleShapeComponent(ref<Entity::EcsData> _data, const float _radius, const float _half_height);
		~CapsuleShapeComponent() override;

		ERA_VIRTUAL_REFLECT(ShapeComponent)

	protected:
		physx::PxShape* create_shape() override;

	private:
		float radius = 1.0f;
		float half_height = 1.0f;
	};

	class TriangleMeshShapeComponent final : public ShapeComponent
	{
	public:
		TriangleMeshShapeComponent() = default;
		TriangleMeshShapeComponent(ref<Entity::EcsData> _data, ref<mesh_asset> _asset, const vec3& _size);
		~TriangleMeshShapeComponent() override;

		ERA_VIRTUAL_REFLECT(ShapeComponent)

	protected:
		physx::PxShape* create_shape() override;

	private:
		ref<mesh_asset> asset = nullptr;
		vec3 size = vec3(1.0f);
	};

	class ConvexMeshShapeComponent final : public ShapeComponent
	{
	public:
		ConvexMeshShapeComponent() = default;
		ConvexMeshShapeComponent(ref<Entity::EcsData> _data, ref<mesh_asset> _asset, const vec3& _size);
		~ConvexMeshShapeComponent() override;

		ERA_VIRTUAL_REFLECT(ShapeComponent)

	protected:
		physx::PxShape* create_shape() override;

	private:
		ref<mesh_asset> asset = nullptr;
		vec3 size = vec3(1.0f);
	};
}