#pragma once

#include "core_api.h"

#include "core/math.h"

#include "ecs/entity.h"

#include <functional>

namespace era_engine
{
	class World;
	class PrefabAsset;

	struct PrefabInstantiateParams
	{
		Entity parent;
		trs transform = trs::identity;
	};

	using PrefabSpawnCallback = std::function<void(const std::vector<Entity>& top_level_entities)>;

	// ECS functions must be called from the main thread, only loading runs in background.
	class ERA_CORE_API PrefabUtils final
	{
		PrefabUtils() = delete;

	public:
		static ref<PrefabAsset> load(const fs::path& path);
		static ref<PrefabAsset> load_async(const fs::path& path);
		static ref<PrefabAsset> create_from_string(std::string_view text);

		static std::vector<Entity> instantiate(World* world, const ref<PrefabAsset>& prefab, const PrefabInstantiateParams& params = {});
		static std::vector<Entity> instantiate(World* world, const fs::path& path, const PrefabInstantiateParams& params = {});

		static void instantiate_async(World* world, const fs::path& path, const PrefabInstantiateParams& params = {}, PrefabSpawnCallback callback = {});
		static void instantiate_async(World* world, const ref<PrefabAsset>& prefab, const PrefabInstantiateParams& params = {}, PrefabSpawnCallback callback = {});

		static void process_spawn_queue(World* world);
		static size_t get_pending_spawn_count(World* world);

		static std::vector<Entity> get_instance_roots(const Entity& instance_entity);

		static bool apply_instance(const Entity& instance_entity);
		static std::vector<Entity> revert_instance(const Entity& instance_entity);
		static void unpack_instance(const Entity& instance_entity);

		static std::vector<Entity> duplicate(const std::vector<Entity>& entities);
	};
}
