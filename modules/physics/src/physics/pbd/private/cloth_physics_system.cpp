#include "physics/pbd/private/cloth_physics_system.h"
#include "physics/core/physics.h"
#include "physics/core/physics_utils.h"
#include "physics/body_component.h"
#include "physics/pbd/cloth_physics_component.h"
#include "physics/pbd/deformable_surface.h"

#include <rendering/ecs/renderer_holder_root_component.h>
#include <rendering/debug_visualization.h>

#include <core/cpu_profiling.h>
#include <core/debug/debug_var.h>

#include <ecs/base_components/transform_component.h>
#include <ecs/rendering/cloth_render_component.h>
#include <ecs/update_groups.h>

#include <gpu/PxPhysicsGpu.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::physics
{
	constexpr static uint32 MAX_CLOTHES = 32;
	constexpr static physx::PxReal thickness = 0.075f;
	constexpr static physx::PxReal bending_stiffness = 0.00001f;

	static DebugVar<bool> visualize_cloth_physics = DebugVar<bool>("physics.visualize_cloth_physics", false);

	class PostSolveCallback : public BasePostSolveCallback, public physx::PxUserAllocated
	{
	public:
		CUstream skinning_stream;
		physx::PxCudaContextManager* context_manager;
		physx::PxDeformableSkinning* skinning;
		physx::PxArray<SurfaceSkinningHelper> skinning_helpers;
		physx::HostAndDeviceBuffer<physx::PxTrimeshSkinningGpuData> packaged_skinning_data;

		PostSolveCallback(physx::PxCudaContextManager* _context_manager, physx::PxU32 maxNumCloths) :
			context_manager(_context_manager)
		{
			const physx::PxU32 CU_STREAM_NON_BLOCKING = 0x1;
			context_manager->getCudaContext()->streamCreate(&skinning_stream, CU_STREAM_NON_BLOCKING);

			skinning = PxGetPhysicsGpu()->createDeformableSkinning(context_manager);

			packaged_skinning_data.initialize(context_manager, maxNumCloths);
			skinning_helpers.reserve(maxNumCloths);
		}

		void setCloth(physx::PxDeformableSurface* deformableSurface, physx::PxVec3* skinnedPointsRestPosition, physx::PxU32 nbSkinnedPoints)
		{
			skinning_helpers.pushBack(SurfaceSkinningHelper(context_manager, deformableSurface, skinnedPointsRestPosition, nbSkinnedPoints));
		}

		virtual void onPostSolve(CUevent startEvent)
		{
			context_manager->getCudaContext()->streamWaitEvent(skinning_stream, startEvent);

			for (physx::PxU32 i = 0; i < skinning_helpers.size(); ++i)
			{
				skinning_helpers[i].package_gpu_data(packaged_skinning_data.host_data[i]);
			}
			packaged_skinning_data.copy_host_to_device_async(skinning_stream, skinning_helpers.size());

			skinning->computeNormalVectors(packaged_skinning_data.device_data, skinning_helpers.size(), skinning_stream);
			skinning->evaluateVerticesEmbeddedIntoSurface(packaged_skinning_data.device_data, skinning_helpers.size(), skinning_stream);

			context_manager->getCudaContext()->eventRecord(startEvent, skinning_stream);
		}

		virtual void synchronize()
		{
			context_manager->getCudaContext()->streamSynchronize(skinning_stream);
		}

		virtual physx::PxVec3* getSkinnedVertices(physx::PxU32 clothIndex)
		{
			return skinning_helpers[clothIndex].skinned_vertices.host_data;
		}

		~PostSolveCallback()
		{
			context_manager->getCudaContext()->streamDestroy(skinning_stream);
			for (physx::PxU32 i = 0; i < skinning_helpers.size(); ++i)
			{
				skinning_helpers[i].release();
			}

			PX_DELETE(skinning);
		}
	};

	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<ClothPhysicsSystem>("ClothPhysicsSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))
			.method("update", &ClothPhysicsSystem::update)(metadata("update_group", update_types::BEFORE_PHYSICS))
			.method("update_after_simulation", &ClothPhysicsSystem::update_after_simulation)(metadata("update_group", update_types::AFTER_PHYSICS))
			.method("debug_draw", &ClothPhysicsSystem::debug_draw)(metadata("update_group", update_types::RENDER), metadata("After", std::vector<std::string>{"PhysicsVisualizationSystem::update"}));
	}

	ClothPhysicsSystem::ClothPhysicsSystem(World* _world)
		: System(_world)
	{
	}

	ClothPhysicsSystem::~ClothPhysicsSystem()
	{
		if (post_solve_callback != nullptr)
		{
			delete post_solve_callback;
			post_solve_callback = nullptr;
		}

		PX_RELEASE(cloth_material)
	}

	void ClothPhysicsSystem::init()
	{
		entt::registry& registry = world->get_registry();
		registry.on_construct<ClothPhysicsComponent>().connect<&ClothPhysicsSystem::on_cloth_created>(this);
		registry.on_destroy<ClothPhysicsComponent>().connect<&ClothPhysicsSystem::on_cloth_released>(this);

		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);

		post_solve_callback = PX_NEW(PostSolveCallback)(PhysicsEngine::get_physics_core()->get_cuda_context_manager(), physx::PxU32(MAX_CLOTHES));

		PhysicsEngine::execute_write([&]() {
			PhysicsEngine::get_physics_core()->get_scene()->setDeformableSurfaceGpuPostSolveCallback(post_solve_callback);
			});

		cloth_material = PhysicsEngine::get_physics_core()->get_physics()->createDeformableSurfaceMaterial(1.e8f, 0.3f, 0.5f, thickness, bending_stiffness);
	}

	void ClothPhysicsSystem::update(float dt)
	{
		using namespace physx;

		ZoneScopedN("ClothPhysicsSystem::update");

		process_added_clothes(dt);
	}

	void ClothPhysicsSystem::update_after_simulation(float dt)
	{
		using namespace physx;

		ZoneScopedN("ClothPhysicsSystem::update_after_simulation");

		PxCudaContextManager* cuda_context_manager = PhysicsEngine::get_physics_core()->get_cuda_context_manager();

		for (auto&& [entity_handle, transform_component, cloth_component] : world->group(components_group<TransformComponent, ClothPhysicsComponent>).each())
		{
			cloth_component.surface->copy_deformed_vertices_from_gpu();
			cloth_component.sync_positions_buffers_locked();
		}
	}

	void ClothPhysicsSystem::debug_draw(float dt)
	{
		using namespace physx;

		if (!visualize_cloth_physics)
		{
			return;
		}

		for (auto&& [entity_handle, transform_component, cloth_component] : world->group(components_group<TransformComponent, ClothPhysicsComponent>).each())
		{
			const uint32 num_particles = cloth_component.num_x * cloth_component.num_z;

			for (size_t i = 0; i < num_particles; i++)
			{
				PxVec4 p_i = (PxVec4)cloth_component.native_pos_buffer[i];
				vec3 pos_i = vec3(p_i.x, p_i.y, p_i.z);
				render_point(pos_i, vec4(0.107f, 1.0f, 0.0f, 1.0f), renderer_holder_rc->ldr_render_pass, false);
			}
		}
	}

	void ClothPhysicsSystem::process_added_clothes(float dt)
	{
		using namespace physx;

		ZoneScopedN("ClothPhysicsSystem::process_added_clothes");

		ScopedSpinLock _lock{ sync };

		PxCudaContextManager* cuda_context_manager = PhysicsEngine::get_physics_core()->get_cuda_context_manager();

		for (auto iter = clothes_to_init.begin(); iter != clothes_to_init.end();)
		{
			Entity entity = world->get_entity(*iter);

			ClothPhysicsComponent* cloth_component = entity.get_component<ClothPhysicsComponent>();

			TransformComponent* transform_component = entity.get_component<TransformComponent>();

			ClothRenderComponent* render_component = entity.add_component<ClothRenderComponent>();
			render_component->get_data_internal = std::bind(&ClothPhysicsComponent::get_render_data, cloth_component, std::placeholders::_1);

			// Test cloth data.
			{
				PxCookingParams params(PhysicsEngine::get_physics_core()->get_tolerance_scale());
				params.meshWeldTolerance = 0.001f;
				params.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES);
				params.buildTriangleAdjacencies = false;
				params.buildGPUData = true;
				params.midphaseDesc = PxMeshMidPhase::eBVH34;
				params.meshPreprocessParams |= PxMeshPreprocessingFlag::eENABLE_VERT_MAPPING;

				PxReal size = 5.0f;

				const vec3& position = transform_component->get_world_transform().position;

				DeformableSurface deformable_surface = add_quad_deformable_surface(*PhysicsEngine::get_physics_core()->get_physics(), 
					params, 
					PxTransform(PxVec3(-0.5f * size, 0.0f, -0.5f * size) + create_PxVec3(position)),
					cloth_component->num_x,
					cloth_component->num_z, 
					size, 
					size, 
					&cloth_material, 
					1, 
					cuda_context_manager, 
					thickness, 
					cloth_component->density);

				deformable_surface.get_deformable_surface()->setSelfCollisionFilterDistance(thickness * 2.5f);
				deformable_surface.get_deformable_surface()->setLinearDamping(0.f);
				deformable_surface.get_deformable_surface()->setDeformableBodyFlag(PxDeformableBodyFlag::eDISABLE_SELF_COLLISION, false);
				deformable_surface.get_deformable_surface()->setMaxLinearVelocity(1000.0f);

				PxU32 collisionPairUpdateFrequency = 1;
				PxU32 nbCollisionSubsteps = 1;
				deformable_surface.get_deformable_surface()->setNbCollisionPairUpdatesPerTimestep(collisionPairUpdateFrequency);
				deformable_surface.get_deformable_surface()->setNbCollisionSubsteps(nbCollisionSubsteps);

				PxFilterData filter_data;
				filter_data.word0 = -1; // word0 = own ID
				filter_data.word1 = -1;  // word1 = ID mask to filter pairs that trigger a contact callback
				deformable_surface.get_deformable_surface()->getShape()->setSimulationFilterData(filter_data);

				cloth_component->surface = std::make_unique<DeformableSurface>(std::move(deformable_surface));

				const uint32 num_particles = cloth_component->num_x * cloth_component->num_z;
				uint32 size_in_bytes = num_particles * sizeof(PxVec4);
				cloth_component->allocator->initialize(0, (size_in_bytes + sizeof(PxVec4)) * 8.0f);
				cloth_component->native_pos_buffer = cloth_component->allocator->allocate<PxVec4>(num_particles, true);
				cloth_component->view_pos_buffer = cloth_component->allocator->allocate<vec3>(num_particles, true);
			}
			post_solve_callback->setCloth(cloth_component->surface->get_deformable_surface(), &cloth_component->surface->get_skinned_mesh_for_write().vertices[0], PxU32(cloth_component->surface->get_skinned_mesh().vertices.size()));

			iter = clothes_to_init.erase(iter);
		}
	}

	void ClothPhysicsSystem::on_cloth_created(entt::registry& registry, entt::entity entity_handle)
	{
		ScopedSpinLock _lock{ sync };

		clothes_to_init.push_back(static_cast<Entity::Handle>(entity_handle));
	}

	void ClothPhysicsSystem::on_cloth_released(entt::registry& registry, entt::entity entity_handle)
	{
		ScopedSpinLock _lock{ sync };

		Entity entity = world->get_entity(entity_handle);

		ClothPhysicsComponent* cloth_physics_component = entity.get_component<ClothPhysicsComponent>();

		if (cloth_physics_component != nullptr)
		{
			cloth_physics_component->allocator->reset(true);
		}
	}
}