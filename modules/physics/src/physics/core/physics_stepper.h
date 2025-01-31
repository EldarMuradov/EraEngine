#pragma once

#include "physics/physx_api.h"

namespace era_engine::physics
{
    class MultithreadStepper;

    class PhysicsStepper
    {
    public:
        PhysicsStepper();
        virtual ~PhysicsStepper();

        virtual bool advance(physx::PxScene* scene, physx::PxReal dt, void* scratch_block, physx::PxU32 scratch_block_size) = 0;
        virtual void wait(physx::PxScene* scene) = 0;
        virtual void subStepStrategy(const physx::PxReal step_size, physx::PxU32& substep_count, physx::PxReal& substep_size) = 0;

        virtual void setSubStepper(const physx::PxReal step_size, const physx::PxU32 max_steps);

        virtual void renderDone();
    };

    class StepperTask : public physx::PxLightCpuTask
    {
    public:
        virtual ~StepperTask();

        void setStepper(MultithreadStepper* stepper);

        MultithreadStepper* getStepper();
        const MultithreadStepper* getStepper() const;

        // physx::PxLightCpuTask API
        const char* getName() const override;
        void run() override;

    protected:
        MultithreadStepper* mtstepper = nullptr;
    };

    class StepperTaskSimulate : public StepperTask
    {
    public:
        StepperTaskSimulate();
        virtual ~StepperTaskSimulate();

        void run() override;
    };

    class MultithreadStepper : public PhysicsStepper
    {
    public:
        MultithreadStepper();

        virtual ~MultithreadStepper();

        bool advance(physx::PxScene* scene, physx::PxReal dt, void* scratch_block, physx::PxU32 block_size) override;

        virtual void subStepDone(StepperTask* owner_task);

        void renderDone() override;

        void wait(physx::PxScene* scene) override;

        virtual void shutdown();

        virtual void reset() = 0;
        void subStepStrategy(const physx::PxReal step_size, physx::PxU32& substep_count, physx::PxReal& substep_size) override = 0;
        virtual void simulate(physx::PxBaseTask* owner_task);

        physx::PxReal getSubStepSize() const;

    protected:
        void substep(StepperTask& completion_task);

        StepperTaskSimulate simulate_task;
        StepperTask completion0, completion1;
        physx::PxScene* physics_scene;
        physx::PxSync* sync;

        physx::PxU32 current_sub_step;
        physx::PxU32 nb_sub_steps;

        void* scratch_block_ptr;
        physx::PxU32 scratch_block_size;
        physx::PxReal sub_step_size;

        bool first_completion_pending;
    };

    class FixedStepper : public MultithreadStepper
    {
    public:
        FixedStepper();
        virtual ~FixedStepper();

        void setup(const float _step_size, const uint32 _max_sub_steps = 1);

        void subStepStrategy(const physx::PxReal step_size, physx::PxU32& substep_count, physx::PxReal& substep_size) override;

        void reset() override;

        void setSubStepper(const physx::PxReal _step_size, const physx::PxU32 _max_steps) override;

    protected:
        physx::PxReal accumulator;
        physx::PxReal fixed_sub_step_size;
        physx::PxU32 max_sub_steps;
    };
}