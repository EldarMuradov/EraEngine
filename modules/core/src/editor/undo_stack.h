// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "core/memory.h"

namespace era_engine
{
	struct UndoStack
	{
		UndoStack();

		template <typename T>
		void push_action(const char* name, const T& entry); // Type T must have member function void toggle().

		std::pair<bool, const char*> undo_possible();
		std::pair<bool, const char*> redo_possible();

		void undo();
		void redo();

		void reset();
		bool show_history(bool& open);
		void verify();

	private:
		typedef void (*ToggleFunc)(void*);

		void push_action(const char* name, const void* entry, uint64 entry_size, ToggleFunc toggle);

		struct alignas(16) EntryHeader
		{
			NODISCARD char* get_name() const
			{
				return (char*)(this + 1);
			}

			NODISCARD void* get_data()
			{
				return (uint8*)get_name() + name_length;
			}

			NODISCARD void* get_one_after_end()
			{
				return (uint8*)get_data() + entry_size;
			}

			ToggleFunc toggle;

			EntryHeader* newer;
			EntryHeader* older;

			uint64 name_length; // Includes null-terminator.
			uint64 entry_size;
		};

		uint8* memory = nullptr;
		uint32 memory_size{};

		uint8* next_to_write = nullptr;
		EntryHeader* oldest = nullptr;
		EntryHeader* newest = nullptr;
	};

	template<typename T>
	inline void UndoStack::push_action(const char* name, const T& entry)
	{
		static_assert(std::is_trivially_destructible_v<T>, "Undo entries must be trivially destructible.");

		ToggleFunc toggle = [](void* data)
			{
				T* t = (T*)data;
				t->toggle();
			};

		push_action(name, &entry, sizeof(T), toggle);
	}
}