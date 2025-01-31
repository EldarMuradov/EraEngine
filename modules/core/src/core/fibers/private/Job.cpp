#include "core/fibers/Job.h"
#include "core/fibers/Counter.h"

namespace era_engine
{
	void JobInfo::Execute()
	{
		if (!IsNull()) 
		{
			GetDelegate()->Call();
		}

		if (m_counter) 
		{
			m_counter->Decrement();
		}
	}
}