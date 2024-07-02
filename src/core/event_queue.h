#pragma once

namespace era_engine
{
	constexpr size_t MAX_PENDING = 1024;

	template <typename Event_>
	using EventType = std::enable_if_t<!std::is_pointer_v<Event_>, bool>;

	template <typename Event, EventType<Event> = true>
	using ProcessEventFunction = void (*)(Event&);

	template <typename Event, EventType<Event> = true>
	struct event_queue
	{
		virtual void pushEvent(const Event& event) noexcept
		{
			if (num < MAX_PENDING - 1)
				pending[num++] = event;
			else
				LOG_WARNING("Event Queue> Can't push event in queue!");
		}

		virtual void processQueue(ProcessEventFunction<Event> func) noexcept
		{
			for (size_t i = 0; i < num; ++i)
			{
				func(pending[i]);
			}
			num = 0;
		}

		virtual bool empty() const noexcept { return num == 0; }

	protected:
		Event pending[MAX_PENDING];
		size_t num = 0;
	};

	template <typename Event, EventType<Event> = true>
	struct concurrent_event_queue : event_queue<Event>
	{
		virtual void pushEvent(const Event& event) noexcept
		{
			shared_spin_lock lock{ sync };
			queue.pushEvent(event);
		}

		virtual void processQueue(ProcessEventFunction<Event> func) noexcept
		{
			shared_spin_lock lock{ sync };
			queue.processQueue(func);
		}

		virtual bool empty() const noexcept { return queue.empty(); }

	private:
		event_queue<Event> queue;

		spin_lock sync;
	};
}