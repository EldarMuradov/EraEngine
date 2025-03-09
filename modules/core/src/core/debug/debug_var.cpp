#include "core/debug/debug_var.h"
#include "core/debug/debug_var_storage.h"

#include <rttr/registration>

namespace era_engine
{

	RegistrableVar::RegistrableVar(const std::string& _name)
		: name(_name)
	{
		DebugVarsStorage::get_instance()->register_var(this);
	}

	RegistrableVar::~RegistrableVar()
	{
	}

}