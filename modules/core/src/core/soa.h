// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

typedef float* soa_float;
typedef int32* soa_int32;
typedef uint32* soa_uint32;
typedef uint16* soa_uint16;

struct ERA_CORE_API soa_vec2
{
	float* x, y;
};

struct ERA_CORE_API soa_vec3
{
	float* x, y, z;
};

struct ERA_CORE_API soa_vec4
{
	float* x, y, z, w;
};

struct ERA_CORE_API soa_quat
{
	float* x, y, z, w;
};

struct ERA_CORE_API soa_mat2
{
	float*
		m00, m10,
		m01, m11;
};

struct ERA_CORE_API soa_mat3
{
	float*
		m00, m10, m20,
		m01, m11, m21,
		m02, m12, m22;
};

struct ERA_CORE_API soa_mat4
{
	float*
		m00, m10, m20, m30,
		m01, m11, m21, m31,
		m02, m12, m22, m32,
		m03, m13, m23, m33;
};