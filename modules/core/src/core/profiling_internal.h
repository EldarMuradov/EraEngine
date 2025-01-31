#pragma once

#include "core_api.h"

#define COMPOSITE_VARNAME_(a, b) a##b
#define COMPOSITE_VARNAME(a, b) COMPOSITE_VARNAME_(a, b)

namespace era_engine
{
	enum ProfileEventType : uint16
	{
		profile_event_none,
		profile_event_frame_marker,
		profile_event_begin_block,
		profile_event_end_block,

		profile_event_count
	};

	struct ERA_CORE_API ProfileEvent
	{
		ProfileEventType type;
		uint16 cl_type; // For gpu profiler.
		uint32 thread_id;
		const char* name;
		uint64 timestamp;
	};
}

#ifdef PROFILING_INTERNAL

#define INVALID_PROFILE_BLOCK 0xFFFF

namespace era_engine
{
	struct ERA_CORE_API ProfileBlock
	{
		uint16 first_child;
		uint16 last_child;
		uint16 next_sibling;
		uint16 parent;

		uint64 start_clock;
		uint64 end_clock;

		float rel_start;
		float duration;

		uint32 thread_id;

		const char* name;
	};

	struct ERA_CORE_API ProfileFrame
	{
		uint64 start_clock;
		uint64 end_clock;
		uint64 global_frame_id;

		float duration;
	};

	inline constexpr float left_padding = 5.0f;

	struct ERA_CORE_API ProfilerPersistent
	{
		uint32 highlight_frame_index = -1;

		float frame_width_multiplier = 1.f;
		float callstack_left_padding = left_padding;

		float horizontal_scroll_anchor = 0;
		bool horizontal_scrolling = false;
	};

	struct ERA_CORE_API ProfilerTimeline
	{
		uint32 num_frames;
		float total_width;

		float bar_height16ms;
		float bar_height33ms;

		float right_edge;
		float horizontal_bar_stride;
		float bar_width;
		float call_stack_top;

		uint32 color_index = 0;

		ProfilerPersistent& persistent;

		ProfilerTimeline(ProfilerPersistent& _persistent, uint32 _num_frames);
		bool draw_header(bool& pause_recording);
		void draw_overview_frame(ProfileFrame& frame, uint32 frame_index, uint32 current_frame);
		void end_overview();

		void draw_highlight_frame_info(ProfileFrame& frame);
		void draw_call_stack(ProfileBlock* blocks, uint16 start_index, const char* name = 0);
		void draw_millisecond_spacings(ProfileFrame& frame) const;
		void handle_user_interactions();
	};

	// Returns true if frame-end marker is found.
	bool handle_profile_event(ProfileEvent* events, uint32 event_index, uint32 num_events, uint16* stack, uint32& d, ProfileBlock* blocks, uint32& num_blocks_used, uint64& frame_end_timestamp, bool lookahead);
	void copy_profile_blocks(ProfileBlock* src, uint16* stack, uint32 depth, ProfileBlock* dest, uint32& num_dest_blocks);
}
#endif