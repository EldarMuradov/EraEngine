#include "core/fibers/Fiber.h"
#include "core/fibers/Exception.h"


#ifdef _WIN32
#include <Windows.h>
#endif

// TODO: Add exceptiosn for invalid stuff?

namespace era_engine
{
	static void LaunchFiber(Fiber* fiber)
	{
		auto callback = fiber->GetCallback();
		if (callback == nullptr) 
		{
			throw Exception("LaunchFiber: callback is nullptr");
		}

		callback(fiber);
	}

	Fiber::Fiber()
	{
#ifdef _WIN32
		m_fiber = CreateFiber(0, (LPFIBER_START_ROUTINE)LaunchFiber, this);
		m_thread_fiber = false;
#endif
	}

	Fiber::~Fiber()
	{
#ifdef _WIN32
		if (m_fiber && !m_thread_fiber) 
		{
			DeleteFiber(m_fiber);
		}
#endif
	}

	void Fiber::FromCurrentThread()
	{
#ifdef _WIN32
		if (m_fiber && !m_thread_fiber) 
		{
			DeleteFiber(m_fiber);
		}

		m_fiber = ConvertThreadToFiber(nullptr);
		m_thread_fiber = true;
#endif
	}

	void Fiber::SetCallback(Callback_t cb)
	{
		if (cb == nullptr) 
		{
			throw Exception("callback cannot be nullptr");
		}

		m_callback = cb;
	}

	void Fiber::SwitchTo(Fiber* fiber, void* userdata)
	{
		if (fiber == nullptr || fiber->m_fiber == nullptr) 
		{
			throw Exception("Invalid fiber (nullptr or invalid)");
		}

		fiber->m_userdata = userdata;
		fiber->m_return_fiber = this;

		SwitchToFiber(fiber->m_fiber);
	}

	void Fiber::SwitchBack()
	{
		if (m_return_fiber && m_return_fiber->m_fiber) 
		{
			SwitchToFiber(m_return_fiber->m_fiber);
		}
		else 
		{
			throw Exception("Unable to switch back from Fiber (none or invalid return fiber)");
		}
	}
}