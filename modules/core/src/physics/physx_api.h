#pragma once

#ifdef setBit
#undef setBit
#endif // setBit

#ifndef PX_PHYSX_STATIC_LIB
#define PX_PHYSX_STATIC_LIB
#endif

#include <cuda.h>
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <extensions/PxRaycastCCD.h>
#include <extensions/PxRemeshingExt.h>
#include <extensions/PxSoftBodyExt.h>
#include <cudamanager/PxCudaContextManager.h>
#include <extensions/PxParticleExt.h>
#include <PxPBDParticleSystem.h>
#include <extensions/PxParticleClothCooker.h>
#include <cudamanager/PxCudaContext.h>

#define PX_CONTACT_BUFFER_SIZE 64

#define PVD_HOST "127.0.0.1"

#define PX_ENABLE_RAYCAST_CCD 0

#define PX_PARTICLE_USE_ALLOCATOR 1

#define PX_PHYSICS_ENABLED = 1

#define PX_NB_MAX_RAYCAST_HITS 64
#define PX_NB_MAX_RAYCAST_DISTANCE 128

#define PX_VEHICLE 1

#if DEBUG
#define PX_BLAST_ENABLE 0
#define PX_VEHICLE_DEBUG 1
#elif _DEBUG
#define PX_BLAST_ENABLE 0
#define PX_VEHICLE_DEBUG 1
#else
#define PX_BLAST_ENABLE 1
#define PX_VEHICLE_DEBUG 0
#endif

#ifndef PX_RELEASE
#define PX_RELEASE(x)	if(x)	{ x->release(); x = nullptr;}
#endif

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#define PX_DEVICE_ALLOC(cudaContextManager, deviceBuffer, numElements) cudaContextManager->allocDeviceBuffer(deviceBuffer, numElements, PX_FL)
#define PX_DEVICE_ALLOC_T(T, cudaContextManager, numElements) cudaContextManager->allocDeviceBuffer<T>(numElements, PX_FL)
#define PX_DEVICE_FREE(cudaContextManager, deviceBuffer) cudaContextManager->freeDeviceBuffer(deviceBuffer);

#define PX_PINNED_HOST_ALLOC(cudaContextManager, pinnedHostBuffer, numElements) cudaContextManager->allocPinnedHostBuffer(pinnedHostBuffer, numElements, PX_FL)
#define PX_PINNED_HOST_ALLOC_T(T, cudaContextManager, numElements) cudaContextManager->allocPinnedHostBuffer<T>(numElements, PX_FL)
#define PX_PINNED_HOST_FREE(cudaContextManager, pinnedHostBuffer) cudaContextManager->freePinnedHostBuffer(pinnedHostBuffer);

#define PX_SCENE_QUERY_SETUP_SWEEP_CAST_ALL() PX_SCENE_QUERY_SETUP(true); \
		physx::PxSweepBufferN<1> buffer

#define PX_SCENE_QUERY_SETUP_SWEEP_CAST() PX_SCENE_QUERY_SETUP(false); \
		px_dynamic_hit_buffer<physx::PxSweepHit> buffer

#define PX_SCENE_QUERY_SETUP_CHECK() PX_SCENE_QUERY_SETUP(false); \
		physx::PxOverlapBufferN<1> buffer

#define PX_SCENE_QUERY_SETUP_OVERLAP() PX_SCENE_QUERY_SETUP(false); \
		px_dynamic_hit_buffer<physx::PxOverlapHit> buffer

#define PX_SCENE_QUERY_COLLECT_OVERLAP() results.clear(); \
		results.resize(buffer.getNbTouches()); \
		size_t resultSize = results.size(); \
		for (int32 i = 0; i < resultSize; i++) \
		{ \
			const auto& hit = buffer.getTouch(i); \
			results[i] = hit.shape ? *reinterpret_cast<uint32_t*>(hit.shape->userData) : 0; \
		}

#if PX_VEHICLE
#include <vehicle/PxVehicleUtil.h>
#include <snippetutils/SnippetUtils.h>
#include <snippetvehicle2common/enginedrivetrain/EngineDrivetrain.h>
#include <snippetvehicle2common/serialization/BaseSerialization.h>
#include <snippetvehicle2common/serialization/EngineDrivetrainSerialization.h>
#include <snippetvehicle2common/SnippetVehicleHelpers.h>
#endif

#include "core/math.h"
#include "core/math_simd.h"

namespace physx
{
	constexpr float PX_HALF_MAX_F32 = PX_MAX_F32 / 2.0f;

	PX_FORCE_INLINE constexpr PxU32 id(PxU32 x, PxU32 y, PxU32 numY) { return x * numY + y; }

