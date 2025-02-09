#pragma once

namespace era_engine
{

	class EditorStartup final
	{
	public:
		EditorStartup() = default;
		~EditorStartup() = default;

		void start(int argc, char** argv);
	};

}