#include "physics/destructions/destruction_system.h"
#include "physics/destructions/destructible_component.h"
#include "physics/core/physics.h"
#include "physics/core/physics_utils.h"
#include "physics/body_component.h"

#include <core/cpu_profiling.h>

#include <ecs/base_components/transform_component.h>
#include <ecs/update_groups.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<DestructionSystem>("DestructionSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))
			.method("update", &DestructionSystem::update)(metadata("update_group", update_types::BEFORE_PHYSICS));
	}

	DestructionSystem::DestructionSystem(World* _world)
		: System(_world)
	{
	}

	void DestructionSystem::init()
	{
		entt::registry& registry = world->get_registry();
		registry.on_construct<DestructibleComponent>().connect<&DestructionSystem::on_destructible_created>(this);
	}

	void DestructionSystem::update(float dt)
	{
		ZoneScopedN("DestructionSystem::update");

		process_added_destructibles(dt);
	}

	void DestructionSystem::process_added_destructibles(float dt)
	{
		using namespace physx;

		ZoneScopedN("DestructionSystem::process_added_destructibles");

		ScopedSpinLock _lock{ sync };

		for (auto iter = destructibles_to_init.begin(); iter != destructibles_to_init.end();)
		{
			Entity entity = world->get_entity(*iter);

			DestructibleComponent* destructibe_component = entity.get_component<DestructibleComponent>();

			if (destructibe_component->is_root)
			{
				void* mem = malloc(NvBlastAssetGetFamilyMemorySize(destructibe_component->blast_asset->blast_asset, nullptr));

				NvBlastFamily* family = NvBlastAssetCreateFamily(mem, destructibe_component->blast_asset->blast_asset, nullptr);
				destructibe_component->family = family;

				std::vector<float> bonds_healths;
				bonds_healths.resize(NvBlastAssetGetBondCount(destructibe_component->blast_asset->blast_asset, nullptr), destructibe_component->health);

				std::vector<float> support_chunks_healths;
				support_chunks_healths.resize(NvBlastAssetGetSupportChunkCount(destructibe_component->blast_asset->blast_asset, nullptr), destructibe_component->health);

				NvBlastActorDesc actor_desc{};
				actor_desc.initialBondHealths = bonds_healths.data();
				actor_desc.initialSupportChunkHealths = support_chunks_healths.data();

				// Provide scratch memory
				std::vector<char> scratch(NvBlastFamilyGetRequiredScratchForCreateFirstActor(family, nullptr));

				// Create the first actor
				NvBlastActor* actor = NvBlastFamilyCreateFirstActor(family, &actor_desc, scratch.data(), nullptr);
				ASSERT(actor != nullptr);

				destructibe_component->blast_actor = actor;
			}
			// else assume actor is created via fracturing

			PxRigidActor* rigid_actor = PhysicsUtils::get_body_component(entity)->get_rigid_actor();
			if (rigid_actor == nullptr)
			{
				continue;
			}

			iter = destructibles_to_init.erase(iter);
		}
	}

	void DestructionSystem::on_destructible_created(entt::registry& registry, entt::entity entity_handle)
	{
		ScopedSpinLock _lock{ sync };

		destructibles_to_init.push_back(static_cast<Entity::Handle>(entity_handle));
	}
}