#include <pch.h>

#include <NvBlastGlobals.h>
#include "NvBlastExtPxCollisionBuilderImpl.h"
#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtPxAsset.h"
#include <geometry/PxConvexMesh.h>
#include "PxPhysics.h"
#include "cooking/PxCooking.h"
#include <NvBlastPxSharedHelpers.h>
#include <vector>
#include <set>

using namespace physx;

#define SAFE_ARRAY_NEW(T, x) ((x) > 0) ? reinterpret_cast<T*>(NVBLAST_ALLOC(sizeof(T) * (x))) : nullptr;
#define SAFE_ARRAY_DELETE(x)                                                                                           \
	if (x != nullptr)                                                                                                  \
	{                                                                                                                  \
		NVBLAST_FREE(x);                                                                                               \
		x = nullptr;                                                                                                   \
	}

namespace Nv
{
namespace Blast
{

CollisionHullImpl::~CollisionHullImpl()
{
	SAFE_ARRAY_DELETE(points);
	SAFE_ARRAY_DELETE(indices);
	SAFE_ARRAY_DELETE(polygonData);
}

CollisionHullImpl::CollisionHullImpl(const CollisionHull& hullToCopy)
{
	pointsCount      = hullToCopy.pointsCount;
	indicesCount     = hullToCopy.indicesCount;
	polygonDataCount = hullToCopy.polygonDataCount;

	points      = SAFE_ARRAY_NEW(NvcVec3, pointsCount);
	indices     = SAFE_ARRAY_NEW(uint32_t, indicesCount);
	polygonData = SAFE_ARRAY_NEW(HullPolygon, polygonDataCount);
	memcpy(points, hullToCopy.points, sizeof(points[0]) * pointsCount);
	memcpy(indices, hullToCopy.indices, sizeof(indices[0]) * indicesCount);
	memcpy(polygonData, hullToCopy.polygonData, sizeof(polygonData[0]) * polygonDataCount);
}

CollisionHull* ExtPxCollisionBuilderImpl::buildCollisionGeometry(uint32_t verticesCount, const NvcVec3* vData)
{
	CollisionHull* output = new CollisionHullImpl();
	std::vector<physx::PxVec3> vertexData(verticesCount);
	memcpy(vertexData.data(), vData, sizeof(physx::PxVec3) * verticesCount);

	PxConvexMeshDesc convexMeshDescr;
	PxConvexMesh* resultConvexMesh;
	PxBounds3 bounds;
	// Scale chunk to unit cube size, to avoid numerical errors
	bounds.setEmpty();
	for (uint32_t i = 0; i < vertexData.size(); ++i)
	{
		bounds.include(vertexData[i]);
	}
	PxVec3 bbCenter = bounds.getCenter();
	float scale = PxMax(PxAbs(bounds.getExtents(0)), PxMax(PxAbs(bounds.getExtents(1)), PxAbs(bounds.getExtents(2))));
	for (uint32_t i = 0; i < vertexData.size(); ++i)
	{
		vertexData[i] = vertexData[i] - bbCenter;
		vertexData[i] *= (1.0f / scale);
	}
	bounds.setEmpty();
	for (uint32_t i = 0; i < vertexData.size(); ++i)
	{
		bounds.include(vertexData[i]);
	}
	convexMeshDescr.points.data   = vertexData.data();
	convexMeshDescr.points.stride = sizeof(PxVec3);
	convexMeshDescr.points.count  = (uint32_t)vertexData.size();
	convexMeshDescr.flags         = PxConvexFlag::eCOMPUTE_CONVEX;

	auto cookingParams = PxCookingParams(PxGetPhysics().getTolerancesScale());


#if PX_GPU_BROAD_PHASE
	cookingParams.buildGPUData = true;
#endif

	cookingParams.convexMeshCookingType = PxConvexMeshCookingType::eQUICKHULL;
	cookingParams.gaussMapLimit = 32;
	cookingParams.suppressTriangleMeshRemapTable = false;
	cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
	//cookingParams.meshPreprocessParams = PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;

	resultConvexMesh = PxCreateConvexMesh(cookingParams, convexMeshDescr);

	if (!resultConvexMesh)
	{
		vertexData.clear();
		vertexData.push_back(bounds.minimum);
		vertexData.push_back(PxVec3(bounds.minimum.x, bounds.maximum.y, bounds.minimum.z));
		vertexData.push_back(PxVec3(bounds.maximum.x, bounds.maximum.y, bounds.minimum.z));
		vertexData.push_back(PxVec3(bounds.maximum.x, bounds.minimum.y, bounds.minimum.z));
		vertexData.push_back(PxVec3(bounds.minimum.x, bounds.minimum.y, bounds.maximum.z));
		vertexData.push_back(PxVec3(bounds.minimum.x, bounds.maximum.y, bounds.maximum.z));
		vertexData.push_back(PxVec3(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z));
		vertexData.push_back(PxVec3(bounds.maximum.x, bounds.minimum.y, bounds.maximum.z));
		convexMeshDescr.points.data  = vertexData.data();
		convexMeshDescr.points.count = (uint32_t)vertexData.size();

		resultConvexMesh = PxCreateConvexMesh(cookingParams, convexMeshDescr);
	}
	output->polygonDataCount = resultConvexMesh->getNbPolygons();
	if (output->polygonDataCount)
		output->polygonData = SAFE_ARRAY_NEW(HullPolygon, output->polygonDataCount);
	output->pointsCount  = resultConvexMesh->getNbVertices();
	output->points       = SAFE_ARRAY_NEW(NvcVec3, output->pointsCount);
	int32_t indicesCount = 0;
	PxHullPolygon hPoly;
	for (uint32_t i = 0; i < resultConvexMesh->getNbPolygons(); ++i)
	{
		HullPolygon& pd = output->polygonData[i];
		resultConvexMesh->getPolygonData(i, hPoly);
		pd.indexBase = hPoly.mIndexBase;
		pd.vertexCount   = hPoly.mNbVerts;
		pd.plane[0]  = hPoly.mPlane[0];
		pd.plane[1]  = hPoly.mPlane[1];
		pd.plane[2]  = hPoly.mPlane[2];
		pd.plane[3]  = hPoly.mPlane[3];

		pd.plane[0] /= scale;
		pd.plane[1] /= scale;
		pd.plane[2] /= scale;
		pd.plane[3] -= (pd.plane[0] * bbCenter.x + pd.plane[1] * bbCenter.y + pd.plane[2] * bbCenter.z);
		float length = sqrt(pd.plane[0] * pd.plane[0] + pd.plane[1] * pd.plane[1] + pd.plane[2] * pd.plane[2]);
		pd.plane[0] /= length;
		pd.plane[1] /= length;
		pd.plane[2] /= length;
		pd.plane[3] /= length;
		indicesCount = PxMax(indicesCount, pd.indexBase + pd.vertexCount);
	}
	output->indicesCount = indicesCount;
	output->indices      = SAFE_ARRAY_NEW(uint32_t, indicesCount);
	for (uint32_t i = 0; i < resultConvexMesh->getNbVertices(); ++i)
	{
		PxVec3 p          = resultConvexMesh->getVertices()[i] * scale + bbCenter;
		output->points[i] = fromPxShared(p);
	}
	for (int32_t i = 0; i < indicesCount; ++i)
	{
		output->indices[i] = resultConvexMesh->getIndexBuffer()[i];
	}
	resultConvexMesh->release();
	return output;
}

void ExtPxCollisionBuilderImpl::releaseCollisionHull(Nv::Blast::CollisionHull* ch) const
{
	if (ch)
	{
		SAFE_ARRAY_DELETE(ch->indices);
		SAFE_ARRAY_DELETE(ch->points);
		SAFE_ARRAY_DELETE(ch->polygonData);
		delete ch;
	}
}

physx::PxConvexMesh* ExtPxCollisionBuilderImpl::buildConvexMesh(const CollisionHull& hull)
{
	/* PxCooking::createConvexMesh expects PxHullPolygon input, which matches HullPolygon */
	static_assert(sizeof(PxHullPolygon) == sizeof(HullPolygon), "HullPolygon size mismatch");
	static_assert(offsetof(PxHullPolygon, mPlane) == offsetof(HullPolygon, plane), "HullPolygon layout mismatch");
	static_assert(offsetof(PxHullPolygon, mNbVerts) == offsetof(HullPolygon, vertexCount), "HullPolygon layout mismatch");
	static_assert(offsetof(PxHullPolygon, mIndexBase) == offsetof(HullPolygon, indexBase),
	              "HullPolygon layout mismatch");

	PxConvexMeshDesc convexMeshDescr;
	convexMeshDescr.indices.data   = hull.indices;
	convexMeshDescr.indices.count  = (uint32_t)hull.indicesCount;
	convexMeshDescr.indices.stride = sizeof(uint32_t);

	convexMeshDescr.points.data   = hull.points;
	convexMeshDescr.points.count  = (uint32_t)hull.pointsCount;
	convexMeshDescr.points.stride = sizeof(PxVec3);
	
	convexMeshDescr.polygons.data   = hull.polygonData;
	convexMeshDescr.polygons.count  = (uint32_t)hull.polygonDataCount;
	convexMeshDescr.polygons.stride = sizeof(PxHullPolygon);

	auto cookingParams = PxCookingParams(PxGetPhysics().getTolerancesScale());

#if PX_GPU_BROAD_PHASE
	cookingParams.buildGPUData = true;
#endif

	cookingParams.convexMeshCookingType = PxConvexMeshCookingType::eQUICKHULL;
	cookingParams.gaussMapLimit = 32;
	cookingParams.suppressTriangleMeshRemapTable = false;
	cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
	//cookingParams.meshPreprocessParams = PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;

	return PxCreateConvexMesh(cookingParams, convexMeshDescr);
}

void ExtPxCollisionBuilderImpl::buildPhysicsChunks(uint32_t chunkCount, uint32_t* hullOffsets,
                                              CollisionHull** hulls, ExtPxChunk* physicsChunks,
                                              ExtPxSubchunk* physicsSubchunks)
{
	//Nv::Blast::AuthoringResult& result = *ar;
	//uint32_t chunkCount                = (uint32_t)result.chunkCount;
	//uint32_t* hullOffsets              = result.collisionHullOffset;
	//CollisionHull** hulls               = result.collisionHull;
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		int32_t beg = hullOffsets[i];
		int32_t end = hullOffsets[i + 1];
		for (int32_t subhull = beg; subhull < end; ++subhull)
		{
			physicsSubchunks[subhull].transform = physx::PxTransform(physx::PxIdentity);
			physicsSubchunks[subhull].geometry  = physx::PxConvexMeshGeometry(
                reinterpret_cast<physx::PxConvexMesh*>(buildConvexMesh(*hulls[subhull])));
		}
		physicsChunks[i].isStatic           = false;
		physicsChunks[i].subchunkCount      = static_cast<uint32_t>(end - beg);
		physicsChunks[i].firstSubchunkIndex = beg;
	}
}

void ExtPxCollisionBuilderImpl::release()
{
	NVBLAST_DELETE(this, ExtPxCollisionBuilderImpl);
}

}  // namespace Blast
}  // namespace Nv
