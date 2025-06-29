// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "core/math.h"

#include <half/half.c>

const half half::min_value = (uint16)0b1111101111111111;
const half half::max_value = (uint16)0b0111101111111111;

const mat2 mat2::identity =
{
	1.f, 0.f,
	0.f, 1.f,
};

const mat2 mat2::zero =
{
	0.f, 0.f,
	0.f, 0.f,
};

const mat3 mat3::identity =
{
	1.f, 0.f, 0.f,
	0.f, 1.f, 0.f,
	0.f, 0.f, 1.f,
};

const mat3 mat3::zero =
{
	0.f, 0.f, 0.f,
	0.f, 0.f, 0.f,
	0.f, 0.f, 0.f,
};

const mat4 mat4::identity =
{
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f,
};

const mat4 mat4::zero =
{
	0.f, 0.f, 0.f, 0.f,
	0.f, 0.f, 0.f, 0.f,
	0.f, 0.f, 0.f, 0.f,
	0.f, 0.f, 0.f, 0.f,
};

const vec2 vec2::zero = { 0.f, 0.f };
const vec3 vec3::zero = { 0.f, 0.f, 0.f };
const vec4 vec4::zero = { 0.f, 0.f, 0.f, 0.f };

const quat quat::identity = { 0.f, 0.f, 0.f, 1.f };
const quat quat::zero = { 0.f, 0.f, 0.f, 0.f };

const dual_quat dual_quat::identity = { { 0.f, 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f, 0.f } };
const dual_quat dual_quat::zero = { { 0.f, 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 0.f } };

const trs trs::identity = { vec3(0.f, 0.f, 0.f), quat(0.f, 0.f, 0.f, 1.f), vec3(1.f, 1.f, 1.f) };

half::half(float f)
{
	h = half_from_float(*(uint32*)&f);
}

half::half(uint16 i)
{
	h = i;
}

half::operator float()
{
	uint32 f = half_to_float(h);
	return *(float*)&f;
}

half operator+(half a, half b) { half result; result.h = half_add(a.h, b.h); return result; }
half& operator+=(half& a, half b) { a = a + b; return a; }
half operator-(half a, half b) { half result; result.h = half_sub(a.h, b.h); return result; }
half& operator-=(half& a, half b) { a = a - b; return a; }
half operator*(half a, half b) { half result; result.h = half_mul(a.h, b.h); return result; }
half& operator*=(half& a, half b) { a = a * b; return a; }
half operator/(half a, half b) { half result; result.h = half_div(a.h, b.h); return result; }
half& operator/=(half& a, half b) { a = a / b; return a; }

mat2 operator*(const mat2& a, const mat2& b)
{
	vec2 r0 = row(a, 0);
	vec2 r1 = row(a, 1);

	vec2 c0 = col(b, 0);
	vec2 c1 = col(b, 1);

	mat2 result;
	result.m00 = dot(r0, c0); result.m01 = dot(r0, c1);
	result.m10 = dot(r1, c0); result.m11 = dot(r1, c1);
	return result;
}

mat3 operator*(const mat3& a, const mat3& b)
{
	vec3 r0 = row(a, 0);
	vec3 r1 = row(a, 1);
	vec3 r2 = row(a, 2);

	vec3 c0 = col(b, 0);
	vec3 c1 = col(b, 1);
	vec3 c2 = col(b, 2);

	mat3 result;
	result.m00 = dot(r0, c0); result.m01 = dot(r0, c1); result.m02 = dot(r0, c2);
	result.m10 = dot(r1, c0); result.m11 = dot(r1, c1); result.m12 = dot(r1, c2);
	result.m20 = dot(r2, c0); result.m21 = dot(r2, c1); result.m22 = dot(r2, c2);
	return result;
}

mat3 operator+(const mat3& a, const mat3& b)
{
	mat3 result;
	for (uint32 i = 0; i < 9; ++i)
	{
		result.m[i] = a.m[i] + b.m[i];
	}
	return result;
}

mat3& operator+=(mat3& a, const mat3& b)
{
	for (uint32 i = 0; i < 9; ++i)
	{
		a.m[i] += b.m[i];
	}
	return a;
}

mat3 operator-(const mat3& a, const mat3& b)
{
	mat3 result;
	for (uint32 i = 0; i < 9; ++i)
	{
		result.m[i] = a.m[i] - b.m[i];
	}
	return result;
}

mat4 operator*(const mat4& a, const mat4& b)
{
#ifndef SIMD_AVX_2
	mat4 result;
	vec4 r0 = row(a, 0);
	vec4 r1 = row(a, 1);
	vec4 r2 = row(a, 2);
	vec4 r3 = row(a, 3);

	vec4 c0 = col(b, 0);
	vec4 c1 = col(b, 1);
	vec4 c2 = col(b, 2);
	vec4 c3 = col(b, 3);

	result.m00 = dot(r0, c0); result.m01 = dot(r0, c1); result.m02 = dot(r0, c2); result.m03 = dot(r0, c3);
	result.m10 = dot(r1, c0); result.m11 = dot(r1, c1); result.m12 = dot(r1, c2); result.m13 = dot(r1, c3);
	result.m20 = dot(r2, c0); result.m21 = dot(r2, c1); result.m22 = dot(r2, c2); result.m23 = dot(r2, c3);
	result.m30 = dot(r3, c0); result.m31 = dot(r3, c1); result.m32 = dot(r3, c2); result.m33 = dot(r3, c3);
	return result;
#else
	w8_float a0, a1, b0, b1;

#if ROW_MAJOR
	w8_float u0 = b.m;
	w8_float u1 = b.m + 8;
	w8_float t0 = a.m;
	w8_float t1 = a.m + 8;
#else
	w8_float t0 = b.m;
	w8_float t1 = b.m + 8;
	w8_float u0 = a.m;
	w8_float u1 = a.m + 8;
#endif

	a0 = _mm256_shuffle_ps(t0, t0, _MM_SHUFFLE(0, 0, 0, 0));
	a1 = _mm256_shuffle_ps(t1, t1, _MM_SHUFFLE(0, 0, 0, 0));
	b0 = _mm256_permute2f128_ps(u0, u0, 0x00);
	w8_float c0 = a0 * b0;
	w8_float c1 = a1 * b0;

	a0 = _mm256_shuffle_ps(t0, t0, _MM_SHUFFLE(1, 1, 1, 1));
	a1 = _mm256_shuffle_ps(t1, t1, _MM_SHUFFLE(1, 1, 1, 1));
	b0 = _mm256_permute2f128_ps(u0, u0, 0x11);
	c0 = fmadd(a0, b0, c0);
	c1 = fmadd(a1, b0, c1);

	a0 = _mm256_shuffle_ps(t0, t0, _MM_SHUFFLE(2, 2, 2, 2));
	a1 = _mm256_shuffle_ps(t1, t1, _MM_SHUFFLE(2, 2, 2, 2));
	b1 = _mm256_permute2f128_ps(u1, u1, 0x00);
	c0 = fmadd(a0, b1, c0);
	c1 = fmadd(a1, b1, c1);

	a0 = _mm256_shuffle_ps(t0, t0, _MM_SHUFFLE(3, 3, 3, 3));
	a1 = _mm256_shuffle_ps(t1, t1, _MM_SHUFFLE(3, 3, 3, 3));
	b1 = _mm256_permute2f128_ps(u1, u1, 0x11);
	c0 = fmadd(a0, b1, c0);
	c1 = fmadd(a1, b1, c1);

	mat4 result;
	c0.store(result.m);
	c1.store(result.m + 8);
	return result;
#endif
}

mat2 operator*(const mat2& a, float b) { mat2 result; for (uint32 i = 0; i < 4; ++i) { result.m[i] = a.m[i] * b; } return result; }
mat3 operator*(const mat3& a, float b) { mat3 result; for (uint32 i = 0; i < 9; ++i) { result.m[i] = a.m[i] * b; } return result; }

#if ROW_MAJOR
mat4 operator*(const mat4& a, float b) { mat4 result; for (uint32 i = 0; i < 4; ++i) { result.rows[i] = a.rows[i] * b; } return result; }
#else
mat4 operator*(const mat4& a, float b) { mat4 result; for (uint32 i = 0; i < 4; ++i) { result.cols[i] = a.cols[i] * b; } return result; }
#endif

