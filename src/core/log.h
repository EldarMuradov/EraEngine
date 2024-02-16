#pragma once

extern bool logWindowOpen;

enum message_type
{
	message_type_normal,
	message_type_warning,
	message_type_error,

	message_type_count,
};

#if ENABLE_MESSAGE_LOG

#if LOG_LEVEL_PROFILE
#define LOG_MESSAGE(message, ...) logMessageInternal(message_type_normal, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_WARNING(message, ...) logMessageInternal(message_type_warning, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_ERROR(message, ...) logMessageInternal(message_type_error, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#else
#define LOG_MESSAGE(message, ...) logMessage(message_type_normal, message, __VA_ARGS__)
#define LOG_WARNING(message, ...) logMessage(message_type_warning, message, __VA_ARGS__)
#define LOG_ERROR(message, ...) logMessage(message_type_error, message, __VA_ARGS__)
#endif

void logMessageInternal(message_type type, const char* file, const char* function, uint32 line, const char* format, ...);
void logMessage(message_type type, const char* format, ...);

void initializeMessageLog();
void updateMessageLog(float dt);

#else
#define LOG_MESSAGE(...)
#define LOG_WARNING(...)
#define LOG_ERROR(...)

#define initializeMessageLog(...)
#define updateMessageLog(...)

#endif