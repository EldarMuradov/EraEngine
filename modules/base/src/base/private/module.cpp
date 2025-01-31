#include "base/module.h"

#include <rttr/registration>

namespace era_engine
{

	IModule::IModule(const char* _name) noexcept
		: name(_name)
	{
	}

	bool IModule::initialize(void* engine)
	{
		state = State::LOADED;
		return true;
	}

	bool IModule::terminate()
	{
		state = State::UNLOADED;
		return true;
	}

	IModule::State IModule::get_state() const
	{
		return state;
	}

	const char* IModule::get_name() const
	{
		return name;
	}

	RTTR_PLUGIN_REGISTRATION
	{
		using namespace rttr;
		registration::class_<IModule>("IModule")
			.constructor<>()
			.constructor<const char*>()
			.method("initialize", &IModule::initialize)
			.method("terminate", &IModule::terminate)
			.method("get_name", &IModule::get_name)
			.method("get_state", &IModule::get_state)
			(metadata("module_type", IModule::Type::NONE));
	}

}