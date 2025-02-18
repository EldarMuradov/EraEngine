#pragma once

namespace era_engine
{

	class GameStartup final
	{
	public:
		GameStartup() = default;
		~GameStartup() = default;

		void start(int argc, char** argv);
	};

}