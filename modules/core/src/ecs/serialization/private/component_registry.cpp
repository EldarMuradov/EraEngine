#include "ecs/serialization/component_registry.h"
#include "ecs/serialization/serialization_meta.h"

#include "core/log.h"

#include <deque>

namespace era_engine
{
	struct ComponentRegistryData
	{
		std::mutex sync;

		std::deque<ComponentTypeInfo> infos;
		std::unordered_map<rttr::type::type_id, const ComponentTypeInfo*> by_type;
		std::unordered_map<std::string, const ComponentTypeInfo*> by_name;
		std::unordered_map<entt::id_type, const ComponentTypeInfo*> by_storage_id;
	};

	static ComponentRegistryData& get_registry_data()
	{
		static ComponentRegistryData data;
		return data;
	}

	bool ComponentTypeInfo::is_serializable() const
	{
		return add != nullptr && era_engine::is_serializable(type);
	}

	const ComponentTypeInfo* ComponentRegistry::find(const rttr::type& type)
	{
		ComponentRegistryData& data = get_registry_data();
		std::lock_guard _lock{ data.sync };

		const auto iter = data.by_type.find(type.get_id());
		return iter != data.by_type.end() ? iter->second : nullptr;
	}

	const ComponentTypeInfo* ComponentRegistry::find(std::string_view name)
	{
		ComponentRegistryData& data = get_registry_data();
		std::lock_guard _lock{ data.sync };

		const auto iter = data.by_name.find(std::string(name));
		return iter != data.by_name.end() ? iter->second : nullptr;
	}

	const ComponentTypeInfo* ComponentRegistry::find_by_storage_id(entt::id_type storage_id)
	{
		ComponentRegistryData& data = get_registry_data();
		std::lock_guard _lock{ data.sync };

		const auto iter = data.by_storage_id.find(storage_id);
		return iter != data.by_storage_id.end() ? iter->second : nullptr;
	}

	std::vector<const ComponentTypeInfo*> ComponentRegistry::get_all()
	{
		ComponentRegistryData& data = get_registry_data();
		std::lock_guard _lock{ data.sync };

		std::vector<const ComponentTypeInfo*> result;
		result.reserve(data.infos.size());
		for (const ComponentTypeInfo& info : data.infos)
		{
			result.push_back(&info);
		}
		return result;
	}

	void ComponentRegistry::register_type(const ComponentTypeInfo& info)
	{
		ComponentRegistryData& data = get_registry_data();
		std::lock_guard _lock{ data.sync };

		const std::string name = info.type.get_name().to_string();
		if (data.by_type.contains(info.type.get_id()) || data.by_name.contains(name))
		{
			LOG_ERROR("ComponentRegistry> Component %s is registered twice.", name.c_str());
			return;
		}

		const ComponentTypeInfo& stored_info = data.infos.emplace_back(info);
		data.by_type.emplace(info.type.get_id(), &stored_info);
		data.by_name.emplace(name, &stored_info);
		data.by_storage_id.emplace(info.storage_id, &stored_info);
	}
}
