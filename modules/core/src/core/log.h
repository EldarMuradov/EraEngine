// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

namespace era_engine
{
	extern bool log_window_open;

	enum MessageType : uint8_t
	{
		message_type_normal,
		message_type_warning,
		message_type_error,

		message_type_count,
	};
}

#if ENABLE_MESSAGE_LOG

#if LOG_LEVEL_PROFILE
#define LOG_MESSAGE(message, ...) log_message_internal(message_type_normal, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_WARNING(message, ...) log_message_internal(message_type_warning, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_ERROR(message, ...) log_message_internal(message_type_error, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#else
#define LOG_MESSAGE(message, ...) log_message(message_type_normal, message, __VA_ARGS__)
#define LOG_WARNING(message, ...) log_message(message_type_warning, message, __VA_ARGS__)
#define LOG_ERROR(message, ...) log_message(message_type_error, message, __VA_ARGS__)
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