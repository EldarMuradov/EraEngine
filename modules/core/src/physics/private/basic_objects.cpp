#include "physics/basic_objects.h"
#include "physics/core/physics.h"
#include "physics/shape_utils.h"

#include <rttr/registration>

namespace era_engine::physics
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<PlaneComponent>("PlaneComponent")
			.constructor<>();
	}

	PlaneComponent::PlaneComponent(ref<Entity::EcsData> _data, const vec3& _point, const vec3& _norm)
		: Component(_data), point(_point), normal(_norm)
	{
		using namespace physx;

		auto& physics = PhysicsHolder::physics_ref;

		plane = PxCreatePlane(*physics->get_physics(), PxPlane(createPxVec3(point), createPxVec3(normal)), *physics->get_default_material());
		
		PxShape* buffer[1];
		plane->getShapes(buffer, 1);
		ShapeUtils::setup_filtering(buffer[0], -1, -1);

		PxSceneWriteLock lock{ *physics->get_scene() };
		physics->get_scene()->addActor(*plane);
	}

	PlaneComponent::~PlaneComponent()
	{
	}

	void PlaneComponent::release()
	{
		using namespace physx;

		auto& physics = PhysicsHolder::physics_ref;
		PxSceneWriteLock lock{ *physics->get_scene() };
		physics->get_scene()->removeActor(*plane);

		PX_RELEASE(plane)
	}

}