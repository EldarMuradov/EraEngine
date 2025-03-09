#pragma once

#include "core_api.h"

namespace era_engine
{
	class RegistrableVar;

	class ERA_CORE_API DebugVarsStorage final
	{
	public:
		DebugVarsStorage();

		static DebugVarsStorage* get_instance();

		void register_var(RegistrableVar* var);

		std::vector<RegistrableVar*>& get_vars();

		void reset_all();

		void draw_vars(bool& opened);

	private:
		std::vector<RegistrableVar*> vars;
	};
}