#include "ecs/system.h"

namespace era_engine
{

	System::System(World* _world)
		: world(_world)
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