mat2 operator*(float a, const mat2& b) { return b * a; }
mat3 operator*(float a, const mat3& b) { return b * a; }
mat4 operator*(float a, const mat4& b) { return b * a; }

mat2& operator*=(mat2& a, float b) { a = a * b; return a; }
mat3& operator*=(mat3& a, float b) { a = a * b; return a; }
mat4& operator*=(mat4& a, float b) { a = a * b; return a; }

mat2 transpose(const mat2& a)
{
	mat2 result;
	result.m00 = a.m00; result.m01 = a.m10;
	result.m10 = a.m01; result.m11 = a.m11;
	return result;
}

mat3 transpose(const mat3& a)
{
	mat3 result;
	result.m00 = a.m00; result.m01 = a.m10; result.m02 = a.m20;
	result.m10 = a.m01; result.m11 = a.m11; result.m12 = a.m21;
	result.m20 = a.m02; result.m21 = a.m12; result.m22 = a.m22;
	return result;
}

mat4 transpose(const mat4& a)
{
	mat4 result = a;
	transpose(result.f40, result.f41, result.f42, result.f43);
	return result;
}

mat2 invert(const mat2& m)
{
	float det = determinant(m);
	if (det == 0.f)
	{
		return mat2();
	}

	mat2 inv;
	inv.m00 = m.m11;
	inv.m01 = -m.m01;
	inv.m10 = -m.m10;
	inv.m11 = m.m00;

	inv *= 1.f / det;

	return inv;
}

mat3 invert(const mat3& m)
{
	mat3 inv;

	inv.m00 = m.m11 * m.m22 - m.m21 * m.m12;
	inv.m01 = m.m02 * m.m21 - m.m22 * m.m01;
	inv.m02 = m.m01 * m.m12 - m.m11 * m.m02;

	inv.m10 = m.m12 * m.m20 - m.m22 * m.m10;
	inv.m11 = m.m00 * m.m22 - m.m20 * m.m02;
	inv.m12 = m.m02 * m.m10 - m.m12 * m.m00;

	inv.m20 = m.m10 * m.m21 - m.m20 * m.m11;
	inv.m21 = m.m01 * m.m20 - m.m21 * m.m00;
	inv.m22 = m.m00 * m.m11 - m.m10 * m.m01;

	float det = m.m00 * (m.m11 * m.m22 - m.m21 * m.m12)
		- m.m01 * (m.m10 * m.m22 - m.m20 * m.m12)
		+ m.m02 * (m.m10 * m.m21 - m.m20 * m.m11);

	if (det == 0.f)
	{
		return mat3();
	}

	det = 1.f / det;

	inv *= det;

	return inv;
}

mat4 invert(const mat4& m)
{
	mat4 inv;

	inv.m00 = m.m11 * m.m22 * m.m33 -
		m.m11 * m.m32 * m.m23 -
		m.m12 * m.m21 * m.m33 +
		m.m12 * m.m31 * m.m23 +
		m.m13 * m.m21 * m.m32 -
		m.m13 * m.m31 * m.m22;

	inv.m01 = -m.m01 * m.m22 * m.m33 +
		m.m01 * m.m32 * m.m23 +
		m.m02 * m.m21 * m.m33 -
		m.m02 * m.m31 * m.m23 -
		m.m03 * m.m21 * m.m32 +
		m.m03 * m.m31 * m.m22;

	inv.m02 = m.m01 * m.m12 * m.m33 -
		m.m01 * m.m32 * m.m13 -
		m.m02 * m.m11 * m.m33 +
		m.m02 * m.m31 * m.m13 +
		m.m03 * m.m11 * m.m32 -
		m.m03 * m.m31 * m.m12;

	inv.m03 = -m.m01 * m.m12 * m.m23 +
		m.m01 * m.m22 * m.m13 +
		m.m02 * m.m11 * m.m23 -
		m.m02 * m.m21 * m.m13 -
		m.m03 * m.m11 * m.m22 +
		m.m03 * m.m21 * m.m12;

	inv.m10 = -m.m10 * m.m22 * m.m33 +
		m.m10 * m.m32 * m.m23 +
		m.m12 * m.m20 * m.m33 -
		m.m12 * m.m30 * m.m23 -
		m.m13 * m.m20 * m.m32 +
		m.m13 * m.m30 * m.m22;

	inv.m11 = m.m00 * m.m22 * m.m33 -
		m.m00 * m.m32 * m.m23 -
		m.m02 * m.m20 * m.m33 +
		m.m02 * m.m30 * m.m23 +
		m.m03 * m.m20 * m.m32 -
		m.m03 * m.m30 * m.m22;

	inv.m12 = -m.m00 * m.m12 * m.m33 +
		m.m00 * m.m32 * m.m13 +
		m.m02 * m.m10 * m.m33 -
		m.m02 * m.m30 * m.m13 -
		m.m03 * m.m10 * m.m32 +
		m.m03 * m.m30 * m.m12;

	inv.m13 = m.m00 * m.m12 * m.m23 -
		m.m00 * m.m22 * m.m13 -
		m.m02 * m.m10 * m.m23 +
		m.m02 * m.m20 * m.m13 +
		m.m03 * m.m10 * m.m22 -
		m.m03 * m.m20 * m.m12;

	inv.m20 = m.m10 * m.m21 * m.m33 -
		m.m10 * m.m31 * m.m23 -
		m.m11 * m.m20 * m.m33 +
		m.m11 * m.m30 * m.m23 +
		m.m13 * m.m20 * m.m31 -
		m.m13 * m.m30 * m.m21;

	inv.m21 = -m.m00 * m.m21 * m.m33 +
		m.m00 * m.m31 * m.m23 +
		m.m01 * m.m20 * m.m33 -
		m.m01 * m.m30 * m.m23 -
		m.m03 * m.m20 * m.m31 +
		m.m03 * m.m30 * m.m21;

	inv.m22 = m.m00 * m.m11 * m.m33 -
		m.m00 * m.m31 * m.m13 -
		m.m01 * m.m10 * m.m33 +
		m.m01 * m.m30 * m.m13 +
		m.m03 * m.m10 * m.m31 -
		m.m03 * m.m30 * m.m11;

	inv.m23 = -m.m00 * m.m11 * m.m23 +
		m.m00 * m.m21 * m.m13 +
		m.m01 * m.m10 * m.m23 -
		m.m01 * m.m20 * m.m13 -
		m.m03 * m.m10 * m.m21 +
		m.m03 * m.m20 * m.m11;

	inv.m30 = -m.m10 * m.m21 * m.m32 +
		m.m10 * m.m31 * m.m22 +
		m.m11 * m.m20 * m.m32 -
		m.m11 * m.m30 * m.m22 -
		m.m12 * m.m20 * m.m31 +
		m.m12 * m.m30 * m.m21;

	inv.m31 = m.m00 * m.m21 * m.m32 -
		m.m00 * m.m31 * m.m22 -
		m.m01 * m.m20 * m.m32 +
		m.m01 * m.m30 * m.m22 +
		m.m02 * m.m20 * m.m31 -
		m.m02 * m.m30 * m.m21;

	inv.m32 = -m.m00 * m.m11 * m.m32 +
		m.m00 * m.m31 * m.m12 +
		m.m01 * m.m10 * m.m32 -
		m.m01 * m.m30 * m.m12 -
		m.m02 * m.m10 * m.m31 +
		m.m02 * m.m30 * m.m11;

	inv.m33 = m.m00 * m.m11 * m.m22 -
		m.m00 * m.m21 * m.m12 -
		m.m01 * m.m10 * m.m22 +
		m.m01 * m.m20 * m.m12 +
		m.m02 * m.m10 * m.m21 -
		m.m02 * m.m20 * m.m11;

	float det = m.m00 * inv.m00 + m.m10 * inv.m01 + m.m20 * inv.m02 + m.m30 * inv.m03;

	if (det == 0.f)
	{
		return mat4();
	}

	det = 1.f / det;

	inv *= det;

	return inv;
}

