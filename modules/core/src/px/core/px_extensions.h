// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"
#include "core/math_simd.h"

#include "application.h"

#include "px/core/px_physics_engine.h"

namespace era_engine::physics
{
	using namespace physx;

	struct physics_lock
	{
		virtual void lock() = 0;
		virtual void unlock() = 0;

		virtual ~physics_lock() {}
	};

	struct physics_lock_read : physics_lock
	{
		physics_lock_read()
		{
			lock();
		}

		~physics_lock_read()
		{
			unlock();
		}

		virtual void lock()
		{
			physics_holder::physicsRef->lockRead();
		}

		virtual void unlock()
		{
			physics_holder::physicsRef->unlockRead();
		}
	};

	struct physics_lock_write : physics_lock
	{
		physics_lock_write()
		{
			lock();
		}

		~physics_lock_write()
		{
			unlock();
		}

		virtual void lock()
		{
			physics_holder::physicsRef->lockWrite();
		}

		virtual void unlock()
		{
			physics_holder::physicsRef->unlockWrite();
		}
	};

	// Computation by Direct Parameterization of Triangles
	void computeIntegralTerm(float w0, float w1, float w2, float& f1, float& f2, float& f3, float& g0, float& g1, float& g2);

	void computeCenterMassAndInertia(PxVec3* points, int tmax, PxU32* index,
		float mass, PxVec3& cm, mat3& inertia);
	

	vec3 localToWorld(const vec3& localPos, const trs& transform);

	vec3 getWorldLossyScale(const trs& transform, const trs& parentTransform);

	struct px_explode_overlap_callback : PxOverlapCallback
	{
		px_explode_overlap_callback(PxVec3 worldPos, float radius, float explosiveImpulse)
			: worldPosition(worldPos)
			, radius(radius)
			, explosiveImpulse(explosiveImpulse)
			, PxOverlapCallback(hitBuffer, sizeof(hitBuffer) / sizeof(hitBuffer[0])) {}

		PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits);

	private:
		std::set<PxRigidDynamic*> actorBuffer;
		float explosiveImpulse;
		float radius;
		PxVec3 worldPosition;
		PxOverlapHit hitBuffer[512];
	};

	struct px_debug_render_buffer : PxRenderBuffer
	{
		~px_debug_render_buffer() {}

		virtual PxU32 getNbPoints() const { return 0; }
		virtual const PxDebugPoint* getPoints() const { return nullptr; }

		virtual PxU32 getNbLines() const { return static_cast<PxU32>(lines.size()); }
		virtual const PxDebugLine* getLines() const { return lines.data(); }

		virtual PxU32 getNbTriangles() const { return 0; }
		virtual const PxDebugTriangle* getTriangles() const { return nullptr; }

		virtual PxU32 getNbTexts() const { return 0; }
		virtual const PxDebugText* getTexts() const { return nullptr; }

		virtual void append(const PxRenderBuffer& other) {}
		virtual void clear()
		{
			lines.clear();
		}

		virtual PxDebugLine* reserveLines(const PxU32 nbLines) { lines.reserve(nbLines); return &lines[0]; }

		virtual bool empty() const { return lines.empty(); }

		// Unused
		virtual void addPoint(const PxDebugPoint& point) {}

		// Unused
		virtual void addLine(const PxDebugLine& line) {}

		// Unused
		virtual PxDebugPoint* reservePoints(const PxU32 nbLines) { return nullptr; }

		// Unused
		virtual void addTriangle(const PxDebugTriangle& triangle) {}

		// Unused
		virtual void shift(const PxVec3& delta) {}

		std::vector<PxDebugLine> lines;
	};

	void pushVertex(std::vector<vec3>* vertexBuffer, const PxVec3& v0, const PxVec3& v1, const PxVec3& v2, const PxVec3& n);

	struct px_simple_mesh
	{
		struct vertex
		{
			physx::PxVec3 position;
			physx::PxVec3 normal;
			physx::PxVec2 uv;
		};

		constexpr uint32_t getVertexStride() const { return sizeof(vertex); }

		std::vector<vertex> vertices;
		std::vector<uint32_t> indices;

		physx::PxVec3 extents;
		physx::PxVec3 center;
	};

	struct bounds
	{
		void setMinMax(const vec3& min, const vec3& max);

		void encapsulate(const vec3& point);

		void encapsulate(const bounds& bounds);

		vec3 center;
		vec3 extents;
	};

	bounds toBounds(std::vector<vec3> vertices);

	std::vector<PxVec3> createStdVectorPxVec3(const std::vector<vec3> vec);

	std::vector<PxVec2> createStdVectorPxVec2(const ::std::vector<vec2> vec);

	float signedVolumeOfTriangle(const vec3& p1, const vec3& p2, const vec3& p3);

	float volumeOfMesh(ref<submesh_asset> mesh);

	bool validateMesh(ref<submesh_asset> mesh);
	
}