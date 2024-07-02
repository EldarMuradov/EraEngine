#pragma once

#include "px/core/px_physics_engine.h"

namespace era_engine::physics
{
    using namespace physx;

	struct px_multithread_stepper;

    struct px_physics_stepper
    {
        px_physics_stepper() {}

        virtual ~px_physics_stepper() {}

        virtual bool advance(PxScene* scene, PxReal dt, void* scratchBlock, PxU32 scratchBlockSize) = 0;
        virtual void wait(PxScene* scene) = 0;
        virtual void substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize) = 0;

        virtual void setSubStepper(const PxReal stepSize, const PxU32 maxSteps)
        {
        }

        virtual void renderDone()
        {
        }
    };

    struct px_stepper_task : PxLightCpuTask
    {
        void setStepper(px_multithread_stepper* stepper)
        {
            mtstepper = stepper;
        }

        px_multithread_stepper* getStepper()
        {
            return mtstepper;
        }

        const px_multithread_stepper* getStepper() const
        {
            return mtstepper;
        }

        const char* getName() const override
        {
            return "Stepper Task";
        }

        void run() override;

    protected:
        px_multithread_stepper* mtstepper = nullptr;
    };

    struct px_stepper_task_simulate : px_stepper_task
    {
        px_stepper_task_simulate()
        {
        }

        void run() override;
    };

    struct px_multithread_stepper : px_physics_stepper
    {
        px_multithread_stepper()
            : firstCompletionPending(false)
            , physicsScene(nullptr)
            , sync(nullptr)
            , currentSubStep(0)
            , nbSubSteps(0)
        {
            completion0.setStepper(this);
            completion1.setStepper(this);
            simulateTask.setStepper(this);
        }

        ~px_multithread_stepper();

        bool advance(PxScene* scene, PxReal dt, void* scratchBlock, PxU32 blockSize) override;

        virtual void substepDone(px_stepper_task* ownerTask);

        void renderDone() override;

        void wait(PxScene* scene) override
        {
            if (nbSubSteps && sync)
                sync->wait();
        }

        virtual void shutdown();

        virtual void reset() = 0;
        void substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize) override = 0;
        virtual void simulate(PxBaseTask* ownerTask);

        PxReal getSubStepSize() const
        {
            return subStepSize;
        }

    protected:
        void substep(px_stepper_task& completionTask);

        bool firstCompletionPending;
        px_stepper_task_simulate simulateTask;
        px_stepper_task completion0, completion1;
        PxScene* physicsScene;
        PxSync* sync;

        PxU32 currentSubStep;
        PxU32 nbSubSteps;
        PxReal subStepSize;
        void* scratchBlockPtr;
        PxU32 scratchBlockSize;
    };

    struct px_fixed_stepper : px_multithread_stepper
    {
        px_fixed_stepper()
            : px_multithread_stepper()
            , accumulator(0)
            , fixedSubStepSize(0.013f)
            , maxSubSteps(1)
        {
        }

        void setup(const float stepSize, const uint32 maxSubsetps = 1)
        {
            fixedSubStepSize = stepSize;
            maxSubSteps = maxSubsetps;
        }

        void substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize) override;

        void reset() override
        {
            accumulator = 0.0f;
        }

        void setSubStepper(const PxReal stepSize, const PxU32 maxSteps) override
        {
            fixedSubStepSize = stepSize;
            maxSubSteps = maxSteps;
        }

    protected:
        PxReal accumulator;
        PxReal fixedSubStepSize;
        PxU32 maxSubSteps;
    };
}