float determinant(const mat2& m)
{
	return m.m00 * m.m11 - m.m10 * m.m01;
}

float determinant(const mat3& m)
{
	return m.m00 * (m.m11 * m.m22 - m.m21 * m.m12)
		- m.m01 * (m.m10 * m.m22 - m.m20 * m.m12)
		+ m.m02 * (m.m10 * m.m21 - m.m20 * m.m11);
}

float determinant(const mat4& m)
{
	return
		m.m03 * m.m12 * m.m21 * m.m30 - m.m02 * m.m13 * m.m21 * m.m30 -
		m.m03 * m.m11 * m.m22 * m.m30 + m.m01 * m.m13 * m.m22 * m.m30 +
		m.m02 * m.m11 * m.m23 * m.m30 - m.m01 * m.m12 * m.m23 * m.m30 -
		m.m03 * m.m12 * m.m20 * m.m31 + m.m02 * m.m13 * m.m20 * m.m31 +
		m.m03 * m.m10 * m.m22 * m.m31 - m.m00 * m.m13 * m.m22 * m.m31 -
		m.m02 * m.m10 * m.m23 * m.m31 + m.m00 * m.m12 * m.m23 * m.m31 +
		m.m03 * m.m11 * m.m20 * m.m32 - m.m01 * m.m13 * m.m20 * m.m32 -
		m.m03 * m.m10 * m.m21 * m.m32 + m.m00 * m.m13 * m.m21 * m.m32 +
		m.m01 * m.m10 * m.m23 * m.m32 - m.m00 * m.m11 * m.m23 * m.m32 -
		m.m02 * m.m11 * m.m20 * m.m33 + m.m01 * m.m12 * m.m20 * m.m33 +
		m.m02 * m.m10 * m.m21 * m.m33 - m.m00 * m.m12 * m.m21 * m.m33 -
		m.m01 * m.m10 * m.m22 * m.m33 + m.m00 * m.m11 * m.m22 * m.m33;
}

float trace(const mat3& m)
{
	return m.m00 + m.m11 + m.m22;
}

float trace(const mat4& m)
{
	return m.m00 + m.m11 + m.m22 + m.m33;
}

trs invert(const trs& t)
{
	quat inv_rotation = conjugate(t.rotation);
	vec3 inv_scale = 1.f / t.scale;
	vec3 inv_translation = inv_rotation * (inv_scale * -t.position);

	return trs(inv_translation, inv_rotation, inv_scale);
}

vec3 transform_position(const mat4& m, vec3 pos)
{
	return (m * vec4(pos, 1.f)).xyz;
}

vec3 transform_direction(const mat4& m, vec3 dir)
{
	return (m * vec4(dir, 0.f)).xyz;
}

vec3 transform_position(const trs& m, vec3 pos)
{
	return m.rotation * (m.scale * pos) + m.position;
}

vec3 transform_direction(const trs& m, vec3 dir)
{
	return m.rotation * dir;
}

vec3 inverse_transform_position(const trs& m, vec3 pos)
{
	return (conjugate(m.rotation) * (pos - m.position)) / m.scale;
}

vec3 inverse_transform_direction(const trs& m, vec3 dir)
{
	return conjugate(m.rotation) * dir;
}

quat rotate_from_to(vec3 _from, vec3 _to)
{
	vec3 from = normalize(_from);
	vec3 to = normalize(_to);

	float d = dot(from, to);
	if (d >= 1.f)
	{
		return quat(0.f, 0.f, 0.f, 1.f);
	}

	quat q;
	if (d < (1e-6f - 1.f))
	{
		// Rotate 180� around some axis
		vec3 axis = cross(vec3(1.f, 0.f, 0.f), from);
		if (squared_length(axis) == 0.f) // Pick another if colinear
		{
			axis = cross(vec3(0.f, 1.f, 0.f), from);
		}
		axis = normalize(axis);
		q = normalize(quat(axis, M_PI));
	}
	else
	{
		float s = sqrt((1.f + d) * 2.f);
		float invs = 1.f / s;

		vec3 c = cross(from, to);

		q.x = c.x * invs;
		q.y = c.y * invs;
		q.z = c.z * invs;
		q.w = s * 0.5f;
		q = normalize(q);
	}
	return q;
}

void get_axis_rotation(quat q, vec3& axis, float& angle)
{
	float sq_length = squared_length(q.v);
	if (sq_length > 0.f)
	{
		angle = 2.f * acos(q.w);
		float inv_length = 1.f / sqrt(sq_length);
		axis = q.v * inv_length;
	}
	else
	{
		// Angle is 0 (mod 2*pi), so any axis will do.
		angle = 0.f;
		axis = vec3(1.f, 0.f, 0.f);
	}
}

void decompose_quaternion_into_twist_and_swing(quat q, vec3 normalized_twist_axis, quat& twist, quat& swing)
{
	vec3 axis(q.x, q.y, q.z);
	vec3 proj = dot(axis, normalized_twist_axis) * normalized_twist_axis; // This assumes that twistAxis is normalized.
	twist = normalize(quat(proj.x, proj.y, proj.z, q.w));
	swing = q * conjugate(twist);
}

quat slerp(quat from, quat to, float t)
{
	float d = dot(from.v4, to.v4);
	float absDot = d < 0.f ? -d : d;
	float scale0 = 1.f - t;
	float scale1 = t;

	if ((1.f - absDot) > 0.1f)
	{

		float angle = acosf(absDot);
		float inv_sin_theta = 1.f / sinf(angle);
		scale0 = (sinf((1.f - t) * angle) * inv_sin_theta);
		scale1 = (sinf((t * angle)) * inv_sin_theta);
	}

	if (d < 0.f)
	{
		scale1 = -scale1;
	}

	float newX = (scale0 * from.x) + (scale1 * to.x);
	float newY = (scale0 * from.y) + (scale1 * to.y);
	float newZ = (scale0 * from.z) + (scale1 * to.z);
	float newW = (scale0 * from.w) + (scale1 * to.w);

	return normalize(quat(newX, newY, newZ, newW));
}

quat nlerp(quat* qs, float* weights, uint32 count)
{
	vec4 v0 = qs[0].v4;
	vec4 result = v0 * weights[0];

	for (uint32 i = 1; i < count; ++i)
	{
		vec4 v1 = qs[i].v4;
		if (dot(v0, v1) < 0.f) { v1 = -v1; }
		result += v1 * weights[i];
	}

	return { result.f4 };
}

mat3 quaternion_to_mat3(quat q)
{
	if (q.w == 1.f)
	{
		return mat3::identity;
	}

	float qxx = q.x * q.x;
	float qyy = q.y * q.y;
	float qzz = q.z * q.z;
	float qxz = q.x * q.z;
	float qxy = q.x * q.y;
	float qyz = q.y * q.z;
	float qwx = q.w * q.x;
	float qwy = q.w * q.y;
	float qwz = q.w * q.z;

	mat3 result;

	result.m00 = 1.f - 2.f * (qyy + qzz);
	result.m10 = 2.f * (qxy + qwz);
	result.m20 = 2.f * (qxz - qwy);

	result.m01 = 2.f * (qxy - qwz);
	result.m11 = 1.f - 2.f * (qxx + qzz);
	result.m21 = 2.f * (qyz + qwx);

	result.m02 = 2.f * (qxz + qwy);
	result.m12 = 2.f * (qyz - qwx);
	result.m22 = 1.f - 2.f * (qxx + qyy);

	return result;
}

mat4 quaternion_to_mat4(quat q)
{
	if (q.w == 1.f)
	{
		return mat4::identity;
	}

	float qxx = q.x * q.x;
	float qyy = q.y * q.y;
	float qzz = q.z * q.z;
	float qxz = q.x * q.z;
	float qxy = q.x * q.y;
	float qyz = q.y * q.z;
	float qwx = q.w * q.x;
	float qwy = q.w * q.y;
	float qwz = q.w * q.z;

	mat4 result;

	result.m00 = 1.f - 2.f * (qyy + qzz);
	result.m10 = 2.f * (qxy + qwz);
	result.m20 = 2.f * (qxz - qwy);

	result.m01 = 2.f * (qxy - qwz);
	result.m11 = 1.f - 2.f * (qxx + qzz);
	result.m21 = 2.f * (qyz + qwx);

	result.m02 = 2.f * (qxz + qwy);
	result.m12 = 2.f * (qyz - qwx);
	result.m22 = 1.f - 2.f * (qxx + qyy);

	return result;
}

