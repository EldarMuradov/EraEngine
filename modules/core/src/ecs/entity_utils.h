#pragma once

#include "core_api.h"

#ifndef ENTT_ASSERT
#define ENTT_ASSERT(condition, ...) ASSERT(condition)
#endif // !ENTT_ASSERT

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/helper.hpp>

namespace era_engine
{
	class ERA_CORE_API EntityUtils final
	{
		EntityUtils() = delete;

	public:
		template <typename Context_, typename... Args_>
		inline Context_& create_or_get_context_variable(entt::registry& registry, Args_&&... a)
		{
			auto& c = registry.ctx();
			Context_* context = c.find<Context_>();
			if (!context)
			{
				context = &c.emplace<Context_>(std::forward<Args_>(a)...);
			}

			return *context;
		}

		template <typename Context_>
		inline Context_& get_context_variable(entt::registry& registry)
		{
			auto& c = registry.ctx();
			return *c.find<Context_>();
		}

		template <typename Context_>
		inline Context_* try_get_context_variable(entt::registry& registry)
		{
			auto& c = registry.ctx();
			return c.find<Context_>();
		}

		template <typename Context_>
		inline bool does_context_variable_exist(entt::registry& registry)
		{
			auto& c = registry.ctx();
			return c.contains<Context_>();
		}

		template <typename Context_>
		inline void delete_context_variable(entt::registry& registry)
		{
			auto& c = registry.ctx();
			c.erase<Context_>();
		}
	};
}