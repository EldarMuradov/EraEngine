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
        static Context_& create_or_get_context_variable(entt::registry& registry, Args_&&... a)
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
        static Context_& get_context_variable(entt::registry& registry)
        {
            auto& c = registry.ctx();
            return *c.find<Context_>();
        }

        template <typename Context_>
        static Context_* try_get_context_variable(entt::registry& registry)
        {
            auto& c = registry.ctx();
            return c.find<Context_>();
        }

        template <typename Context_>
        static bool does_context_variable_exist(entt::registry& registry)
        {
            auto& c = registry.ctx();
            return c.contains<Context_>();
        }

        template <typename Context_>
        static void delete_context_variable(entt::registry& registry)
        {
            auto& c = registry.ctx();
            c.erase<Context_>();
        }
    };
}