quat mat3_to_quaternion(const mat3& m)
{
#if 1
	float tr = m.m00 + m.m11 + m.m22;

	quat result;
	if (tr > 0.f)
	{
		float s = sqrtf(tr + 1.f) * 2.f; // S=4*qw 
		result.w = 0.25f * s;
		result.x = (m.m21 - m.m12) / s;
		result.y = (m.m02 - m.m20) / s;
		result.z = (m.m10 - m.m01) / s;
	}
	else if ((m.m00 > m.m11) && (m.m00 > m.m22))
	{
		float s = sqrtf(1.f + m.m00 - m.m11 - m.m22) * 2.f; // S=4*qx 
		result.w = (m.m21 - m.m12) / s;
		result.x = 0.25f * s;
		result.y = (m.m01 + m.m10) / s;
		result.z = (m.m02 + m.m20) / s;
	}
	else if (m.m11 > m.m22)
	{
		float s = sqrtf(1.f + m.m11 - m.m00 - m.m22) * 2.f; // S=4*qy
		result.w = (m.m02 - m.m20) / s;
		result.x = (m.m01 + m.m10) / s;
		result.y = 0.25f * s;
		result.z = (m.m12 + m.m21) / s;
	}
	else
	{
		float s = sqrtf(1.f + m.m22 - m.m00 - m.m11) * 2.f; // S=4*qz
		result.w = (m.m10 - m.m01) / s;
		result.x = (m.m02 + m.m20) / s;
		result.y = (m.m12 + m.m21) / s;
		result.z = 0.25f * s;
	}
#else
	quat result;
	result.w = sqrt(1.f + m.m00 + m.m11 + m.m22) * 0.5f;
	float w4 = 1.f / (4.f * result.w);
	result.x = (m.m21 - m.m12) * w4;
	result.y = (m.m02 - m.m20) * w4;
	result.z = (m.m10 - m.m02) * w4;
#endif
	return normalize(result);
}

vec3 quat_to_euler(quat q)
{
	float roll, pitch, yaw;

	// Roll (x-axis rotation)
	float sinr_cosp = 2.f * (q.w * q.x + q.y * q.z);
	float cosr_cosp = 1.f - 2.f * (q.x * q.x + q.y * q.y);
	yaw = atan2(sinr_cosp, cosr_cosp);

	// Pitch (y-axis rotation)
	float sinp = 2.f * (q.w * q.y - q.z * q.x);
	if (abs(sinp) >= 1.f)
	{
		roll = copysign(M_PI / 2, sinp); // Use 90 degrees if out of range
	}
	else
	{
		roll = asin(sinp);
	}

	// Yaw (z-axis rotation)
	float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
	float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
	pitch = atan2(siny_cosp, cosy_cosp);

	return vec3(pitch, yaw, roll);
}

quat euler_to_quat(vec3 euler)
{
	float pitch = euler.x;
	float yaw = euler.y;
	float roll = euler.z;

	// Abbreviations for the various angular functions
	float cy = cos(pitch * 0.5f);
	float sy = sin(pitch * 0.5f);
	float cp = cos(roll * 0.5f);
	float sp = sin(roll * 0.5f);
	float cr = cos(yaw * 0.5f);
	float sr = sin(yaw * 0.5f);

	quat q;
	q.w = cr * cp * cy + sr * sp * sy;
	q.x = sr * cp * cy - cr * sp * sy;
	q.y = cr * sp * cy + sr * cp * sy;
	q.z = cr * cp * sy - sr * sp * cy;

	return q;
}

mat3 outer_product(vec3 a, vec3 b)
{
	vec3 col0 = a * b.x;
	vec3 col1 = a * b.y;
	vec3 col2 = a * b.z;

	mat3 result;
	result.m00 = col0.x;
	result.m10 = col0.y;
	result.m20 = col0.z;
	result.m01 = col1.x;
	result.m11 = col1.y;
	result.m21 = col1.z;
	result.m02 = col2.x;
	result.m12 = col2.y;
	result.m22 = col2.z;
	return result;
}

mat3 get_skew_matrix(vec3 r)
{
	mat3 result;
	result.m00 = 0.f;
	result.m01 = -r.z;
	result.m02 = r.y;
	result.m10 = r.z;
	result.m11 = 0.f;
	result.m12 = -r.x;
	result.m20 = -r.y;
	result.m21 = r.x;
	result.m22 = 0.f;
	return result;
}

mat4 create_perspective_projection_matrix(float fov, float aspect, float near_plane, float far_plane)
{
	mat4 result;

	float a = 1.f / tan(0.5f * fov);

	result.m00 = a / aspect;
	result.m01 = 0.f;
	result.m02 = 0.f;
	result.m03 = 0.f;

	result.m10 = 0.f;
	result.m11 = a;
	result.m12 = 0.f;
	result.m13 = 0.f;

	result.m20 = 0.f;
	result.m21 = 0.f;

	if (far_plane > 0.f)
	{
#if DIRECTX_COORDINATE_SYSTEM
		result.m22 = -far_plane / (far_plane - near_plane);
		result.m23 = result.m22 * near_plane;
#else
		result.m22 = -(far_plane + near_plane) / (far_plane - near_plane);
		result.m23 = -2.f * far_plane * near_plane / (far_plane - near_plane);
#endif
	}
	else
	{
		result.m22 = -1.f;
#if DIRECTX_COORDINATE_SYSTEM
		result.m23 = -near_plane;
#else
		result.m23 = -2.f * near_plane;
#endif
	}

	result.m30 = 0.f;
	result.m31 = 0.f;
	result.m32 = -1.f;
	result.m33 = 0.f;

	return result;
}

mat4 create_perspective_projection_matrix(float width, float height, float fx, float fy, float cx, float cy, float near_plane, float far_plane)
{
	mat4 result;

	result.m00 = 2.f * fx / width;
	result.m01 = 0.f;
	result.m02 = 1.f - 2.f * cx / width;
	result.m03 = 0.f;

	result.m10 = 0.f;
	result.m11 = 2.f * fy / height;
	result.m12 = 2.f * cy / height - 1.f;
	result.m13 = 0.f;

	result.m20 = 0.f;
	result.m21 = 0.f;

	if (far_plane > 0.f)
	{
#if DIRECTX_COORDINATE_SYSTEM
		result.m22 = -far_plane / (far_plane - near_plane);
		result.m23 = result.m22 * near_plane;
#else
		result.m22 = -(far_plane + near_plane) / (far_plane - near_plane);
		result.m23 = -2.f * far_plane * near_plane / (far_plane - near_plane);
#endif
	}
	else
	{
		result.m22 = -1.f;
#if DIRECTX_COORDINATE_SYSTEM
		result.m23 = -near_plane;
#else
		result.m23 = -2.f * near_plane;
#endif
	}

	result.m30 = 0.f;
	result.m31 = 0.f;
	result.m32 = -1.f;
	result.m33 = 0.f;

	return result;
}

mat4 create_perspective_projection_matrix(float r, float l, float t, float b, float near_plane, float far_plane)
{
	mat4 result;

	result.m00 = (2.f * near_plane) / (r - l);
	result.m01 = 0.f;
	result.m02 = (r + l) / (r - l);
	result.m03 = 0.f;

	result.m10 = 0.f;
	result.m11 = (2.f * near_plane) / (t - b);
	result.m12 = (t + b) / (t - b);
	result.m13 = 0.f;

	result.m20 = 0;
	result.m21 = 0;

	if (far_plane > 0.f)
	{
#if DIRECTX_COORDINATE_SYSTEM
		result.m22 = -far_plane / (far_plane - near_plane);
		result.m23 = result.m22 * near_plane;
#else
		result.m22 = -(far_plane + near_plane) / (far_plane - near_plane);
		result.m23 = -2.f * far_plane * near_plane / (far_plane - near_plane);
#endif
	}
	else
	{
		result.m22 = -1.f;
#if DIRECTX_COORDINATE_SYSTEM
		result.m23 = -near_plane;
#else
		result.m23 = -2.f * near_plane;
#endif
	}

	result.m30 = 0.f;
	result.m31 = 0.f;
	result.m32 = -1.f;
	result.m33 = 0.f;

	return result;
}

