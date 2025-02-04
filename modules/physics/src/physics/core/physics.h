#pragma once

#include "physics/physx_api.h"
#include "physics/core/physics_types.h"
#include "physics/core/physics_stepper.h"

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
	//class SoftBody;

	class Physics final
	{
	public:
		Physics();
		~Physics();

		physx::PxScene* get_scene() const;
		physx::PxPhysics* get_physics() const;
		physx::PxMaterial* get_default_material() const;

		physx::PxCudaContextManager* get_cuda_context_manager() const;
		physx::PxCpuDispatcher* get_cpu_dispatcher() const;
		physx::PxTolerancesScale get_tolerance_scale() const;

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

		void set_editor_scene(EditorScene* _editor_scene);

		void explode(const vec3& world_pos, float damage_radius, float explosive_impulse);

		// Raycasting
		RaycastInfo raycast(const BodyComponent* rb, const vec3& dir, int max_dist = PX_NB_MAX_RAYCAST_DISTANCE, bool hit_triggers = true, uint32_t layer_mask = 0, int max_hits = PX_NB_MAX_RAYCAST_HITS);

		// Checking
		bool check_box(const vec3& center, const vec3& half_extents, const quat& rotation, bool hit_triggers = false, uint32 layer_mask = 0);

		bool check_sphere(const vec3& center, const float radius, bool hit_triggers = false, uint32 layer_mask = 0);

		bool check_capsule(const vec3& center, const float radius, const float half_height, const quat& rotation, bool hit_triggers = false, uint32 layer_mask = 0);

		// Overlapping
		OverlapInfo overlap_capsule(const vec3& center, const float radius, const float half_height, const quat& rotation, bool hit_triggers = false, uint32 layer_mask = 0);

		OverlapInfo overlap_box(const vec3& center, const vec3& half_extents, const quat& rotation, bool hit_triggers = false, uint32 layer_mask = 0);

		OverlapInfo overlap_sphere(const vec3& center, const float radius, bool hit_triggers = false, uint32 layer_mask = 0);

	private:
		void step_physics(float step_size);
		void sync_transforms();
		void process_simulation_event_callbacks();

	public:
		volatile float frame_rate = 60.0f;

		std::atomic_uint32_t nb_active_actors{};

		std::set<BodyComponent*> actors;

		std::unordered_map<Entity::Handle, std::vector<ShapeComponent*>> colliders_map;
		std::unordered_map<physx::PxRigidActor*, BodyComponent*> actors_map;

		moodycamel::ConcurrentQueue<CollisionHandlingData> collision_queue;
		moodycamel::ConcurrentQueue<CollisionHandlingData> collision_exit_queue;

		SpinLock sync;

	private:
		physx::PxScene* scene = nullptr;

		physx::PxPhysics* physics = nullptr;

		physx::PxPvd* pvd = nullptr;

		EditorScene* editor_scene = nullptr;

		physx::PxMaterial* default_material = nullptr;

		physx::PxFoundation* foundation = nullptr;

		physx::PxDefaultCpuDispatcher* dispatcher = nullptr;

		physx::PxCudaContextManager* cuda_context_manager = nullptr;

#if PX_ENABLE_RAYCAST_CCD
		physx::RaycastCCDManager* raycast_ccd = nullptr;
#endif

		PhysicsAllocatorCallback* allocator_callback = nullptr;
		ErrorReporter error_reporter;

		ProfilerCallback profiler_callback;

		FixedStepper* stepper = nullptr;

		SimulationFilterCallback simulation_filter_callback;
		ref<SimulationEventCallback> simulation_event_callback = nullptr;

		QueryFilter queryFilter;

		physx::PxTolerancesScale tolerance_scale;

		const uint32_t nb_cpu_dispatcher_threads = 4;

		Allocator allocator;

		friend class PhysicsSystem;
	};

	class PhysicsHolder final
	{
		PhysicsHolder() = delete;

	public:
		static inline ref<Physics> physics_ref = nullptr;
	};
}