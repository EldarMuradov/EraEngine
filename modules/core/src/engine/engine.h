#pragma once

#include <rttr/type>

namespace era_engine
{
	class Engine final
	{
		Engine();
		~Engine();
	public:
		static Engine* instance();
		static void release();

		bool run();

		void terminate();

		template<typename Object>
		Object* get(std::vector<rttr::argument> args = std::vector<rttr::argument>())
		{
			rttr::type type = rttr::type::get<Object>();
			Engine* engine = instance();

			auto iter = engine->single_objects.find(type);
			if (iter != engine->single_objects.end())
			{
				return iter->second.get_value<Object*>();
			}

			rttr::variant object = type.create(args);
			engine->single_objects.emplace(type, object);
			return object.get_value<Object*>();
		}

	private:
		bool update();

		bool running = false;

		std::unordered_map<rttr::type, rttr::variant> single_objects;
	};

	template<typename Object>
	inline Object* get_object(std::vector<rttr::argument> args = std::vector<rttr::argument>())
	{
		return Engine::instance()->get<Object>(args);
	}

	template<typename Object>
	inline Object* get_transient_object(std::vector<rttr::argument> args = std::vector<rttr::argument>())
	{
		rttr::type type = rttr::type::get<Object>();
		rttr::variant object = type.create(args);
		return object.get_value<Object*>();
	}
}