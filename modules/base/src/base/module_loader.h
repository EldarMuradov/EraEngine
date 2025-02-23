#pragma once

#include "base_api.h"
#include "base/module.h"

#include <rttr/type>

#include <string>

namespace era_engine
{
	class Engine;

	class ERA_BASE_API ModuleLoader
	{
	public:
		ModuleLoader() = default;
		ModuleLoader(Engine* _engine, const std::string& _modules = std::string());
		virtual ~ModuleLoader();

		virtual bool load_module(std::string_view name);
		virtual bool unload_module(std::string_view name);

		void add_module(IModule* _module);

		bool status() const;

		RTTR_ENABLE()
			
	private:
		bool load_module_impl(const rttr::array_range<rttr::type>& types, bool is_unique = true);

	protected:
		Engine* engine = nullptr;

		std::vector<IModule*> modules;
		bool all_loaded = true;
	};

}