// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math_simd.h"

#ifndef BOUNDING_VOLUME_SIMD

#define BOUNDING_VOLUME_SIMD

#include "core/math.h"

namespace era_engine
{
	typedef w8_float w_float;
	typedef w8_int w_int;

	typedef wN_vec2<w_float> w_vec2;
	typedef wN_vec3<w_float> w_vec3;
	typedef wN_vec4<w_float> w_vec4;
	typedef wN_quat<w_float> w_quat;
	typedef wN_mat2<w_float> w_mat2;
	typedef wN_mat3<w_float> w_mat3;
}

#endif

namespace era_engine
{
	template <typename simd_t>
	struct ERA_CORE_API wN_bounding_sphere
	{
		wN_vec3<simd_t> center;
		simd_t radius;
	};

	template <typename simd_t>
	struct ERA_CORE_API wN_bounding_capsule
	{
		wN_vec3<simd_t> positionA;
		wN_vec3<simd_t> positionB;
		simd_t radius;
	};

	template <typename simd_t>
	struct ERA_CORE_API wN_bounding_cylinder
	{
		wN_vec3<simd_t> positionA;
		wN_vec3<simd_t> positionB;
		simd_t radius;
	};

	template <typename simd_t>
	struct ERA_CORE_API wN_bounding_box
	{
		static wN_bounding_box fromMinMax(wN_vec3<simd_t> minCorner, wN_vec3<simd_t> maxCorner)
		{
			return wN_bounding_box{ minCorner, maxCorner };
		}

		static wN_bounding_box fromCenterRadius(wN_vec3<simd_t> center, wN_vec3<simd_t> radius)
		{
			return wN_bounding_box{ center - radius, center + radius };
		}

		wN_vec3<simd_t> minCorner;
		wN_vec3<simd_t> maxCorner;
	};

	template <typename simd_t>
	struct ERA_CORE_API wN_bounding_oriented_box
	{
		wN_vec3<simd_t> center;
		wN_vec3<simd_t> radius;
		wN_quat<simd_t> rotation;
	};

	template <typename simd_t>
	struct ERA_CORE_API wN_line_segment
	{
		wN_vec3<simd_t> a, b;
	};

	template <typename simd_t>
	inline auto aabbVsAABB(const wN_bounding_box<simd_t>& a, const wN_bounding_box<simd_t>& b)
	{
		return
			(a.maxCorner.x >= b.minCorner.x) & (a.minCorner.x <= b.maxCorner.x) &
			(a.maxCorner.y >= b.minCorner.y) & (a.minCorner.y <= b.maxCorner.y) &
			(a.maxCorner.z >= b.minCorner.z) & (a.minCorner.z <= b.maxCorner.z);
	}

	template <typename simd_t>
	inline NODISCARD wN_vec3<simd_t> closestPoint_PointSegment(const wN_vec3<simd_t>& q, const wN_line_segment<simd_t>& l)
	{
		wN_vec3<simd_t> ab = l.b - l.a;
		simd_t t = dot(q - l.a, ab) / squared_length(ab);
		t = clamp01(t);
		return l.a + t * ab;
	}

	template <typename simd_t>
	inline wN_vec3<simd_t> closestPoint_PointAABB(const wN_vec3<simd_t>& q, const wN_bounding_box<simd_t>& aabb)
	{
		wN_vec3<simd_t> result;
		for (uint32 i = 0; i < 3; ++i)
		{
			simd_t v = q.data[i];
			v = if_then(v < aabb.minCorner.data[i], aabb.minCorner.data[i], v);
			v = if_then(v > aabb.maxCorner.data[i], aabb.maxCorner.data[i], v);
			result.data[i] = v;
		}
		return result;
	}

	template <typename simd_t>
	inline simd_t closestPoint_SegmentSegment(const wN_line_segment<simd_t>& l1, const wN_line_segment<simd_t>& l2, wN_vec3<simd_t>& c1, wN_vec3<simd_t>& c2)
	{
		w_vec3 d1 = l1.b - l1.a;
		w_vec3 d2 = l2.b - l2.a;
		w_vec3 r = l1.a - l2.a;
		w_float a = dot(d1, d1);
		w_float e = dot(d2, d2);
		w_float f = dot(d2, r);

		w_float s, t;

		w_float c = dot(d1, r);

		w_float b = dot(d1, d2);
		w_float denom = a * e - b * b;

		s = if_then(denom != 0.f, clamp01((b * f - c * e) / denom), 0.f);

		t = (b * s + f) / e;

		s = if_then(t < 0.f, clamp01(-c / a), if_then(t > 1.f, clamp01((b - c) / a), s));
		t = clamp01(t);

		auto eSmall = e <= EPSILON;
		auto aSmall = a <= EPSILON;

		s = if_then(eSmall, clamp01(-c / a), s);
		t = if_then(eSmall, 0.f, t);

		s = if_then(aSmall, 0.f, s);
		t = if_then(aSmall, clamp01(f / e), t);

		s = if_then(aSmall & eSmall, 0.f, s);
		t = if_then(aSmall & eSmall, 0.f, t);

		c1 = l1.a + d1 * s;
		c2 = l2.a + d2 * t;

		return squared_length(c1 - c2);
	}

}