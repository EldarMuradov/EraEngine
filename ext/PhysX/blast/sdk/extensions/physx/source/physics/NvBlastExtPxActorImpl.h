#ifndef NVBLASTEXTPXACTORIMPL_H
#define NVBLASTEXTPXACTORIMPL_H

#include "NvBlastExtPxActor.h"
#include "NvBlastArray.h"
#include "foundation/PxTransform.h"


using namespace physx;

namespace Nv
{
namespace Blast
{


// Forward declarations
class ExtPxFamilyImpl;

struct PxActorCreateInfo
{
	PxTransform	m_transform;
	PxVec3		m_scale;
	PxVec3		m_parentLinearVelocity;
	PxVec3		m_parentAngularVelocity;
	PxVec3		m_parentCOM;
};


class ExtPxActorImpl final : public ExtPxActor
{
public:
	//////// ctor ////////

	ExtPxActorImpl(ExtPxFamilyImpl* family, TkActor* tkActor, const PxActorCreateInfo& pxActorInfo);

	~ExtPxActorImpl()
	{
		release();
	}

	void release();


	//////// interface ////////

	virtual uint32_t					getChunkCount() const override
	{
		return static_cast<uint32_t>(m_chunkIndices.size());
	}

	virtual const uint32_t*				getChunkIndices() const override
	{
		return m_chunkIndices.begin();
	}

	virtual PxRigidDynamic&				getPhysXActor() const override
	{
		return *m_rigidDynamic;
	}

	virtual TkActor&					getTkActor() const override
	{
		return *m_tkActor;
	}

	virtual ExtPxFamily&				getFamily() const override;


private:
	//////// data ////////

	ExtPxFamilyImpl*					m_family;
	TkActor*							m_tkActor;
	PxRigidDynamic*						m_rigidDynamic;
	InlineArray<uint32_t, 4>::type		m_chunkIndices;
};



} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXACTORIMPL_H
