// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "core/memory.h"

struct undo_stack
{
	undo_stack();

	template <typename T>
	void pushAction(const char* name, const T& entry); // Type T must have member function void toggle().

	std::pair<bool, const char*> undoPossible();
	std::pair<bool, const char*> redoPossible();

	void undo();
	void redo();

	void reset();
	bool showHistory(bool& open);
	void verify();

private:
	typedef void (*toggle_func)(void*);

	void pushAction(const char* name, const void* entry, uint64 entrySize, toggle_func toggle);

	struct alignas(16) entry_header
	{
		toggle_func toggle;

		entry_header* newer;
		entry_header* older;

		uint64 nameLength; // Includes null-terminator.
		uint64 entrySize;

		NODISCARD char* getName() const
		{
			return (char*)(this + 1);
		}

		NODISCARD void* getData()
		{
			return (uint8*)getName() + nameLength;
		}

		NODISCARD void* getOneAfterEnd()
		{
			return (uint8*)getData() + entrySize;
		}
	};

	uint8* memory = nullptr;
	uint32 memorySize{};

	uint8* nextToWrite = nullptr;
	entry_header* oldest = nullptr;
	entry_header* newest = nullptr;
};

template<typename T>
inline void undo_stack::pushAction(const char* name, const T& entry)
{
	static_assert(std::is_trivially_destructible_v<T>, "Undo entries must be trivially destructible.");

	toggle_func toggle = [](void* data)
	{
		T* t = (T*)data;
		t->toggle();
	};

	pushAction(name, &entry, sizeof(T), toggle);
}