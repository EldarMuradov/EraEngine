#pragma once

namespace era_engine
{
	class Engine final
	{
	public:
		Engine();
		~Engine();

		bool run();

		void terminate();

	private:
		bool update();

		bool running = false;
	};
}