mat4 create_orthographic_projection_matrix(float r, float l, float t, float b, float near_plane, float far_plane)
{
	mat4 result;

	result.m00 = 2.f / (r - l);
	result.m01 = 0.f;
	result.m02 = 0.f;
	result.m03 = -(r + l) / (r - l);

	result.m10 = 0.f;
	result.m11 = 2.f / (t - b);
	result.m12 = 0.f;
	result.m13 = -(t + b) / (t - b);

	result.m20 = 0.f;
	result.m21 = 0.f;

#if DIRECTX_COORDINATE_SYSTEM
	result.m22 = -1.f / (far_plane - near_plane);
	result.m23 = result.m22 * near_plane;
#else
	result.m22 = -2.f / (far_plane - near_plane);
	result.m23 = -(far_plane + near_plane) / (far_plane - near_plane);
#endif

	result.m30 = 0.f;
	result.m31 = 0.f;
	result.m32 = 0.f;
	result.m33 = 1.f;

	return result;
}

mat4 invert_perspective_projection_matrix(const mat4& m)
{
	mat4 inv;

	inv.m00 = 1.f / m.m00;
	inv.m01 = 0.f;
	inv.m02 = 0.f;
	inv.m03 = m.m02 / m.m00;

	inv.m10 = 0.f;
	inv.m11 = 1.f / m.m11;
	inv.m12 = 0.f;
	inv.m13 = m.m12 / m.m11;

	inv.m20 = 0.f;
	inv.m21 = 0.f;
	inv.m22 = 0.f;
	inv.m23 = -1.f;

	inv.m30 = 0.f;
	inv.m31 = 0.f;
	inv.m32 = 1.f / m.m23;
	inv.m33 = m.m22 / m.m23;

	return inv;
}

mat4 invert_orthographic_projection_matrix(const mat4& m)
{
	mat4 inv;

	inv.m00 = 1.f / m.m00;
	inv.m01 = 0.f;
	inv.m02 = 0.f;
	inv.m03 = -m.m03 / m.m00;

	inv.m10 = 0.f;
	inv.m11 = 1.f / m.m11;
	inv.m12 = 0.f;
	inv.m13 = -m.m13 / m.m11;

	inv.m20 = 0.f;
	inv.m21 = 0.f;
	inv.m22 = 1.f / m.m22;
	inv.m23 = -m.m23 / m.m22;

	inv.m30 = 0.f;
	inv.m31 = 0.f;
	inv.m32 = 0.f;
	inv.m33 = 1.f;

	return inv;
}

mat4 create_translation_matrix(vec3 position)
{
	mat4 result = mat4::identity;
	result.m03 = position.x;
	result.m13 = position.y;
	result.m23 = position.z;
	return result;
}

mat4 create_model_matrix(vec3 position, quat rotation, vec3 scale)
{
	mat4 result;
#if 0
	result.m03 = position.x;
	result.m13 = position.y;
	result.m23 = position.z;
	mat3 rot = quaternion_to_mat3(rotation);
	result.m00 = rot.m00 * scale.x;
	result.m01 = rot.m01 * scale.y;
	result.m02 = rot.m02 * scale.z;
	result.m10 = rot.m10 * scale.x;
	result.m11 = rot.m11 * scale.y;
	result.m12 = rot.m12 * scale.z;
	result.m20 = rot.m20 * scale.x;
	result.m21 = rot.m21 * scale.y;
	result.m22 = rot.m22 * scale.z;
	result.m30 = result.m31 = result.m32 = 0.f;
	result.m33 = 1.f;
#else
	result.m03 = position.x;
	result.m13 = position.y;
	result.m23 = position.z;

	const float x2 = rotation.x + rotation.x;
	const float y2 = rotation.y + rotation.y;
	const float z2 = rotation.z + rotation.z;
	{
		const float xx2 = rotation.x * x2;
		const float yy2 = rotation.y * y2;
		const float zz2 = rotation.z * z2;

		result.m00 = (1.f - (yy2 + zz2)) * scale.x;
		result.m11 = (1.f - (xx2 + zz2)) * scale.y;
		result.m22 = (1.f - (xx2 + yy2)) * scale.z;
	}
	{
		const float yz2 = rotation.y * z2;
		const float wx2 = rotation.w * x2;

		result.m12 = (yz2 - wx2) * scale.z;
		result.m21 = (yz2 + wx2) * scale.y;
	}
	{
		const float xy2 = rotation.x * y2;
		const float wz2 = rotation.w * z2;

		result.m01 = (xy2 - wz2) * scale.y;
		result.m10 = (xy2 + wz2) * scale.x;
	}
	{
		const float xz2 = rotation.x * z2;
		const float wy2 = rotation.w * y2;

		result.m02 = (xz2 + wy2) * scale.z;
		result.m20 = (xz2 - wy2) * scale.x;
	}

	result.m30 = 0.f;
	result.m31 = 0.f;
	result.m32 = 0.f;
	result.m33 = 1.f;
#endif
	return result;
}

mat4 create_billboard_model_matrix(vec3 position, vec3 eye, vec3 scale)
{
	vec3 up(0.f, 1.f, 0.f);
	vec3 forward = normalize(eye - position);
	vec3 right = normalize(cross(up, forward));
	up = cross(forward, right);

	right *= scale.x;
	up *= scale.y;
	forward *= scale.z;

	mat4 result;

	result.m00 = right.x;
	result.m10 = right.y;
	result.m20 = right.z;
	result.m30 = 0.f;

	result.m01 = up.x;
	result.m11 = up.y;
	result.m21 = up.z;
	result.m31 = 0.f;

	result.m02 = forward.x;
	result.m12 = forward.y;
	result.m22 = forward.z;
	result.m32 = 0.f;

	result.m03 = position.x;
	result.m13 = position.y;
	result.m23 = position.z;
	result.m33 = 1.f;

	return result;

}

mat4 trs_to_mat4(const trs& transform)
{
	return create_model_matrix(transform.position, transform.rotation, transform.scale);
}

trs mat4_to_trs(const mat4& m)
{
	trs result;

	vec3 c0(m.m00, m.m10, m.m20);
	vec3 c1(m.m01, m.m11, m.m21);
	vec3 c2(m.m02, m.m12, m.m22);
	result.scale.x = sqrt(dot(c0, c0));
	result.scale.y = sqrt(dot(c1, c1));
	result.scale.z = sqrt(dot(c2, c2));

	vec3 inv_scale = 1.f / result.scale;

	result.position.x = m.m03;
	result.position.y = m.m13;
	result.position.z = m.m23;

	mat3 R;

	R.m00 = m.m00 * inv_scale.x;
	R.m10 = m.m10 * inv_scale.x;
	R.m20 = m.m20 * inv_scale.x;
					
	R.m01 = m.m01 * inv_scale.y;
	R.m11 = m.m11 * inv_scale.y;
	R.m21 = m.m21 * inv_scale.y;
					
	R.m02 = m.m02 * inv_scale.z;
	R.m12 = m.m12 * inv_scale.z;
	R.m22 = m.m22 * inv_scale.z;

	result.rotation = mat3_to_quaternion(R);

	return result;
}

mat4 create_view_matrix(vec3 eye, float pitch, float yaw)
{
	float cos_pitch = cosf(pitch);
	float sin_pitch = sinf(pitch);
	float cos_yaw = cosf(yaw);
	float sin_yaw = sinf(yaw);

	vec3 xAxis(cos_yaw, 0, -sin_yaw);
	vec3 yAxis(sin_yaw * sin_pitch, cos_pitch, cos_yaw * sin_pitch);
	vec3 zAxis(sin_yaw * cos_pitch, -sin_pitch, cos_pitch * cos_yaw);

	mat4 result;
	result.m00 = xAxis.x; result.m10 = yAxis.x; result.m20 = zAxis.x; result.m30 = 0.f;
	result.m01 = xAxis.y; result.m11 = yAxis.y; result.m21 = zAxis.y; result.m31 = 0.f;
	result.m02 = xAxis.z; result.m12 = yAxis.z; result.m22 = zAxis.z; result.m32 = 0.f;
	result.m03 = -dot(xAxis, eye); result.m13 = -dot(yAxis, eye); result.m23 = -dot(zAxis, eye); result.m33 = 1.f;

	return result;
}

