// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include <core/math.h>

#include <extensions/PxGjkQueryExt.h>

namespace era_engine::physics
{
	using namespace physx;

    using px_gjk_support = PxGjkQuery::Support;
    using px_sphere_support = PxGjkQueryExt::SphereSupport;
    using px_box_support = PxGjkQueryExt::BoxSupport;
    using px_capsule_support = PxGjkQueryExt::CapsuleSupport;
    using px_convex_support = PxGjkQueryExt::ConvexMeshSupport;

    struct px_gjk_query
    {
        bool overlapSphere(const vec3& center1, float radius1, const vec3& center2, float radius2)
        {
            px_sphere_support support1, support2;

            support1.radius = radius1;
            support2.radius = radius2;

            return overlapImpl(support1, support2, PxTransform(createPxVec3(center1)), PxTransform(createPxVec3(center2)));
        }

        bool overlapBox(const vec3& center1, const vec3& halfExtents1, const vec3& center2, const vec3& halfExtents2)
        {
            px_box_support support1, support2;

            support1.halfExtents = createPxVec3(halfExtents1);
            support2.halfExtents = createPxVec3(halfExtents2);

            return overlapImpl(support1, support2, PxTransform(createPxVec3(center1)), PxTransform(createPxVec3(center2)));
        }

        bool overlapCapsule(const vec3& center1, float halfHeight1, float radius1, 
            const vec3& center2, float halfHeight2, float radius2)
        {
            px_capsule_support support1, support2;

            support1.halfHeight = halfHeight1;
            support2.halfHeight = halfHeight2;
            support1.radius = radius1;
            support2.radius = radius2;

            return overlapImpl(support1, support2, PxTransform(createPxVec3(center1)), PxTransform(createPxVec3(center2)));
        }

        bool raycastShape(const px_gjk_support& supportShape, const vec3& shapePose, 
            const vec3& rayStart, const vec3& direction, float maxDist, float& hitDist)
        {
            PxVec3 normal, point;
            return raycastImpl(supportShape, PxTransform(createPxVec3(shapePose)), 
                createPxVec3(rayStart), createPxVec3(direction), maxDist, hitDist, normal, point);
        }

    private:
        bool overlapImpl(const PxGjkQuery::Support& a, const PxGjkQuery::Support& b,
            const PxTransform& poseA, const PxTransform& poseB)
        {
            return PxGjkQuery::overlap(a, b, poseA, poseB);
        }

        bool sweepImpl(const PxGjkQuery::Support& a, const PxGjkQuery::Support& b,
            const PxTransform& poseA, const PxTransform& poseB,
            const PxVec3& unitDir, PxReal maxDist,
            PxReal& hitDist, PxVec3& n, PxVec3& p)
        {
            return PxGjkQuery::sweep(a, b, poseA, poseB, unitDir, maxDist, hitDist, n, p);
        }

        bool raycastImpl(const PxGjkQuery::Support& shape, const PxTransform& pose,
            const PxVec3& rayStart, const PxVec3& unitDir, PxReal maxDist,
            PxReal& hitDist, PxVec3& n, PxVec3& p)
        {
            return PxGjkQuery::raycast(shape, pose, rayStart, unitDir, maxDist, hitDist, n, p);
        }

        bool proximityInfoImpl(const PxGjkQuery::Support& a, const PxGjkQuery::Support& b,
            const PxTransform& poseA, const PxTransform& poseB,
            PxReal contactDistance, PxReal toleranceLength,
            PxVec3& pointA, PxVec3& pointB, PxVec3& separatingAxis,
            PxReal& separation)
        {
            return PxGjkQuery::proximityInfo(a, b, poseA, poseB, contactDistance, toleranceLength, 
                pointA, pointB, separatingAxis, separation);
        }

        bool generateContactsImpl(
            const PxGjkQuery::Support& a, const PxGjkQuery::Support& b,
            const PxTransform& poseA, const PxTransform& poseB,
            PxReal contactDistance, PxReal toleranceLength,
            PxContactBuffer& contactBuffer)
        {
            return PxGjkQueryExt::generateContacts(a, b, poseA, poseB,
                contactDistance, toleranceLength, contactBuffer);
        }
    };
}