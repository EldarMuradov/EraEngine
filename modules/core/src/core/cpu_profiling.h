// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/threading.h"
#include "core/profiling_internal.h"

namespace era_engine
{
	extern bool cpu_profiler_window_open;
}

#if ENABLE_CPU_PROFILING

#define _CPU_PROFILE_BLOCK_(counter, name) era_engine::CpuProfileBlockRecorder COMPOSITE_VARNAME(__PROFILE_BLOCK, counter)(name)
#define CPU_PROFILE_BLOCK(name) _CPU_PROFILE_BLOCK_(__COUNTER__, name)

#define MAX_NUM_CPU_PROFILE_BLOCKS 16384
#define MAX_NUM_CPU_PROFILE_EVENTS (MAX_NUM_CPU_PROFILE_BLOCKS * 2)
#define MAX_NUM_CPU_PROFILE_STATS 512

// 1 bit for array index, 1 free bit, 20 bits for events, 10 bits for stats.
#define _CPU_PROFILE_GET_ARRAY_INDEX(v) ((v) >> 31)
#define _CPU_PROFILE_GET_EVENT_INDEX(v) ((v) & 0xFFFFF)
#define _CPU_PROFILE_GET_STAT_INDEX(v)	(((v) >> 20) & 0x3FF)

#define recordProfileEvent(type_, name_) \
	uint32 array_and_event_index = cpu_profile_index++; \
	uint32 event_index = _CPU_PROFILE_GET_EVENT_INDEX(array_and_event_index); \
	uint32 array_index = _CPU_PROFILE_GET_ARRAY_INDEX(array_and_event_index); \
	ASSERT(event_index < MAX_NUM_CPU_PROFILE_EVENTS); \
	era_engine::ProfileEvent* e = cpu_profile_events[array_index] + event_index; \
	e->thread_id = get_thread_id_fast(); \
	e->name = name_; \
	e->type = type_; \
	QueryPerformanceCounter((LARGE_INTEGER*)&e->timestamp); \
	cpu_profile_completely_written[array_index].fetch_add(1, std::memory_order_release); // Mark this event as written. Release means that the compiler may not reorder the previous writes after this.

#define _CPU_PROFILE_STAT(label_value, value, member, value_type) \
	uint32 array_and_stat_index = cpu_profile_index.fetch_add(1 << 20); \
	uint32 stat_index = _CPU_PROFILE_GET_STAT_INDEX(array_and_stat_index); \
	uint32 array_index = _CPU_PROFILE_GET_ARRAY_INDEX(array_and_stat_index); \
	ASSERT(stat_index < MAX_NUM_CPU_PROFILE_STATS); \
	era_engine::ProfileStat* stat_ = cpu_profile_stats[array_index] + stat_index; \
	stat_->label = label_value; \
	stat_->member = value; \
	stat_->type = value_type; \
	cpu_profile_completely_written[array_index].fetch_add(1, std::memory_order_release); // Mark this stat as written. Release means that the compiler may not reorder the previous writes after this.

//#undef recordProfileEvent

#define _CPU_PRINT_PROFILE_BLOCK_(counter, name) era_engine::CpuPrintProfileBlockRecorder COMPOSITE_VARNAME(__PROFILE_BLOCK, counter)(name)
#define CPU_PRINT_PROFILE_BLOCK(name) _CPU_PRINT_PROFILE_BLOCK_(__COUNTER__, name)

namespace era_engine
{
	enum ProfileStatType
	{
		profile_stat_type_bool,
		profile_stat_type_int32,
		profile_stat_type_uint32,
		profile_stat_type_int64,
		profile_stat_type_uint64,
		profile_stat_type_float,
		profile_stat_type_string,
	};

	struct ERA_CORE_API ProfileStat
	{
		const char* label;
		union
		{
			bool bool_value;
			int32 int32_value;
			uint32 uint32_value;
			int64 int64_value;
			uint64 uint64_value;
			float float_value;
			const char* string_value;
		};
		ProfileStatType type;
	};

	ERA_CORE_API extern std::atomic<uint32> cpu_profile_index;
	ERA_CORE_API extern std::atomic<uint32> cpu_profile_completely_written[2];
	ERA_CORE_API extern ProfileEvent cpu_profile_events[2][MAX_NUM_CPU_PROFILE_EVENTS];
	ERA_CORE_API extern ProfileStat cpu_profile_stats[2][MAX_NUM_CPU_PROFILE_STATS];

	struct ERA_CORE_API CpuProfileBlockRecorder
	{
		CpuProfileBlockRecorder(const char* name)
			: name(name)
		{
			recordProfileEvent(profile_event_begin_block, name);
		}

		~CpuProfileBlockRecorder()
		{
			recordProfileEvent(profile_event_end_block, name);
		}

		const char* name;
	};

	inline void cpu_profiling_frame_end_marker()
	{
		recordProfileEvent(profile_event_frame_marker, 0);
	}

	inline void CPU_PROFILE_STAT(const char* label, bool value) { _CPU_PROFILE_STAT(label, value, bool_value, profile_stat_type_bool); }
	inline void CPU_PROFILE_STAT(const char* label, int32 value) { _CPU_PROFILE_STAT(label, value, int32_value, profile_stat_type_int32); }
	inline void CPU_PROFILE_STAT(const char* label, uint32 value) { _CPU_PROFILE_STAT(label, value, uint32_value, profile_stat_type_uint32); }
	inline void CPU_PROFILE_STAT(const char* label, int64 value) { _CPU_PROFILE_STAT(label, value, int64_value, profile_stat_type_int64); }
	inline void CPU_PROFILE_STAT(const char* label, uint64 value) { _CPU_PROFILE_STAT(label, value, uint64_value, profile_stat_type_uint64); }
	inline void CPU_PROFILE_STAT(const char* label, float value) { _CPU_PROFILE_STAT(label, value, float_value, profile_stat_type_float); }
	inline void CPU_PROFILE_STAT(const char* label, const char* value) { _CPU_PROFILE_STAT(label, value, string_value, profile_stat_type_string); }

	ERA_CORE_API void cpu_profiling_resolve_time_stamps();

	struct ERA_CORE_API CpuPrintProfileBlockRecorder
	{
		CpuPrintProfileBlockRecorder(const char* name)
			: name(name)
		{
			QueryPerformanceCounter((LARGE_INTEGER*)&start);
		}

		~CpuPrintProfileBlockRecorder()
		{
			uint64 end;
			QueryPerformanceCounter((LARGE_INTEGER*)&end);

			uint64 clock_frequency;
			QueryPerformanceFrequency((LARGE_INTEGER*)&clock_frequency);

			float duration = (float)(end - start) / clock_frequency * 1000.f;
			std::cout << "Profile block '" << name << "' took " << duration << "ms.\n";
		}

		const char* name;
		uint64 start;
	};
}

#else

#define CPU_PROFILE_BLOCK(...)
#define CPU_PROFILE_STAT(...)

#define cpu_profiling_frame_end_marker(...)
#define cpu_profiling_resolve_time_stamps(...)

#define CPU_PRINT_PROFILE_BLOCK(...)

#endif