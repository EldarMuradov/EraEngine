// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "editor/undo_stack.h"

#include "core/imgui.h"

namespace era_engine
{
	UndoStack::UndoStack()
	{
		memory_size = mb(2);
		memory = (uint8*)malloc(memory_size);
		reset();
	}

	void UndoStack::push_action(const char* name, const void* entry, uint64 entry_size, ToggleFunc toggle)
	{
		uint64 name_length = strlen(name) + 1;
		uint64 aligned_name_length = align_to(name_length, 16);
		uint64 required_space = sizeof(EntryHeader) + aligned_name_length + entry_size;
		uint64 available_space_at_end = memory + memory_size - next_to_write;

		uint8* address;

		if (!newest)
		{
			next_to_write = memory;
			address = next_to_write;
			oldest = 0;
		}
		else
		{
			ASSERT(oldest);
			address = (required_space <= available_space_at_end) ? next_to_write : memory;
		}

		uint8* end = (uint8*)align_to(address + required_space, 16);

		// Clean up blocks which are overriden by the new block
		oldest = newest;
		if (oldest)
		{
			while (true)
			{
				void* oldest_begin = oldest;
				void* oldest_end = oldest->get_one_after_end();

				if (ranges_overlap(address, end, oldest_begin, oldest_end))
				{
					oldest = oldest->newer;
					break;
				}
				if (!oldest->older) { break; }

				oldest = oldest->older;
			}
		}

		if (oldest)
		{
			oldest->older = 0;
		}

		EntryHeader* header = (EntryHeader*)address;
		header->name_length = aligned_name_length;

		char* this_name = header->get_name();
		memcpy(this_name, name, name_length);

		header->older = newest;
		if (newest)
		{
			newest->newer = header;
		}
		if (!oldest)
		{
			oldest = header;
		}
		newest = header;

		header->newer = 0;

		header->toggle = toggle;
		header->entry_size = entry_size;

		void* data = header->get_data();
		memcpy(data, entry, entry_size);

		next_to_write = end;
	}

	std::pair<bool, const char*> UndoStack::undo_possible()
	{
		return
		{
			newest != 0,
			newest ? newest->get_name() : 0
		};
	}

	std::pair<bool, const char*> UndoStack::redo_possible()
	{
		return
		{
			newest && newest->newer || !newest && oldest,
			(newest && newest->newer) ? newest->newer->get_name() : (!newest && oldest) ? oldest->get_name() : 0
		};
	}

	void UndoStack::undo()
	{
		if (newest)
		{
			void* data = newest->get_data();
			newest->toggle(data);

			newest = newest->older;
			if (newest)
			{
				next_to_write = (uint8*)align_to(((uint8*)data + newest->entry_size), 16);
			}
			else
			{
				next_to_write = memory;
			}

			// Keep link to newer
		}
	}

	void UndoStack::redo()
	{
		if (newest && newest->newer)
		{
			newest = newest->newer;

			void* data = newest->get_data();
			newest->toggle(data);

			next_to_write = (uint8*)align_to(((uint8*)data + newest->entry_size), 16);
		}

		if (!newest && oldest)
		{
			void* data = oldest->get_data();
			oldest->toggle(data);

			next_to_write = (uint8*)align_to(((uint8*)data + oldest->entry_size), 16);
			newest = oldest;
		}
	}

	void UndoStack::reset()
	{
		next_to_write = memory;
		oldest = 0;
		newest = 0;
	}

	bool UndoStack::show_history(bool& open)
	{
		bool result = false;

		if (open)
		{
			if (ImGui::Begin(ICON_FA_HISTORY "  Undo history", &open))
			{
				if (ImGui::DisableableButton("Clear history", oldest != 0))
				{
					reset();
				}

				ImGui::Separator();

				bool current_found = false;

				bool clicked = false;
				EntryHeader* target = 0;
				int32 direction = 0;

				bool current = !newest;
				current_found |= current;
				if (ImGui::Selectable("no changes##UndoHistory", current) && !current)
				{
					clicked = true;
					target = 0;
					direction = -1;
				}

				for (EntryHeader* entry = oldest; entry; entry = entry->newer)
				{
					ImGui::PushID(entry);

					bool current = entry == newest;
					current_found |= current;
					if (ImGui::Selectable(entry->get_name(), current) && !current)
					{
						clicked = true;
						target = entry;
						direction = current_found ? 1 : -1;
					}

					ImGui::PopID();
				}

				ASSERT(current_found);

				if (clicked)
				{
					ASSERT(direction != 0);

					while (newest != target)
					{
						(direction == 1) ? redo() : undo();
					}

					result = true;
				}
			}
			ImGui::End();
		}

		return result;
	}

	void UndoStack::verify()
	{
		for (auto entry = oldest; entry; entry = entry->newer)
		{
			if (entry->older)
			{
				ASSERT(entry->older->newer == entry);
			}
			if (entry->newer)
			{
				ASSERT(entry->newer->older == entry);
			}
		}
	}

}