mat4 create_sky_view_matrix(const mat4& v)
{
	mat4 result = v;
	result.m03 = 0.f; result.m13 = 0.f; result.m23 = 0.f;
	return result;
}

mat4 look_at(vec3 eye, vec3 target, vec3 up)
{
	vec3 zAxis = normalize(eye - target);
	vec3 xAxis = normalize(cross(up, zAxis));
	vec3 yAxis = normalize(cross(zAxis, xAxis));

	mat4 result;
	result.m00 = xAxis.x; result.m10 = yAxis.x; result.m20 = zAxis.x; result.m30 = 0.f;
	result.m01 = xAxis.y; result.m11 = yAxis.y; result.m21 = zAxis.y; result.m31 = 0.f;
	result.m02 = xAxis.z; result.m12 = yAxis.z; result.m22 = zAxis.z; result.m32 = 0.f;
	result.m03 = -dot(xAxis, eye); result.m13 = -dot(yAxis, eye); result.m23 = -dot(zAxis, eye); result.m33 = 1.f;

	return result;
}

quat look_at_quaternion(vec3 forward, vec3 up)
{
	vec3 zAxis = -normalize(forward);
	vec3 xAxis = normalize(cross(up, zAxis));
	vec3 yAxis = normalize(cross(zAxis, xAxis));

	mat3 m;
	m.m00 = xAxis.x; m.m01 = yAxis.x; m.m02 = zAxis.x;
	m.m10 = xAxis.y; m.m11 = yAxis.y; m.m12 = zAxis.y;
	m.m20 = xAxis.z; m.m21 = yAxis.z; m.m22 = zAxis.z;

	return mat3_to_quaternion(m);
}

mat4 create_view_matrix(vec3 position, quat rotation)
{
	vec3 target = position + rotation * vec3(0.f, 0.f, -1.f);
	vec3 up = rotation * vec3(0.f, 1.f, 0.f);
	return look_at(position, target, up);
}

mat4 invert_affine(const mat4& m)
{
	vec3 xAxis(m.m00, m.m10, m.m20);
	vec3 yAxis(m.m01, m.m11, m.m21);
	vec3 zAxis(m.m02, m.m12, m.m22);
	vec3 pos(m.m03, m.m13, m.m23);

	vec3 inv_x_axis = xAxis / squared_length(xAxis);
	vec3 inv_y_xis = yAxis / squared_length(yAxis);
	vec3 inv_z_axis = zAxis / squared_length(zAxis);
	vec3 inv_pos(-dot(inv_x_axis, pos), -dot(inv_y_xis, pos), -dot(inv_z_axis, pos));

	mat4 result;
	result.m00 = inv_x_axis.x; result.m10 = inv_y_xis.x; result.m20 = inv_z_axis.x; result.m30 = 0.f;
	result.m01 = inv_x_axis.y; result.m11 = inv_y_xis.y; result.m21 = inv_z_axis.y; result.m31 = 0.f;
	result.m02 = inv_x_axis.z; result.m12 = inv_y_xis.z; result.m22 = inv_z_axis.z; result.m32 = 0.f;
	result.m03 = inv_pos.x; result.m13 = inv_pos.y; result.m23 = inv_pos.z; result.m33 = 1.f;
	return result;
}

bool point_in_triangle(vec3 point, vec3 triA, vec3 triB, vec3& triC)
{
	vec3 e10 = triB - triA;
	vec3 e20 = triC - triA;
	float a = dot(e10, e10);
	float b = dot(e10, e20);
	float c = dot(e20, e20);
	float ac_bb = (a * c) - (b * b);
	vec3 vp = point - triA;
	float d = dot(vp, e10);
	float e = dot(vp, e20);
	float x = (d * c) - (e * b);
	float y = (e * a) - (d * b);
	float z = x + y - ac_bb;
#define uintCast(a) ((uint32&) a)
	return ((uintCast(z) & ~(uintCast(x) | uintCast(y))) & 0x80000000) != 0;
#undef uintCast
}

bool point_in_rectangle(vec2 p, vec2 top_left, vec2 bottom_right)
{
	return p.x >= top_left.x && p.y >= top_left.y && p.x <= bottom_right.x && p.y <= bottom_right.y;
}

bool point_in_box(vec3 p, vec3 min_corner, vec3 max_corner)
{
	return p.x >= min_corner.x && p.y >= min_corner.y && p.z >= min_corner.z && p.x <= max_corner.x && p.y <= max_corner.y && p.z <= max_corner.z;
}

vec2 direction_to_panorama_uv(vec3 dir)
{
	const vec2 invAtan = vec2(INV_TAU, INV_PI);

	vec2 panoUV = vec2(atan2(-dir.x, -dir.z), acos(dir.y)) * invAtan;

	while (panoUV.x < 0.f) { panoUV.x += 1.f; }
	while (panoUV.y < 0.f) { panoUV.y += 1.f; }
	while (panoUV.x > 1.f) { panoUV.x -= 1.f; }
	while (panoUV.y > 1.f) { panoUV.y -= 1.f; }

	return panoUV;
}

float angle_to_zero_to_two_pi(float angle)
{
	while (angle < 0)
	{
		angle += M_TAU;
	}
	while (angle > M_TAU)
	{
		angle -= M_TAU;
	}
	return angle;
}

float angle_to_neg_pi_to_pi(float angle)
{
	while (angle < -M_PI)
	{
		angle += M_TAU;
	}
	while (angle > M_PI)
	{
		angle -= M_TAU;
	}
	return angle;
}

vec2 solve_linear_system(const mat2& A, vec2 b)
{
	float a11 = A.m00, a12 = A.m01, a21 = A.m10, a22 = A.m11;
	float det = a11 * a22 - a12 * a21;
	if (det != 0.f)
	{
		det = 1.f / det;
	}
	vec2 x;
	x.x = det * (a22 * b.x - a12 * b.y);
	x.y = det * (a11 * b.y - a21 * b.x);
	return x;
}

vec3 solve_linear_system(const mat3& A, vec3 b)
{
	vec3 ex(A.m00, A.m10, A.m20);
	vec3 ey(A.m01, A.m11, A.m21);
	vec3 ez(A.m02, A.m12, A.m22);
	float det = dot(ex, cross(ey, ez));
	if (det != 0.f)
	{
		det = 1.f / det;
	}
	vec3 x;
	x.x = det * dot(b, cross(ey, ez));
	x.y = det * dot(ex, cross(b, ez));
	x.z = det * dot(ex, cross(ey, b));
	return x;
}

vec3 get_barycentric_coordinates(vec2 a, vec2 b, vec2 c, vec2 p)
{
	vec2 v0 = b - a, v1 = c - a, v2 = p - a;
	float d00 = dot(v0, v0);
	float d01 = dot(v0, v1);
	float d11 = dot(v1, v1);
	float d20 = dot(v2, v0);
	float d21 = dot(v2, v1);
	float denom = d00 * d11 - d01 * d01;

	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;

	return vec3(u, v, w);
}

vec3 get_barycentric_coordinates(vec3 a, vec3 b, vec3 c, vec3 p)
{
	vec3 v0 = b - a, v1 = c - a, v2 = p - a;
	float d00 = dot(v0, v0);
	float d01 = dot(v0, v1);
	float d11 = dot(v1, v1);
	float d20 = dot(v2, v0);
	float d21 = dot(v2, v1);
	float denom = d00 * d11 - d01 * d01;

	denom = (abs(denom) < EPSILON) ? 1.f : denom;

	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;

	return vec3(u, v, w);
}

bool inside_triangle(vec3 barycentrics)
{
	return barycentrics.x >= 0.f
		&& barycentrics.y >= 0.f
		&& barycentrics.z >= 0.f;
}

