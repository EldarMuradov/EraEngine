#include "base/system.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;

		rttr::registration::class_<System>("System")
			.constructor<void*>()(policy::ctor::as_raw_ptr);
	}

	UpdateGroup::UpdateGroup(const char* _name, UpdateType _update_type) noexcept 
		: name(_name), update_type(_update_type) 
	{
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