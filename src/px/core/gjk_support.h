#pragma once
#include <extensions/PxGjkQueryExt.h>

namespace physics
{
	using namespace physx;

    struct px_sphere_support : PxGjkQuery::Support
    {
        virtual PxReal getMargin() const { return radius; }

        virtual PxVec3 supportLocal(const PxVec3& dir) const
        {
            return PxVec3(0, 0, 0);
        }

        PxReal radius;
    };

    struct px_capsule_support : PxGjkQuery::Support
    {
        virtual PxReal getMargin() const { return radius; }

        virtual PxVec3 supportLocal(const PxVec3& dir) const
        {
            return PxVec3((dir.x > 0 ? halfHeight : -halfHeight), 0, 0);
        }

        PxReal radius, halfHeight;
    };
}