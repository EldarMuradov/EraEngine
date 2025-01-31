#define PROFILING_INTERNAL

#include "core/profiling_internal.h"
#include "core/imgui.h"
#include "core/input.h"

namespace era_engine
{
	bool handle_profile_event(ProfileEvent* events, uint32 event_index, uint32 num_events, uint16* stack, uint32& d, ProfileBlock* blocks, uint32& num_blocks_used, uint64& frame_end_timestamp, bool lookahead)
	{
		ProfileEvent* e = events + event_index;

		bool result = false;

		switch (e->type)
		{
		case profile_event_begin_block:
		{
		REHANDLE_EVENT:
			uint64 timestamp = e->timestamp;

			// The end event of one block often has the exact same timestamp as the begin event of the next block(s).
			// When recording events multithreaded (e.g. when rendering on multiple cores), the order of these events 
			// in the CPU-side array may not be correct.
			// We thus search for possible end blocks with the same timestamp here and - if we already saw a matching 
			// begin block earlier - process them first.

			if (lookahead && d > 0)
			{
				ProfileBlock* current_stack_top = blocks + stack[d - 1];

				for (uint32 j = event_index + 1; j < num_events && events[j].timestamp == timestamp; ++j)
				{
					if (events[j].type == profile_event_end_block
						&& events[j].thread_id != e->thread_id				// Event is from another thread than current event. Events on the same thread are always in the correct order.
						&& events[j].thread_id == current_stack_top->thread_id	// Event is from the same thread as current stack top.
						&& events[j].name == current_stack_top->name)			// Event has the same name as current stack top.
					{
						--d;
						current_stack_top->end_clock = timestamp;

						events[j].type = profile_event_none; // Mark as handled.
						goto REHANDLE_EVENT;
					}
				}
			}

			uint32 index = num_blocks_used++;

			ProfileBlock& block = blocks[index];
			block.start_clock = timestamp;
			block.end_clock = 0;
			block.parent = (d == 0) ? INVALID_PROFILE_BLOCK : stack[d - 1];
			block.name = e->name;
			block.thread_id = e->thread_id;
			block.first_child = INVALID_PROFILE_BLOCK;
			block.last_child = INVALID_PROFILE_BLOCK;
			block.next_sibling = INVALID_PROFILE_BLOCK;

			if (block.parent != INVALID_PROFILE_BLOCK) // d > 0.
			{
				ProfileBlock* parent = blocks + block.parent;

				if (parent->first_child == INVALID_PROFILE_BLOCK)
				{
					parent->first_child = index;
				}
				if (parent->last_child != INVALID_PROFILE_BLOCK)
				{
					ProfileBlock* last_child = blocks + parent->last_child;
					last_child->next_sibling = index;
				}
				parent->last_child = index;
			}
			else if (stack[d] != INVALID_PROFILE_BLOCK) // d is guaranteed to be 0 here. Therefore we can check for the validity of the stack element. This is always initialized.
			{
				ProfileBlock* current = blocks + stack[d];
				current->next_sibling = index;
			}

			stack[d] = index;
			++d;
		} break;

		case profile_event_end_block:
		{
			--d;

			ProfileBlock* block = blocks + stack[d];
			ASSERT(block->name == e->name);

			block->end_clock = e->timestamp;
		} break;

		case profile_event_frame_marker:
		{
			frame_end_timestamp = e->timestamp;
			result = true;
		} break;
		}

		return result;
	}

	void copy_profile_blocks(ProfileBlock* src, uint16* stack, uint32 depth, ProfileBlock* dest, uint32& num_dest_blocks)
	{
		for (uint32 d = 0; d < depth; ++d)
		{
			uint32 index = num_dest_blocks++;
			ProfileBlock& old = src[stack[d]];
			ProfileBlock& block = dest[index];

			block.start_clock = old.start_clock;
			block.end_clock = 0;
			block.parent = (d == 0) ? INVALID_PROFILE_BLOCK : stack[d - 1];
			block.name = old.name;
			block.thread_id = old.thread_id;
			block.first_child = INVALID_PROFILE_BLOCK;
			block.last_child = INVALID_PROFILE_BLOCK;
			block.next_sibling = INVALID_PROFILE_BLOCK;

			if (block.parent != INVALID_PROFILE_BLOCK) // d > 0.
			{
				ProfileBlock* parent = dest + block.parent;

				if (parent->first_child == INVALID_PROFILE_BLOCK)
				{
					parent->first_child = index;
				}
				parent->last_child = index;
			}

			stack[d] = index;
		}
	}

	static const ImColor highlight_frame_color = ImGui::green;

	inline constexpr float timeline_bottom = 400.f;
	inline constexpr float right_padding = 50.f;
	inline constexpr float highlight_top = timeline_bottom + 150.f;
	inline constexpr float vertical_bar_stride = 40.f;