vec3 get_tangent(vec3 normal)
{
	vec3 tangent = (abs(normal.x) >= 0.57735f) ? vec3(normal.y, -normal.x, 0.f) : vec3(0.f, normal.z, -normal.y);
	return normalize(tangent);
}

void get_tangents(vec3 normal, vec3& out_tangent, vec3& out_bitangent)
{
	out_tangent = get_tangent(normal);
	out_bitangent = cross(normal, out_tangent);
}

vec4 uniform_sample_sphere(vec2 E)
{
	float phi = 2 * M_PI * E.x;
	float cos_theta = 1.f - 2.f * E.y;
	float sin_theta = sqrt(1.f - cos_theta * cos_theta);

	vec3 H;
	H.x = sin_theta * cos(phi);
	H.y = sin_theta * sin(phi);
	H.z = cos_theta;

	float PDF = 1.f / (4.f * M_PI);

	return vec4(H, PDF);
}

vec3 local_to_world(const vec3& local_pos, const trs& transform)
{
	mat4 model =
	{
		1, 0, 0, transform.position.x,
		0, 1, 0, transform.position.y,
		0, 0, 1, transform.position.z,
		0, 0, 0, 1
	};

	mat4 rot = quaternion_to_mat4(transform.rotation);

	mat4 scaleMatrix =
	{
		transform.scale.x, 0, 0, 0,
		0, transform.scale.y, 0, 0,
		0, 0, transform.scale.z, 0,
		0, 0, 0, 1
	};

	mat4 model_matrix = model * rot * scaleMatrix;

	vec4 local_pos4 = { local_pos.x, local_pos.y, local_pos.z, 1 };
	vec4 world_pos4 = model_matrix * local_pos4;

	return { world_pos4.x, world_pos4.y, world_pos4.z };
}

#define _gamma 5.828427124f // FOUR_GAMMA_SQUARED = sqrt(8)+3
#define _cstar 0.923879532f // cos(pi/8)
#define _sstar 0.3826834323f // sin(p/8)

static void cond_swap(bool c, float& X, float& Y)
{
	float Z = X;
	X = c ? Y : X;
	Y = c ? Z : Y;
}

static void cond_neg_swap(bool c, float& X, float& Y)
{
	float Z = -X;
	X = c ? Y : X;
	Y = c ? Z : Y;
}

static void approximate_givens_quaternion(float a11, float a12, float a22, float& ch, float& sh)
{
	// Given givens angle computed by approximateGivensAngles,
	// compute the corresponding rotation quaternion
	ch = 2 * (a11 - a22);
	sh = a12;
	bool b = _gamma * sh * sh < ch * ch;
	float w = 1.f / sqrt(ch * ch + sh * sh);
	ch = b ? w * ch : _cstar;
	sh = b ? w * sh : _sstar;
}

static void qr_givens_quaternion(float a1, float a2, float& ch, float& sh)
{
	// a1 = pivot point on diagonal
	// a2 = lower triangular entry we want to annihilate
	float rho = sqrt(a1 * a1 + a2 * a2);

	sh = rho > EPSILON ? a2 : 0.f;
	ch = fabs(a1) + fmax(rho, EPSILON);
	bool b = a1 < 0;
	cond_swap(b, sh, ch);
	float w = 1.f / sqrt(ch * ch + sh * sh);
	ch *= w;
	sh *= w;
}

static void jacobi_conjugation(const int x, const int y, const int z,
	float& s11,
	float& s21, float& s22,
	float& s31, float& s32, float& s33,
	quat& q)
{
	float ch, sh;
	approximate_givens_quaternion(s11, s21, s22, ch, sh);

	float scale = ch * ch + sh * sh;
	float a = (ch * ch - sh * sh) / scale;
	float b = (2 * sh * ch) / scale;

	// make temp copy of S
	float _s11 = s11;
	float _s21 = s21; float _s22 = s22;
	float _s31 = s31; float _s32 = s32; float _s33 = s33;

	// perform conjugation S = Q'*S*Q
	// Q already implicitly solved from a, b
	s11 = a * (a * _s11 + b * _s21) + b * (a * _s21 + b * _s22);
	s21 = a * (-b * _s11 + a * _s21) + b * (-b * _s21 + a * _s22);	s22 = -b * (-b * _s11 + a * _s21) + a * (-b * _s21 + a * _s22);
	s31 = a * _s31 + b * _s32;								s32 = -b * _s31 + a * _s32; s33 = _s33;

	// update cumulative rotation qV
	float tmp[3];
	tmp[0] = q.x * sh;
	tmp[1] = q.y * sh;
	tmp[2] = q.z * sh;
	sh *= q.w;

	q.x *= ch;
	q.y *= ch;
	q.z *= ch;
	q.w *= ch;

	// (x,y,z) corresponds to ((0,1,2),(1,2,0),(2,0,1))
	// for (p,q) = ((0,1),(1,2),(0,2))
	q.v4.data[z] += sh;
	q.v4.w -= tmp[z]; // w
	q.v4.data[x] += tmp[y];
	q.v4.data[y] -= tmp[x];

	// re-arrange matrix for next iteration
	_s11 = s22;
	_s21 = s32; _s22 = s33;
	_s31 = s21; _s32 = s31; _s33 = s11;
	s11 = _s11;
	s21 = _s21; s22 = _s22;
	s31 = _s31; s32 = _s32; s33 = _s33;
}

static void jacobi_eigenanlysis(
	float& s11,
	float& s21, float& s22,
	float& s31, float& s32, float& s33,
	// quaternion representation of V
	quat& q)
{
	q = quat::identity;
	for (int i = 0; i < 4; ++i)
	{
		// We wish to eliminate the maximum off-diagonal element
		// on every iteration, but cycling over all 3 possible rotations
		// in fixed order (p,q) = (1,2) , (2,3), (1,3) still retains
		//  asymptotic convergence.
		jacobi_conjugation(0, 1, 2, s11, s21, s22, s31, s32, s33, q); // p,q = 0,1
		jacobi_conjugation(1, 2, 0, s11, s21, s22, s31, s32, s33, q); // p,q = 1,2
		jacobi_conjugation(2, 0, 1, s11, s21, s22, s31, s32, s33, q); // p,q = 0,2
	}
}

static float dist2(float x, float y, float z)
{
	return x * x + y * y + z * z;
}

static void sort_singular_values(mat3& B, mat3& V)
{
	float* Bm = B.m;
	float* Vm = V.m;
	float rho1 = dist2(Bm[0], Bm[1], Bm[2]);
	float rho2 = dist2(Bm[3], Bm[4], Bm[5]);
	float rho3 = dist2(Bm[6], Bm[7], Bm[8]);

	bool c = rho1 < rho2;
	cond_neg_swap(c, Bm[0], Bm[3]); cond_neg_swap(c, Vm[0], Vm[3]);
	cond_neg_swap(c, Bm[1], Bm[4]); cond_neg_swap(c, Vm[1], Vm[4]);
	cond_neg_swap(c, Bm[2], Bm[5]); cond_neg_swap(c, Vm[2], Vm[5]);
	cond_swap(c, rho1, rho2);
	c = rho1 < rho3;
	cond_neg_swap(c, Bm[0], Bm[6]); cond_neg_swap(c, Vm[0], Vm[6]);
	cond_neg_swap(c, Bm[1], Bm[7]); cond_neg_swap(c, Vm[1], Vm[7]);
	cond_neg_swap(c, Bm[2], Bm[8]); cond_neg_swap(c, Vm[2], Vm[8]);
	cond_swap(c, rho1, rho3);
	c = rho2 < rho3;
	cond_neg_swap(c, Bm[3], Bm[6]); cond_neg_swap(c, Vm[3], Vm[6]);
	cond_neg_swap(c, Bm[4], Bm[7]); cond_neg_swap(c, Vm[4], Vm[7]);
	cond_neg_swap(c, Bm[5], Bm[8]); cond_neg_swap(c, Vm[5], Vm[8]);
}

