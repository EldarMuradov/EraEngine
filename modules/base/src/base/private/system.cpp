#include "base/system.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<System>("System")
			.constructor<void*>()(policy::ctor::as_raw_ptr);
	}

	std::unordered_map<std::string, UpdateGroup*> UpdatesHolder::global_groups;
	std::vector<std::string> UpdatesHolder::update_order;

	UpdateGroup::UpdateGroup(const char* _name, UpdateType _update_type, bool _main_thread/* = false*/) noexcept
		: name(_name), update_type(_update_type), main_thread(_main_thread)
	{
		UpdatesHolder::global_groups.emplace(std::string(_name), this);
	}

	System::System(World* _world)
		: world(_world)
	{
	}

	System::System(void* _world)
		: world(static_cast<World*>(_world))
	{
	}

	System::~System()
	{
	}

	void System::init()
	{
	}

	void System::update(float dt)
	{
	}
}