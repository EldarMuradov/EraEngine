#include <pch.h>
#include <px/core/px_stepper.h>
#include <core/cpu_profiling.h>

namespace era_engine::physics
{
    void px_stepper_task::run()
    {
        mtstepper->substepDone(this);
        release();
    }

    void px_stepper_task_simulate::run()
    {
        mtstepper->simulate(mCont);
    }

    px_multithread_stepper::~px_multithread_stepper()
    {
        RELEASE_PTR(sync)
    }

    bool px_multithread_stepper::advance(PxScene* scene, PxReal dt, void* scratchBlock, PxU32 blockSize)
    {
        scratchBlockPtr = scratchBlock;
        scratchBlockSize = blockSize;

        if (!sync)
            sync = new PxSync();

        substepStrategy(dt, nbSubSteps, subStepSize);

        if (nbSubSteps == 0)
            return false;

        physicsScene = scene;

        sync->reset();

        currentSubStep = 1;

        completion0.setContinuation(*physicsScene->getTaskManager(), nullptr);

        substep(completion0);
        firstCompletionPending = true;

        return true;
    }

    void px_multithread_stepper::substepDone(px_stepper_task* ownerTask)
    {
        {
#ifndef PX_PROFILE
            PxSceneWriteLock writeLock(*mScene);
#endif
            physicsScene->fetchResults(true);
        }

        if (currentSubStep >= nbSubSteps)
        {
            sync->set();
        }
        else
        {
            px_stepper_task& s = ownerTask == &completion0 ? completion1 : completion0;
            s.setContinuation(*physicsScene->getTaskManager(), nullptr);
            currentSubStep++;

            substep(s);

            // After the first substep, completions run freely
            s.removeReference();
        }
    }

    void px_multithread_stepper::renderDone()
    {
        if (firstCompletionPending)
        {
            completion0.removeReference();
            firstCompletionPending = false;
        }
    }

    void px_multithread_stepper::shutdown()
    {
        RELEASE_PTR(sync)
    }

    void px_multithread_stepper::simulate(PxBaseTask* ownerTask)
    {
        CPU_PROFILE_BLOCK("Physics.Simulate");

        PxSceneWriteLock writeLock(*physicsScene);

        physicsScene->simulate(subStepSize, ownerTask, scratchBlockPtr, scratchBlockSize);
    }

    void px_multithread_stepper::substep(px_stepper_task& completionTask)
    {
        // step
        {
            simulateTask.setContinuation(&completionTask);
            simulateTask.removeReference();
        }
    }

    void px_fixed_stepper::substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize)
    {
        if (accumulator > fixedSubStepSize)
            accumulator = 0.0f;

        accumulator += stepSize;
        if (accumulator < fixedSubStepSize)
        {
            substepCount = 0;
            return;
        }

        substepSize = fixedSubStepSize;
        substepCount = PxMin(PxU32(accumulator / fixedSubStepSize), maxSubSteps);

        accumulator -= PxReal(substepCount) * substepSize;
    }
}