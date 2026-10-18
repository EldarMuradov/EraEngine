// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

namespace era_engine
{
    extern bool log_window_open;

    enum MessageType : uint8_t
    {
        MESSAGE_TYPE_NORMAL,
        MESSAGE_TYPE_WARNING,
        MESSAGE_TYPE_ERROR,

        MESSAGE_TYPE_COUNT,
    };
}

#if ENABLE_MESSAGE_LOG

#if LOG_LEVEL_PROFILE
#define LOG_MESSAGE(message, ...) log_message_internal(MESSAGE_TYPE_NORMAL, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_WARNING(message, ...) log_message_internal(MESSAGE_TYPE_WARNING, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_ERROR(message, ...) log_message_internal(MESSAGE_TYPE_ERROR, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#else
#define LOG_MESSAGE(message, ...) log_message(MESSAGE_TYPE_NORMAL, message, __VA_ARGS__)
#define LOG_WARNING(message, ...) log_message(MESSAGE_TYPE_WARNING, message, __VA_ARGS__)
#define LOG_ERROR(message, ...) log_message(MESSAGE_TYPE_ERROR, message, __VA_ARGS__)
#endif

#include "core_api.h"

#include "ecs/system.h"

namespace era_engine
{
    ERA_CORE_API void log_message_internal(MessageType type, const char* file, const char* function, uint32 line, const char* format, ...);
    ERA_CORE_API void log_message(MessageType type, const char* format, ...);

    class ERA_CORE_API LogSystem final : public System
    {
    public:
        LogSystem(World* _world);
        ~LogSystem();

        void init() override;
        void update(float dt) override;

        ERA_VIRTUAL_REFLECT(System)
    };
}

#else

#define LOG_MESSAGE(...)
#define LOG_WARNING(...)
#define LOG_ERROR(...)

#endif