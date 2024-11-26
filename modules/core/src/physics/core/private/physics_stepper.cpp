#include "physics/core/physics_stepper.h"

#include "core/cpu_profiling.h"

namespace era_engine::physics
{

	PhysicsStepper::PhysicsStepper()
	{
	}

	PhysicsStepper::~PhysicsStepper()
	{
	}

	void PhysicsStepper::setSubStepper(const physx::PxReal step_size, const physx::PxU32 max_steps)
	{
	}

	void PhysicsStepper::renderDone()
	{
	}

	StepperTask::~StepperTask()
	{
	}

	void StepperTask::setStepper(MultithreadStepper* stepper)
	{
		mtstepper = stepper;
	}

	MultithreadStepper* StepperTask::getStepper()
	{
		return mtstepper;
	}

	const MultithreadStepper* StepperTask::getStepper() const
	{
		return mtstepper;
	}

	const char* StepperTask::getName() const
	{
		return "Stepper Task";
	}

	void StepperTask::run()
	{
		mtstepper->subStepDone(this);
		release();
	}

	StepperTaskSimulate::StepperTaskSimulate()
	{
	}

	StepperTaskSimulate::~StepperTaskSimulate()
	{
	}

	void StepperTaskSimulate::run()
	{
		mtstepper->simulate(mCont);
	}

	MultithreadStepper::MultithreadStepper()
		: first_completion_pending(false)
		, physics_scene(nullptr)
		, sync(nullptr)
		, current_sub_step(0)
		, nb_sub_steps(0)
	{
		completion0.setStepper(this);
		completion1.setStepper(this);
		simulate_task.setStepper(this);
	}

	MultithreadStepper::~MultithreadStepper()
	{
		RELEASE_PTR(sync)
	}

	bool MultithreadStepper::advance(physx::PxScene* scene, physx::PxReal dt, void* scratch_block, physx::PxU32 block_size)
	{
		using namespace physx;
		scratch_block_ptr = scratch_block;
		scratch_block_size = block_size;

		if (!sync)
		{
			sync = new PxSync();
		}

		subStepStrategy(dt, nb_sub_steps, sub_step_size);

		if (nb_sub_steps == 0)
		{
			return false;
		}

		physics_scene = scene;

		sync->reset();

		current_sub_step = 1;

		completion0.setContinuation(*physics_scene->getTaskManager(), nullptr);

		substep(completion0);
		first_completion_pending = true;

		return true;
	}

	void MultithreadStepper::subStepDone(StepperTask* owner_task)
	{
		using namespace physx;
		{
			PxSceneWriteLock write_lock(*physics_scene);
			physics_scene->fetchResults(true);
		}

		if (current_sub_step >= nb_sub_steps)
		{
			sync->set();
		}
		else
		{
			StepperTask& s = owner_task == &completion0 ? completion1 : completion0;
			s.setContinuation(*physics_scene->getTaskManager(), nullptr);
			current_sub_step++;

			substep(s);

			// After the first substep, completions run freely
			s.removeReference();
		}
	}

	void MultithreadStepper::renderDone()
	{
		if (first_completion_pending)
		{
			completion0.removeReference();
			first_completion_pending = false;
		}
	}

	void MultithreadStepper::wait(physx::PxScene* scene)
	{
		if (nb_sub_steps && sync)
		{
			sync->wait();
		}
	}

	void MultithreadStepper::shutdown()
	{
		RELEASE_PTR(sync)
	}

	void MultithreadStepper::simulate(physx::PxBaseTask* owner_task)
	{
		CPU_PROFILE_BLOCK("Physics.Simulate");

		physx::PxSceneWriteLock write_lock(*physics_scene);

		physics_scene->simulate(sub_step_size, owner_task, scratch_block_ptr, scratch_block_size);
	}

	physx::PxReal MultithreadStepper::getSubStepSize() const
	{
		return sub_step_size;
	}

	void MultithreadStepper::substep(StepperTask& completion_task)
	{
		simulate_task.setContinuation(&completion_task);
		simulate_task.removeReference();
	}

	FixedStepper::FixedStepper()
		: MultithreadStepper()
		, accumulator(0)
		, fixed_sub_step_size(0.013f)
		, max_sub_steps(1)
	{
	}

	FixedStepper::~FixedStepper()
	{
	}

	void FixedStepper::setup(const float _step_size, const uint32 _max_sub_steps)
	{
		fixed_sub_step_size = _step_size;
		max_sub_steps = _max_sub_steps;
	}

	void FixedStepper::subStepStrategy(const physx::PxReal step_size, physx::PxU32& substep_count, physx::PxReal& substep_size)
	{
		using namespace physx;
		if (accumulator > fixed_sub_step_size)
		{
			accumulator = 0.0f;
		}

		accumulator += step_size;
		if (accumulator < fixed_sub_step_size)
		{
			substep_count = 0;
			return;
		}

		substep_size = fixed_sub_step_size;
		substep_count = PxMin(PxU32(accumulator / fixed_sub_step_size), max_sub_steps);

		accumulator -= PxReal(substep_count) * substep_size;
	}

	void FixedStepper::reset()
	{
		accumulator = 0.0f;
	}

	void FixedStepper::setSubStepper(const physx::PxReal _step_size, const physx::PxU32 _max_steps)
	{
		fixed_sub_step_size = _step_size;
		max_sub_steps = _max_steps;
	}

}