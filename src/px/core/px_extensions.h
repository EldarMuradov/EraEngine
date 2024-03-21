﻿#pragma once
#include <core/math.h>
#include <core/math_simd.h>

namespace physics
{
	// Computation by Direct Parameterization of Triangles
	inline void computeIntegralTerm(float w0, float w1, float w2, float& f1, float& f2, float& f3, float& g0, float& g1, float& g2) noexcept
	{
		float temp0 = w0 + w1;
		f1 = temp0 + w2;

		float temp1 = w0 * w0;

		float temp2 = temp1 + w1 * temp0;

		f2 = temp2 + w2 * f1;
		f3 = w0 * temp1 + w1 * temp2 + w2 * f2;

		g0 = f2 + w0 * (f1 + w0);
		g1 = f2 + w1 * (f1 + w1);
		g2 = f2 + w2 * (f1 + w2);
	}

	inline void computeCenterMassAndInertia(physx::PxVec3* points, int tmax, physx::PxU32* index,
		float mass, physx::PxVec3& cm, mat3& inertia) noexcept
	{
		// Order: 1, x, y, z, xˆ2, yˆ2, zˆ2, xy, yz, zx
		float integral[10] = { 0,0,0,0,0,0,0,0,0,0 };

		for (size_t t = 0; t < tmax; t++)
		{
			// Get vertices of triangle t
			int i0 = index[3 * t];
			int i1 = index[3 * t + 1];
			int i2 = index[3 * t + 2];

			float x0 = points[i0].x;
			float y0 = points[i0].y;
			float z0 = points[i0].z;

			float x1 = points[i1].x;
			float y1 = points[i1].y;
			float z1 = points[i1].z;

			float x2 = points[i2].x;
			float y2 = points[i2].y;
			float z2 = points[i2].z;

			// Get edges and cross product of edges
			float a1 = x1 - x0;
			float b1 = y1 - y0;
			float c1 = z1 - z0;

			float a2 = x2 - x0;
			float b2 = y2 - y0;
			float c2 = z2 - z0;

			float d0 = b1 * c2 - b2 * c1;
			float d1 = a2 * c1 - a1 * c2;
			float d2 = a1 * b2 - a2 * b1;

			float f1x, f2x, f3x;
			float f1y, f2y, f3y;
			float f1z, f2z, f3z;

			float g0x, g1x, g2x;
			float g0y, g1y, g2y;
			float g0z, g1z, g2z;

			computeIntegralTerm(x0, x1, x2, f1x, f2x, f3x, g0x, g1x, g2x);
			computeIntegralTerm(y0, y1, y2, f1y, f2y, f3y, g0y, g1y, g2y);
			computeIntegralTerm(z0, z1, z2, f1z, f2z, f3z, g0z, g1z, g2z);

			// Update integrals
			integral[0] += d0 * f1x;
			integral[1] += d0 * f2x;
			integral[2] += d1 * f2y;
			integral[3] += d2 * f2z;
			integral[4] += d0 * f3x;
			integral[5] += d1 * f3y;
			integral[6] += d2 * f3z;
			integral[7] += d0 * (y0 * g0x + y1 * g1x + y2 * g2x);
			integral[8] += d1 * (z0 * g0y + z1 * g1y + z2 * g2y);
			integral[9] += d2 * (x0 * g0z + x1 * g1z + x2 * g2z);
		}

		integral[0] *= oneDiv6;
		integral[1] *= oneDiv24;
		integral[2] *= oneDiv24;
		integral[3] *= oneDiv24;
		integral[4] *= oneDiv60;
		integral[5] *= oneDiv60;
		integral[6] *= oneDiv60;
		integral[7] *= oneDiv120;
		integral[8] *= oneDiv120;
		integral[9] *= oneDiv120;

		mass = integral[0];

		// Center of mass
		cm.x = integral[1] / mass;
		cm.y = integral[2] / mass;
		cm.z = integral[3] / mass;

		// Inertia relative to world origin
		inertia.m00 = integral[5] + integral[6];
		inertia.m11 = integral[4] + integral[6];
		inertia.m22 = integral[4] + integral[5];
		inertia.m01 = -integral[7];
		inertia.m12 = -integral[8];
		inertia.m02 = -integral[9];

		// Inertia relative to center of mass
		inertia.m00 -= mass * (cm.y * cm.y + cm.z * cm.z);
		inertia.m11 -= mass * (cm.z * cm.z + cm.x * cm.x);
		inertia.m22 -= mass * (cm.x * cm.x + cm.y * cm.y);
		inertia.m01 += mass * cm.x * cm.y;
		inertia.m12 += mass * cm.y * cm.z;
		inertia.m02 += mass * cm.z * cm.x;
	}
}