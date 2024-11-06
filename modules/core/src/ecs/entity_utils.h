#pragma once

#ifndef ENTT_ASSERT
#define ENTT_ASSERT(condition, ...) ASSERT(condition)
#endif // !ENTT_ASSERT

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/helper.hpp>

namespace era_engine
{
	class EntityUtils final
	{
		EntityUtils() = delete;

	public:
		template <typename Context_, typename... Args_>
		inline Context_& createOrGetContextVariable(entt::registry& registry, Args_&&... a)
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
		inline Context_& getContextVariable(entt::registry& registry)
		{
			auto& c = registry.ctx();
			return *c.find<Context_>();
		}

		template <typename Context_>
		inline Context_* tryGetContextVariable(entt::registry& registry)
		{
			auto& c = registry.ctx();
			return c.find<Context_>();
		}

		template <typename Context_>
		inline bool doesContextVariableExist(entt::registry& registry)
		{
			auto& c = registry.ctx();
			return c.contains<Context_>();
		}

		template <typename Context_>
		inline void deleteContextVariable(entt::registry& registry)
		{
			auto& c = registry.ctx();
			c.erase<Context_>();
		}
	};
}