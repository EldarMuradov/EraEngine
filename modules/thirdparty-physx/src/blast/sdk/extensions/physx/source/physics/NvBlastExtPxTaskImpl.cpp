#include <pch.h>

#include "NvBlastGlobals.h"
#include "NvBlastExtPxTaskImpl.h"

#include "NvBlastTkGroup.h"

using namespace Nv::Blast;


uint32_t ExtGroupTaskManagerImpl::process(uint32_t workerCount)
{
	NVBLAST_CHECK_WARNING(m_group != nullptr, "ExtGroupTaskManager::process cannot process, no group set.", return 0);
	NVBLAST_CHECK_WARNING(m_sync.isDone(), "ExtGroupTaskManager::process group is already being processed.", return 0);

	// at least one task must start, even when dispatcher has none specified
	uint32_t dispatcherThreads = m_taskManager.getCpuDispatcher()->getWorkerCount();
	dispatcherThreads = dispatcherThreads > 0 ? dispatcherThreads : 1;

	// not expecting an arbitrary amount of tasks
	uint32_t availableTasks = TASKS_MAX_COUNT;

	// use workerCount tasks, unless dispatcher has less threads or less tasks are available
	uint32_t requestedTasks = workerCount > 0 ? workerCount : dispatcherThreads;
	requestedTasks = requestedTasks > dispatcherThreads ? dispatcherThreads : requestedTasks;
	requestedTasks = requestedTasks > availableTasks ? availableTasks : requestedTasks;

	// ensure the group has enough memory allocated for concurrent processing
	m_group->setWorkerCount(requestedTasks);

	// check if there is work to do
	uint32_t jobCount = m_group->startProcess();

	if (jobCount)
	{
		// don't start more tasks than jobs are available
		requestedTasks = requestedTasks > jobCount ? jobCount : requestedTasks;

		// common counter for all tasks
		m_counter.reset(jobCount);

		// set to busy state
		m_sync.setCount(requestedTasks);

		// set up tasks
		for (uint32_t i = 0; i < requestedTasks; i++)
		{
			m_tasks[i].setup(m_group, &m_counter, &m_sync);
			m_tasks[i].setContinuation(m_taskManager, nullptr);
			m_tasks[i].removeReference();
		}

		return requestedTasks;
	}

	// there was no work to be done
	return 0;
}


bool ExtGroupTaskManagerImpl::wait(bool block)
{
	if (block && !m_sync.isDone())
	{
		m_sync.wait();
	}
	if (m_sync.isDone())
	{
		return m_group->endProcess();
	}
	return false;
}


void ExtGroupTaskManagerImpl::setGroup(TkGroup* group)
{
	NVBLAST_CHECK_WARNING(m_sync.isDone(), "ExtGroupTaskManager::setGroup trying to change group while processing.", return);

	m_group = group;
}


ExtGroupTaskManager* ExtGroupTaskManager::create(physx::PxTaskManager& taskManager, TkGroup* group)
{
	return NVBLAST_NEW(ExtGroupTaskManagerImpl) (taskManager, group);
}


void ExtGroupTaskManagerImpl::release()
{
	NVBLAST_CHECK_WARNING(m_sync.isDone(), "ExtGroupTaskManager::release group is still being processed.", return);

	NVBLAST_DELETE(this, ExtGroupTaskManagerImpl);
}