	static const ImColor color_table[] =
	{
		ImColor(107, 142, 35),
		ImColor(220, 20, 60),
		ImColor(128, 0, 0),
		ImColor(124, 252, 0),
		ImColor(60, 179, 113),
		ImColor(250, 235, 215),
		ImColor(0, 100, 0),
		ImColor(0, 255, 255),
		ImColor(143, 188, 143),
		ImColor(233, 150, 122),
		ImColor(255, 255, 0),
		ImColor(147, 112, 219),
		ImColor(255, 69, 0),
		ImColor(255, 215, 0),
		ImColor(221, 160, 221),
		ImColor(25, 25, 112),
		ImColor(138, 43, 226),
		ImColor(0, 128, 128),
		ImColor(0, 191, 255),
		ImColor(189, 183, 107),
		ImColor(176, 224, 230),
		ImColor(65, 105, 225),
		ImColor(255, 250, 240),
		ImColor(139, 69, 19),
		ImColor(245, 255, 250),
		ImColor(188, 143, 143),
	};

	ProfilerTimeline::ProfilerTimeline(ProfilerPersistent& _persistent, uint32 _num_frames)
		: persistent(_persistent), num_frames(_num_frames)
	{
		total_width = ImGui::GetContentRegionAvail().x - left_padding - right_padding;

		bar_height16ms = 100.f;
		bar_height33ms = bar_height16ms * 2.f;
		
		right_edge = left_padding + total_width;
		horizontal_bar_stride = total_width / num_frames;
		bar_width = horizontal_bar_stride/* * 0.9f*/;

		call_stack_top = highlight_top;
	}

	bool ProfilerTimeline::draw_header(bool& pause_recording)
	{
		bool result = false;
		if (ImGui::Button(pause_recording ? (ICON_FA_PLAY "  Resume recording") : (ICON_FA_PAUSE "  Pause recording")))
		{
			pause_recording = !pause_recording;
			result = true;
		}
		ImGui::SameLine();
		ImGui::Text("The last %u frames are recorded. Click on one frame to get a detailed hierarchical view of all blocks. Zoom into detail view with mouse wheel and click and drag to shift the display.", num_frames);
		return result;
	}