static void qr_decomposition(mat3& B, mat3& Q, mat3& R)
{
	float ch1, sh1, ch2, sh2, ch3, sh3;
	float a, b;

	// first givens rotation (ch,0,0,sh)
	qr_givens_quaternion(B.m00, B.m10, ch1, sh1);
	a = 1 - 2 * sh1 * sh1;
	b = 2 * ch1 * sh1;

	// Apply B = Q' * B.
	float* Rm = R.m;
	float* Bm = B.m;
	Rm[0] = a * Bm[0] + b * Bm[1];  Rm[3] = a * Bm[3] + b * Bm[4];  Rm[6] = a * Bm[6] + b * Bm[7];
	Rm[1] = -b * Bm[0] + a * Bm[1]; Rm[4] = -b * Bm[3] + a * Bm[4]; Rm[7] = -b * Bm[6] + a * Bm[7];
	Rm[2] = Bm[2];          Rm[5] = Bm[5];          Rm[8] = Bm[8];

	// Second givens rotation (ch,0,-sh,0).
	qr_givens_quaternion(Rm[0], Rm[2], ch2, sh2);
	a = 1 - 2 * sh2 * sh2;
	b = 2 * ch2 * sh2;

	// Apply B = Q' * B.
	Bm[0] = a * Rm[0] + b * Rm[2];  Bm[3] = a * Rm[3] + b * Rm[5];  Bm[6] = a * Rm[6] + b * Rm[8];
	Bm[1] = Rm[1];           Bm[4] = Rm[4];           Bm[7] = Rm[7];
	Bm[2] = -b * Rm[0] + a * Rm[2]; Bm[5] = -b * Rm[3] + a * Rm[5]; Bm[8] = -b * Rm[6] + a * Rm[8];

	// Third givens rotation (ch,sh,0,0).
	qr_givens_quaternion(Bm[4], Bm[5], ch3, sh3);
	a = 1 - 2 * sh3 * sh3;
	b = 2 * ch3 * sh3;
	// R is now set to desired value.
	Rm[0] = Bm[0];             Rm[3] = Bm[3];           Rm[6] = Bm[6];
	Rm[1] = a * Bm[1] + b * Bm[2];     Rm[4] = a * Bm[4] + b * Bm[5];   Rm[7] = a * Bm[7] + b * Bm[8];
	Rm[2] = -b * Bm[1] + a * Bm[2];    Rm[5] = -b * Bm[4] + a * Bm[5];  Rm[8] = -b * Bm[7] + a * Bm[8];

	// Construct the cumulative rotation Q=Q1 * Q2 * Q3.
	// The number of floating point operations for three quaternion multiplications
	// is more or less comparable to the explicit form of the joined matrix.
	// Certainly more memory-efficient!
	float sh12 = sh1 * sh1;
	float sh22 = sh2 * sh2;
	float sh32 = sh3 * sh3;

	float* Qm = Q.m;
	Qm[0] = (-1 + 2 * sh12) * (-1 + 2 * sh22);
	Qm[3] = 4 * ch2 * ch3 * (-1 + 2 * sh12) * sh2 * sh3 + 2 * ch1 * sh1 * (-1 + 2 * sh32);
	Qm[6] = 4 * ch1 * ch3 * sh1 * sh3 - 2 * ch2 * (-1 + 2 * sh12) * sh2 * (-1 + 2 * sh32);

	Qm[1] = 2 * ch1 * sh1 * (1 - 2 * sh22);
	Qm[4] = -8 * ch1 * ch2 * ch3 * sh1 * sh2 * sh3 + (-1 + 2 * sh12) * (-1 + 2 * sh32);
	Qm[7] = -2 * ch3 * sh3 + 4 * sh1 * (ch3 * sh1 * sh3 + ch1 * ch2 * sh2 * (-1 + 2 * sh32));

	Qm[2] = 2 * ch2 * sh2;
	Qm[5] = 2 * ch3 * (1 - 2 * sh22) * sh3;
	Qm[8] = (-1 + 2 * sh22) * (-1 + 2 * sh32);
}

singular_value_decomposition compute_svd(const mat3& A)
{
	mat3 ATA = transpose(A) * A;

	mat3 U, V;

	quat q;
	jacobi_eigenanlysis(ATA.m00, ATA.m10, ATA.m11, ATA.m20, ATA.m21, ATA.m22, q);
	V = quaternion_to_mat3(q);

	mat3 B = A * V;

	sort_singular_values(B, V);
	mat3 S;
	qr_decomposition(B, U, S);

	return { U, V, vec3(S.m00, S.m11, S.m22) };
}

static void compute_minor(mat3& m, const mat3& from, uint32 d)
{
	for (uint32 y = 0; y < 3; ++y)
	{
		for (uint32 x = 0; x < 3; ++x)
		{
			if (x < d || y < d)
			{
				m.m[y * 3 + x] = (x == y) ? 1.f : 0.f;
			}
			else
			{
				m.m[y * 3 + x] = from.m[y * 3 + x];
			}
		}
	}
}

static void compute_householder_factor(mat3& mat, const vec3& v)
{
	for (uint32 i = 0; i < 3; ++i)
	{
		for (uint32 j = 0; j < 3; ++j)
		{
			mat.m[i * 3 + j] = -2.f * v.data[i] * v.data[j];
		}
	}
	for (uint32 i = 0; i < 3; ++i)
	{
		mat.m[i * 3 + i] += 1.f;
	}
}

QRDecomposition create_qr_decomposition(const mat3& mat)
{
	mat3 qv[2];

	// Temp array.
	mat3 z = mat;
	mat3 z1;

	for (uint32 k = 0; k < 2; ++k)
	{
		compute_minor(z1, z, k);
		vec3 x = col(z1, k);

		float a = length(x);
		if (mat.m[k * 3 + k] > 0.f)
		{
			a = -a;
		}

		vec3 e;
		for (uint32 i = 0; i < 3; ++i)
		{
			e.data[i] = (i == k) ? 1.f : 0.f;
		}

		e = x + a * e;
		e = normalize(e);

		// qv[k] = I - 2 *e*e^T
		compute_householder_factor(qv[k], e);

		z = qv[k] * z1;

	}

	QRDecomposition result;
	result.Q = qv[0];

	// After this loop, we will obtain Q (up to a transpose operation)
	for (uint32 i = 1; i < 2; ++i)
	{
		z1 = qv[i] * result.Q;
		result.Q = z1;
	}

	result.R = result.Q * mat;
	result.Q = transpose(result.Q);

	return result;
}

bool get_eigen(const mat3& A, vec3& out_eigen_values, mat3& out_eigen_vectors)
{
	mat3 Q = create_qr_decomposition(A).Q;
	mat3 Qt = transpose(Q);

	mat3 tmp = Qt * A;
	mat3 E = tmp * Q;

	mat3 U = Q;

	vec3 res = diagonal(E);
	vec3 init = diagonal(A);

	vec3 error = init - res;
	double e = dot(error, error);

	while (e > 0.001f)
	{
		init = res;

		mat3 Q = create_qr_decomposition(E).Q;
		mat3 Qt = transpose(Q);

		tmp = Qt * E;
		E = tmp * Q;

		tmp = U * Q;
		U = tmp;

		res = diagonal(E);
		vec3 error = init - res;
		e = dot(error, error);
	}

	out_eigen_values = res;
	out_eigen_vectors = U;

	return true;
}

quat shortest_arc(const vec3& from, const vec3& to)
{
	const float dot_product = dot(from, to);
	if (fuzzy_equals(dot_product, 1.f))
	{
		return {};
	}
	else if (fuzzy_equals(dot_product, -1.f))
	{
		vec3 non_collinear_vector(0.f, 1.f, 0.f);
		if (fuzzy_equals(abs(dot(from, non_collinear_vector)), 1.f))
		{
			non_collinear_vector = vec3(1.f, 0.f, 0.f);
		}

		const vec3 orthogonal_vector = normalize(cross(from, non_collinear_vector));

		return quat(orthogonal_vector.x, orthogonal_vector.y, orthogonal_vector.z, 0.f);
	}
	else
	{
		const vec3 axis = cross(from, to);
		const quat unorm = quat(axis.x, axis.y, axis.z, dot_product + 1.f);
		return normalize(unorm);
	}
}