	inline PxVec3 operator*(PxVec3 a, PxVec3 b) { return PxVec3(a.x * b.x, a.y * b.y, a.z * b.z); }
	inline PxVec4 operator*(PxVec4 a, PxVec4 b) { return PxVec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }

	PX_FORCE_INLINE PxVec4 createPxVec4(const vec3& vec) { return PxVec4(vec.x, vec.y, vec.z, 0); }
	PX_FORCE_INLINE PxVec4 createPxVec4(const vec4& vec) { return PxVec4(vec.x, vec.y, vec.z, vec.w); }

	PX_FORCE_INLINE PxVec3 createPxVec3(const vec3& vec) { return PxVec3(vec.x, vec.y, vec.z); }
	PX_FORCE_INLINE PxVec2 createPxVec2(const vec2& vec) { return PxVec2(vec.x, vec.y); }
    PX_FORCE_INLINE PxVec3 createPxVec3(vec3&& vec) { return PxVec3(vec.x, vec.y, vec.z); }
	PX_FORCE_INLINE PxVec2 createPxVec2(vec2&& vec) { return PxVec2(vec.x, vec.y); }

	PX_FORCE_INLINE PxQuat createPxQuat(const quat& q) { return PxQuat(q.x, q.y, q.z, q.w); }
	PX_FORCE_INLINE PxQuat createPxQuat(quat&& q) { return PxQuat(q.x, q.y, q.z, q.w); }

	PX_FORCE_INLINE vec3 createVec3(const PxVec3& vec) { return vec3(vec.x, vec.y, vec.z); }
	PX_FORCE_INLINE vec2 createVec2(const PxVec2& vec) { return vec2(vec.x, vec.y); }
	PX_FORCE_INLINE vec3 createVec3(PxVec3&& vec) { return vec3(vec.x, vec.y, vec.z); }
	PX_FORCE_INLINE vec2 createVec2(PxVec2&& vec) { return vec2(vec.x, vec.y); }

	PX_FORCE_INLINE PxMat44 createPxMat44(const mat4& mat)
	{
		PxMat44 newMat;

		newMat[0][0] = mat.m00;
		newMat[0][1] = mat.m01;
		newMat[0][2] = mat.m02;
		newMat[0][3] = mat.m03;

		newMat[1][0] = mat.m10;
		newMat[1][1] = mat.m11;
		newMat[1][2] = mat.m12;
		newMat[1][3] = mat.m13;

		newMat[2][0] = mat.m20;
		newMat[2][1] = mat.m21;
		newMat[2][2] = mat.m22;
		newMat[2][3] = mat.m23;

		newMat[3][0] = mat.m30;
		newMat[3][1] = mat.m31;
		newMat[3][2] = mat.m32;
		newMat[3][3] = mat.m33;

		return newMat;
	}

	PX_FORCE_INLINE mat4 createMat44(const PxMat44& mat)
	{
		mat4 newMat;

		newMat.m00 = mat[0][0];
		newMat.m01 = mat[0][1];
		newMat.m02 = mat[0][2];
		newMat.m03 = mat[0][3];

		newMat.m10 = mat[1][0];
		newMat.m11 = mat[1][1];
		newMat.m12 = mat[1][2];
		newMat.m13 = mat[1][3];

		newMat.m20 = mat[2][0];
		newMat.m21 = mat[2][1];
		newMat.m22 = mat[2][2];
		newMat.m23 = mat[2][3];

		newMat.m30 = mat[3][0];
		newMat.m31 = mat[3][1];
		newMat.m32 = mat[3][2];
		newMat.m33 = mat[3][3];

		return newMat;
	}

	PX_FORCE_INLINE quat createQuat(const PxQuat& q) { return quat(q.x, q.y, q.z, q.w); }
	PX_FORCE_INLINE quat createQuat(PxQuat&& q) { return quat(q.x, q.y, q.z, q.w); }
							 
	PX_FORCE_INLINE PxVec2 min(const PxVec2& a, const PxVec2& b) { return PxVec2(std::min(a.x, b.x), std::min(a.y, b.y)); }
	PX_FORCE_INLINE PxVec3 min(const PxVec3& a, const PxVec3& b) { return PxVec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)); }
							 
	PX_FORCE_INLINE PxVec2 max(const PxVec2& a, const PxVec2& b) { return PxVec2(std::max(a.x, b.x), std::max(a.y, b.y)); }
	PX_FORCE_INLINE PxVec3 max(const PxVec3& a, const PxVec3& b) { return PxVec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)); }
}

namespace era_engine::physics
{
	static inline physx::PxVec3 gravity(0.0f, -9.81f, 0.0f);
}