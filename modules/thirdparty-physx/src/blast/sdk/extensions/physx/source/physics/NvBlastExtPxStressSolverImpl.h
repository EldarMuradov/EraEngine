#ifndef NVBLASTEXTPXSTRESSSOLVERIMPL_H
#define NVBLASTEXTPXSTRESSSOLVERIMPL_H

#include "NvBlastExtPxStressSolver.h"
#include "NvBlastExtPxListener.h"
#include "NvBlastArray.h"
#include "NvBlastHashSet.h"

namespace Nv
{
namespace Blast
{


class ExtPxStressSolverImpl final : public ExtPxStressSolver, ExtPxListener
{
	NV_NOCOPY(ExtPxStressSolverImpl)

public:
	ExtPxStressSolverImpl(ExtPxFamily& family, ExtStressSolverSettings settings);


	//////// ExtPxStressSolver interface ////////

	virtual void							release() override;

	virtual ExtStressSolver&				getSolver() const override
	{
		return *m_solver;
	}

	virtual void							update(bool doDamage) override;


	//////// ExtPxListener interface ////////

	virtual void							onActorCreated(ExtPxFamily& family, ExtPxActor& actor) final;

	virtual void							onActorDestroyed(ExtPxFamily& family, ExtPxActor& actor) final;


private:
	~ExtPxStressSolverImpl();


	//////// data ////////

	ExtPxFamily&				m_family;
	ExtStressSolver*			m_solver;
	HashSet<ExtPxActor*>::type  m_actors;
};


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXSTRESSSOLVERIMPL_H
