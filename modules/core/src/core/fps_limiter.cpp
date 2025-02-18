#include "core/fps_limiter.h"

namespace era_engine
{

	FpsLimiter::FpsLimiter(int32_t _frame_rate, bool _enabled)
	{
		enabled = _enabled;
		frame_time = 1000.0f / static_cast<float>(_frame_rate);
	}

	FpsLimiter::~FpsLimiter()
	{
	}

	void FpsLimiter::set_enabled(bool _enabled)
	{
		enabled = _enabled;
	}

	bool FpsLimiter::get_enabled() const
	{
		return enabled;
	}

	void FpsLimiter::begin_frame()
	{
		frame_start_time = std::chrono::high_resolution_clock::now();
		frame_started = true;
	}

	void FpsLimiter::end_frame()
	{
		if(frame_started)
		{
			auto current_timme = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float, std::milli> elapsed_seconds = current_timme - frame_start_time;
			float wait_time = frame_time - elapsed_seconds.count();
			if (wait_time > 0.0f && enabled)
			{
				wait(wait_time);
			}
			frame_started = false;
		}
	}

	void FpsLimiter::wait(float wait_time)
	{
		std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(wait_time));
	}

}