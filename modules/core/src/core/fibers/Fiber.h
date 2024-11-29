#pragma once

namespace era_engine
{
	class Fiber
	{
	public:
		using Callback_t = void(*)(Fiber*);

	private:
		void* m_fiber = nullptr;
		bool m_thread_fiber = false;

		Fiber* m_return_fiber = nullptr;

		Callback_t m_callback = nullptr;
		void* m_userdata = nullptr;

		Fiber(void* fiber) :
			m_fiber(fiber)
		{};

	public:
		Fiber();
		Fiber(const Fiber&) = delete;
		~Fiber();

		// Converts current Thread to a Fiber
		void FromCurrentThread();

		// Set Callback
		void SetCallback(Callback_t);

		// Fiber Switching
		void SwitchTo(Fiber*, void* = nullptr);
		void SwitchBack();
		
		// Getter
		inline Callback_t GetCallback() const { return m_callback; };
		inline void* GetUserdata() const { return m_userdata; };
		inline bool IsValid() const { return m_fiber && m_callback; };
	};
}