	void ProfilerTimeline::draw_overview_frame(ProfileFrame& frame, uint32 frame_index, uint32 current_frame)
	{
		if (frame.duration > 0.f)
		{
			float left = left_padding + frame_index * horizontal_bar_stride;
			float height = frame.duration / (1000.f / 60.f) * bar_height16ms;
			if (height > 0.f)
			{
				float top = timeline_bottom - height;

				ImGui::PushID(frame_index);

				ImGui::SetCursorPos(ImVec2(left, top));

				ImColor color = (frame_index == persistent.highlight_frame_index) ? highlight_frame_color : ImGui::red;
				color = (frame_index == current_frame) ? ImGui::yellow : color;

				bool result = ImGui::ColorButton("", color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(bar_width, height));
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Frame %llu (%fms)", frame.global_frame_id, frame.duration);
				}

				if (result)
				{
					persistent.highlight_frame_index = frame_index;
				}

				ImGui::PopID();
			}
		}
	}

	void ProfilerTimeline::end_overview()
	{
		ImGui::SetCursorPos(ImVec2(left_padding, timeline_bottom - bar_height16ms - 1));
		ImGui::ColorButton("##60FPS", ImGui::white, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(total_width, 1));

		ImGui::SetCursorPos(ImVec2(left_padding, timeline_bottom - bar_height33ms - 1));
		ImGui::ColorButton("##30FPS", ImGui::white, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(total_width, 1));

		ImGui::SetCursorPos(ImVec2(right_edge + 3.f, timeline_bottom - bar_height16ms - 1));
		ImGui::Text("16.7ms");

		ImGui::SetCursorPos(ImVec2(right_edge + 3.f, timeline_bottom - bar_height33ms - 1));
		ImGui::Text("33.3ms");
	}

	void ProfilerTimeline::draw_highlight_frame_info(ProfileFrame& frame)
	{
		ImGui::SetCursorPos(ImVec2(left_padding, highlight_top - 80.f));
		ImGui::Text("Frame %llu (%fms)", frame.global_frame_id, frame.duration);
	}

	void ProfilerTimeline::draw_call_stack(ProfileBlock* blocks, uint16 start_index, const char* name)
	{
#if 0
		ImGui::SameLine();
		if (ImGui::Button("Dump this frame to stdout"))
		{
			uint16 current_index = 0;
			uint32 depth = 0;

			while (current_index != INVALID_PROFILE_BLOCK)
			{
				ProfileBlock* current = blocks + current_index;

				for (uint32 i = 0; i < depth; ++i)
				{
					std::cout << " ";
				}
				std::cout << current->name << '\n';

				// Advance.
				uint16 next_index = current->first_child;
				if (next_index == INVALID_PROFILE_BLOCK)
				{
					next_index = current->next_sibling;

					if (next_index == INVALID_PROFILE_BLOCK)
					{
						uint16 next_ancestor = current->parent;
						while (next_ancestor != INVALID_PROFILE_BLOCK)
						{
							--depth;
							if (blocks[next_ancestor].next_sibling != INVALID_PROFILE_BLOCK)
							{
								next_index = blocks[next_ancestor].next_sibling;
								break;
							}
							next_ancestor = blocks[next_ancestor].parent;
						}
					}
				}
				else
				{
					++depth;
				}
				current_index = nextIndex;
			}
		}
#endif

		if (name)
		{
			ImGui::SetCursorPos(ImVec2(left_padding, call_stack_top - 5.f));
			ImGui::Text(name);
		}

		if (call_stack_top != highlight_top)
		{
			ImGui::SetCursorPos(ImVec2(left_padding, call_stack_top - 5.f));
			ImGui::ColorButton("", ImGui::white, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(total_width + right_padding, 1));
		}

		static const float bar_height = vertical_bar_stride * 0.8f;

		const float frame_width16ms = total_width * persistent.frame_width_multiplier;
		const float frame_width33ms = frame_width16ms * 2.f;

		// Call stack.
		uint16 current_index = start_index;
		uint32 depth = 0;
		uint32 max_depth = 0;

		while (current_index != INVALID_PROFILE_BLOCK)
		{
			ProfileBlock* current = blocks + current_index;

			// Draw.
			float top = call_stack_top + depth * vertical_bar_stride;
			float left = persistent.callstack_left_padding + current->rel_start / (1000.f / 60.f) * frame_width16ms;
			float width = current->duration / (1000.f / 60.f) * frame_width16ms;
			if (width > 0.f) // Important. ImGui renders zero-size elements with a default size (> 0).
			{
				ImGui::SetCursorPos(ImVec2(left, top));
				ImGui::ColorButton(current->name, color_table[color_index++],
					ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_NoDragDrop,
					ImVec2(width, bar_height));

				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s: %.3fms", current->name, current->duration);
				}

				ImGui::PushClipRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
				ImGui::SetCursorPos(ImVec2(left + ImGui::GetStyle().FramePadding.x, top + ImGui::GetStyle().FramePadding.y));
				ImGui::Text("%s: %.3fms%s", current->name, current->duration, (current->end_clock == 0) ? " (continues in next frame" : "");
				ImGui::PopClipRect();


				if (color_index >= arraysize(color_table))
				{
					color_index = 0;
				}
			}

			// Advance
			uint16 nextIndex = current->first_child;
			if (nextIndex == INVALID_PROFILE_BLOCK)
			{
				nextIndex = current->next_sibling;

				if (nextIndex == INVALID_PROFILE_BLOCK)
				{
					uint16 next_ancestor = current->parent;
					while (next_ancestor != INVALID_PROFILE_BLOCK)
					{
						--depth;
						if (blocks[next_ancestor].next_sibling != INVALID_PROFILE_BLOCK)
						{
							nextIndex = blocks[next_ancestor].next_sibling;
							break;
						}
						next_ancestor = blocks[next_ancestor].parent;
					}
				}
			}
			else
			{
				++depth;
				max_depth = max(depth, max_depth);
			}
			current_index = nextIndex;
		}

		ASSERT(depth == 0);

		call_stack_top += (max_depth + 1) * vertical_bar_stride + 10.f;
	}

	void ProfilerTimeline::draw_millisecond_spacings(ProfileFrame& frame) const
	{
		const float frame_width16ms = total_width * persistent.frame_width_multiplier;
		const float frame_width33ms = frame_width16ms * 2.f;

		float call_stack_height = call_stack_top - highlight_top;

		const float text_spacing = 30.f;
		const float line_height = call_stack_height + text_spacing;
		const float line_top = highlight_top - text_spacing;

		// 0ms
		ImGui::SetCursorPos(ImVec2(persistent.callstack_left_padding, line_top));
		ImGui::ColorButton("##0ms", ImGui::white, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(1, line_height));

		ImGui::SetCursorPos(ImVec2(persistent.callstack_left_padding + 2, line_top - 5));
		ImGui::Text("0ms");

		// 16ms
		ImGui::SetCursorPos(ImVec2(persistent.callstack_left_padding + frame_width16ms, line_top));
		ImGui::ColorButton("##16ms", ImGui::white, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(1, line_height));

		ImGui::SetCursorPos(ImVec2(persistent.callstack_left_padding + frame_width16ms + 2, line_top - 5));
		ImGui::Text("16.7ms");

		// 33ms
		ImGui::SetCursorPos(ImVec2(persistent.callstack_left_padding + frame_width33ms, line_top));
		ImGui::ColorButton("##33ms", ImGui::white, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(1, line_height));

		ImGui::SetCursorPos(ImVec2(persistent.callstack_left_padding + frame_width33ms + 2, line_top - 5));
		ImGui::Text("33.3ms");

		// 1ms spacings
		const ImVec4 normal_color(0.2f, 0.2f, 0.2f, 1.f);
		const ImVec4 special_color(0.5f, 0.5f, 0.5f, 1.f);

		const float millisecond_spacing = frame_width33ms / (1000.f / 30.f);
		for (uint32 i = 1; i <= 33; ++i)
		{
			ImVec4 color = (i % 5 == 0) ? special_color : normal_color;

			ImGui::SetCursorPos(ImVec2(persistent.callstack_left_padding + i * millisecond_spacing, line_top + 8));
			ImGui::ColorButton("", color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(1, line_height));
		}

		// Frame end
		ImGui::SetCursorPos(ImVec2(persistent.callstack_left_padding + frame.duration * millisecond_spacing, line_top));
		ImGui::ColorButton("##Frame end", ImGui::blue, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(1, line_height));

		ImGui::SetCursorPos(ImVec2(persistent.callstack_left_padding + frame.duration * millisecond_spacing + 2, line_top - 5));
		ImGui::Text("Frame end");
	}

	void ProfilerTimeline::handle_user_interactions()
	{
		const float frame_width16ms = total_width * persistent.frame_width_multiplier;
		const float frame_width33ms = frame_width16ms * 2.f;

		float call_stack_height = call_stack_top - highlight_top;

		const float text_spacing = 30.f;
		const float line_height = call_stack_height + text_spacing;
		const float line_top = highlight_top - text_spacing;

		// Invisible widget to block window dragging in this area
		ImGui::SetCursorPos(ImVec2(left_padding, highlight_top));
		ImGui::InvisibleButton("Blocker", ImVec2(total_width + right_padding, call_stack_height));

		ImVec2 mouse_pos = ImGui::GetMousePos();
		ImVec2 window_pos = ImGui::GetWindowPos();

		float rel_mouse_x = mouse_pos.x - window_pos.x;

		bool over_stack = false;
		if (!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId) && // If dropdown or smth is open, don't interact with call stack.
			ImGui::IsMouseHoveringRect(ImVec2(left_padding + window_pos.x, highlight_top + window_pos.y), ImVec2(left_padding + total_width + right_padding + window_pos.x, highlight_top + call_stack_height + window_pos.y), false))
		{
			over_stack = true;

			// Hover time
			float hovered_x = ImGui::GetMousePos().x - ImGui::GetWindowPos().x;
			float hovered_time = (hovered_x - persistent.callstack_left_padding) / frame_width33ms * (1000.f / 30.f);
			ImGui::SetCursorPos(ImVec2(hovered_x, line_top));
			ImGui::ColorButton("", ImGui::yellow, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(1, line_height));

			ImGui::SetCursorPos(ImVec2(hovered_x + 2, line_top - 5));
			ImGui::Text("%.3fms", hovered_time);
		}

		// Horizontal scrolling
		if (!ImGui::IsMouseDown(ImGuiPopupFlags_MouseButtonLeft))
		{
			persistent.horizontal_scrolling = false;
		}
		if (over_stack && ImGui::IsMouseClicked(ImGuiPopupFlags_MouseButtonLeft))
		{
			persistent.horizontal_scrolling = rel_mouse_x;
			persistent.horizontal_scrolling = true;
		}
		if (persistent.horizontal_scrolling)
		{
			persistent.callstack_left_padding += rel_mouse_x - persistent.horizontal_scrolling;
			persistent.horizontal_scrolling = rel_mouse_x;
		}

		// Zooming
		if (over_stack)
		{
			float zoom = ImGui::GetIO().MouseWheel;

			if (zoom != 0.f && ImGui::IsKeyDown(key_ctrl))
			{
				float t = inverse_lerp(persistent.callstack_left_padding, persistent.callstack_left_padding + frame_width16ms, rel_mouse_x);

				persistent.frame_width_multiplier += zoom * 0.1f;
				persistent.frame_width_multiplier = max(persistent.frame_width_multiplier, 0.2f);

				float new_frame_width16ms = total_width * persistent.frame_width_multiplier;
				persistent.callstack_left_padding = rel_mouse_x - t * new_frame_width16ms;
			}
		}

		ImGui::SetCursorPos(ImVec2(left_padding, highlight_top + call_stack_height));
	}
}