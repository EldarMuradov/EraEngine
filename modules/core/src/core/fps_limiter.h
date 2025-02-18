#pragma once

#include "core_api.h"

#include <chrono>

namespace era_engine
{

	class ERA_CORE_API FpsLimiter final
	{
	public:
		FpsLimiter(int32_t _frame_rate = 60.0f, bool _enabled = true);
		~FpsLimiter();

		void set_enabled(bool _enabled);
		bool get_enabled() const;

		void begin_frame();
		void end_frame();

	private:
		void wait(float wait_time);

	private:
		float frame_time = 0.0f;
		bool enabled = true;
		bool frame_started = false;
		
		std::chrono::high_resolution_clock::time_point frame_start_time;
	};

}