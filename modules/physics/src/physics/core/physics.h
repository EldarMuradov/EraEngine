#pragma once

#include "physics/physx_api.h"
#include "physics/core/physics_types.h"
#include "physics/core/physics_stepper.h"
#include "physics/material.h"

#include <core/memory.h>
#include <core/math.h>

#include <ecs/entity.h>

#include <concurrentqueue/concurrentqueue.h>

namespace era_engine
{
	class EditorScene;
}

namespace era_engine::physics
{
	class BodyComponent;
	class ShapeComponent;

	class ERA_CORE_API PhysicsDescriptor
	{
	public:
		physx::PxBroadPhaseType::Enum broad_phase = physx::PxBroadPhaseType::ePABP;
		bool enable_pvd = true;
		bool enable_tgs_solver = true;
		const char* omni_pvd_path = "out.ovd";
	};

	class Physics final
	{
	public:
		Physics(const PhysicsDescriptor& _descriptor = {});
		~Physics();

		void init_scene();

		physx::PxScene* get_scene() const;
		physx::PxPhysics* get_physics() const;
		ref<PhysicsMaterial> get_default_material() const;

		ref<PhysicsMaterial> create_material(float restitution = 0.6f,
			float static_friction = 0.8f,
			float dynamic_friction = 0.8f);

		physx::PxCudaContextManager* get_cuda_context_manager() const;
		physx::PxCpuDispatcher* get_cpu_dispatcher() const;
		physx::PxTolerancesScale get_tolerance_scale() const;

		bool is_gpu() const;

		void release();

		void start();
		void update(float dt);

		void start_simulation(float dt);
		void end_simulation(float dt);

		void reset_actors_velocity_and_inertia();

		void add_shape_to_entity_data(ShapeComponent* shape);
		void remove_shape_from_entity_data(ShapeComponent* shape);

		void add_actor(BodyComponent* actor, physx::PxRigidActor* physx_actor);
		void remove_actor(BodyComponent* actor);

		void release_scene();

		void explode(const vec3& world_pos, float damage_radius, float explosive_impulse);

	private:
		void sync_transforms();
		void process_simulation_event_callbacks();

	public:
		std::atomic_uint32_t nb_active_actors{};

		std::set<BodyComponent*> actors;

		std::unordered_map<Entity::Handle, std::vector<ShapeComponent*>> colliders_map;
		std::unordered_map<physx::PxRigidActor*, BodyComponent*> actors_map;

		moodycamel::ConcurrentQueue<CollisionHandlingData> collision_queue;
		moodycamel::ConcurrentQueue<CollisionHandlingData> collision_exit_queue;

		SpinLock sync;

	private:
		PhysicsDescriptor descriptor;
		Allocator allocator;

		physx::PxScene* scene = nullptr;

		void* scratch_mem_block = nullptr;

		physx::PxPhysics* physics = nullptr;

		physx::PxPvd* pvd = nullptr;

		ref<PhysicsMaterial> default_material = nullptr;

		physx::PxFoundation* foundation = nullptr;

		physx::PxDefaultCpuDispatcher* dispatcher = nullptr;

		physx::PxCudaContextManager* cuda_context_manager = nullptr;

		PhysicsAllocatorCallback* allocator_callback = nullptr;
		ErrorReporter error_reporter;

		ProfilerCallback profiler_callback;

		FixedStepper* stepper = nullptr;

		SimulationFilterCallback simulation_filter_callback;
		ref<SimulationEventCallback> simulation_event_callback = nullptr;

		std::vector<ref<PhysicsMaterial>> materials;

		physx::PxTolerancesScale tolerance_scale;

		const uint32_t nb_cpu_dispatcher_threads = 4;
		static constexpr uint64 scratch_mem_block_size = MB(32U);

		bool use_stepper = false;

		friend class PhysicsSystem;
	};

	class PhysicsHolder final
	{
		PhysicsHolder() = delete;

	public:
		static inline ref<Physics> physics_ref = nullptr;
	};
}