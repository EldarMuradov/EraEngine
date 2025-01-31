#pragma once

#include "core_api.h"

#include "core/log.h"
#include "core/sync.h"

namespace era_engine
{
	template <typename Event_>
	using EventType = std::enable_if_t<!std::is_pointer_v<Event_>, bool>;

	template <typename Event, EventType<Event> = true>
	using ProcessEventFunction = void (*)(Event&);

	template <typename Event, size_t MAX_PENDING = 1024, EventType<Event> = true>
	struct ERA_CORE_API EventQueue
	{
		virtual void push_event(const Event& event)
		{
			if (num < MAX_PENDING - 1)
			{
				pending[num++] = event;
			}
			else
			{
				LOG_WARNING("Event Queue> Can't push event in queue!");
			}
		}

		virtual void process_queue(ProcessEventFunction<Event> func)
		{
			for (size_t i = 0; i < num; ++i)
			{
				func(pending[i]);
			}
			num = 0;
		}

		bool empty() const noexcept { return num == 0; }

	protected:
		Event pending[MAX_PENDING];
		size_t num = 0;
	};

	template <typename Event, size_t MAX_PENDING = 1024, EventType<Event> = true>
	struct ERA_CORE_API ConcurrentEventQueue : EventQueue<Event, MAX_PENDING>
	{
		void push_event(const Event& event) override
		{
			ScopedSpinLock lock{ sync };
			EventQueue::push_event(event);
		}

		void process_queue(ProcessEventFunction<Event> func) override
		{
			ScopedSpinLock lock{ sync };
			EventQueue::process_queue(func);
		}

	private:
		SpinLock sync;
	};
}