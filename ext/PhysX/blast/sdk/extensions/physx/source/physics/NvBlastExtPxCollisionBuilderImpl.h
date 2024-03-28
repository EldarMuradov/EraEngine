#ifndef NVBLASTEXTPXCOLLISIONBUILDERIMPL_H
#define NVBLASTEXTPXCOLLISIONBUILDERIMPL_H

#include "NvBlastExtPxCollisionBuilder.h"
#include "NvBlastExtAuthoringTypes.h"

namespace physx
{
	class PxCooking;
	class PxPhysicsInsertionCallback;
}
namespace Nv
{
	namespace Blast
	{

		struct CollisionHullImpl : public CollisionHull
		{
			CollisionHullImpl() {};
			CollisionHullImpl(const CollisionHull& hullToCopy);
			~CollisionHullImpl();
		};

		class ExtPxCollisionBuilderImpl : public ExtPxCollisionBuilder
		{
		public:
			ExtPxCollisionBuilderImpl() {}
	        virtual ~ExtPxCollisionBuilderImpl() {};
			void release() override;
			CollisionHull* buildCollisionGeometry(uint32_t verticesCount, const NvcVec3* vertexData) override;
			void releaseCollisionHull(CollisionHull* hull) const override;

			physx::PxConvexMesh* buildConvexMesh(const CollisionHull& hull) override;
			void buildPhysicsChunks(uint32_t chunkCount, uint32_t* hullOffsets, CollisionHull** hulls,
				ExtPxChunk* physicsChunks, ExtPxSubchunk* physicsSubchunks) override;
		};

	}  // namespace Blast
}  // namespace Nv


#endif  // ifndef NVBLASTEXTPXCOLLISIONBUILDERIMPL_H
