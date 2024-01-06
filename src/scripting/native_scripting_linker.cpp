#include "pch.h"
#include "native_scripting_linker.h"
#include <core/log.h>
#include <scene/scene.h>
#include "application.h"
#include <EraScriptingLauncher-Lib/src/script.h>
#include <ai/navigation_component.h>

application* enative_scripting_linker::app;

namespace bind
{
	static void add_force_internal(uint32_t id, uint8_t mode, float* force)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto rb = entity.getComponentIfExists<px_rigidbody_component>())
		{
			vec3 f = vec3(force[0], force[1], force[2]);
			rb->addForce(f, (px_force_mode)mode);
			std::cout << "Force" << "\n";
		}
		else
			std::cerr << "bliaaaa addForce";
	}

	static void initialize_rigidbody_internal(uint32_t id, uint8_t type)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (!entity.hasComponent<px_rigidbody_component>())
			entity.addComponent<px_rigidbody_component>((px_rigidbody_type)type);
	}

	static float get_mass_internal(uint32_t id)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto rb = entity.getComponentIfExists<px_rigidbody_component>())
			return rb->getMass();
		else
			return -1.0f;
	}

	static void set_mass_internal(uint32_t id, float mass)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto rb = entity.getComponentIfExists<px_rigidbody_component>())
			rb->setMass(mass);
		else
			std::cerr << "bliaaaa";
	}

	static float* get_linear_velocity_internal(uint32_t id)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto rb = entity.getComponentIfExists<px_rigidbody_component>())
		{
			float* f = new float[3];
			f[0] = rb->getLinearVelocity().data[0];
			f[1] = rb->getLinearVelocity().data[1];
			f[2] = rb->getLinearVelocity().data[2];
			return f;
		}
		else
			return new float[3];
	}

	static float* get_angular_velocity_internal(uint32_t id)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto rb = entity.getComponentIfExists<px_rigidbody_component>())
		{
			float* f = new float[3];
			f[0] = rb->getAngularVelocity().data[0];
			f[1] = rb->getAngularVelocity().data[1];
			f[2] = rb->getAngularVelocity().data[2];
			return f;
		}
		else
			return new float[3];
	}

	static void create_script_internal(uint32_t id, const char* name)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto script = entity.getComponentIfExists<script_component>())
		{
			script->typeNames.push_back(name);
		}
		else
		{
			entity.addComponent<script_component>(name);
		}
	}

	static void initialize_navigation_internal(uint32_t id, uint8_t type)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (!entity.hasComponent<navigation_component>())
			entity.addComponent<navigation_component>((nav_type)type);
	}

	static void set_destination_internal(uint32_t id, float* destPtr)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto nav_comp = entity.getComponentIfExists<navigation_component>())
		{
			vec3 pos = vec3(destPtr[0], 0.0f, destPtr[2]);
			nav_comp->destination = pos;
		}
		else
		{
			std::cerr << "bliaaaaaaa\n";
		}
	}

	static void log_message_internal(uint8_t mode, const char* message)
	{
		message_type type = (message_type)mode;

		if (type == message_type_warning)
			LOG_WARNING(message);
		else if (type == message_type_error)
			LOG_ERROR(message);
		else
			LOG_MESSAGE(message);
	}
}

void enative_scripting_linker::init()
{
	lib = LoadLibraryA("EraScriptingCPPDecls.dll");
	if (!lib || lib == INVALID_HANDLE_VALUE)
	{
		std::cerr << "bliaaaa";
		return;
	}

	get_builder get_builder_func_instance = (get_builder)GetProcAddress(lib, "createNativeScriptingBuilder");
	builder = get_builder_func_instance();

	bindFunctions();
}

void enative_scripting_linker::release()
{
	RELEASE_PTR(builder)
	if (lib)
		FreeLibrary(lib);
}

void enative_scripting_linker::bindFunctions()
{
	if (builder)
	{
		// Rigidbody
		{
			builder->functions.emplace("getLinearVelocity", BIND(bind::get_linear_velocity_internal, float*, uint32_t));
			builder->functions.emplace("getAngularVelocity", BIND(bind::get_angular_velocity_internal, float*, uint32_t));
			builder->functions.emplace("initializeRigidbody", BIND(bind::initialize_rigidbody_internal, void, uint32_t, uint8_t));
			builder->functions.emplace("getMass", BIND(bind::get_mass_internal, float, uint32_t));
			builder->functions.emplace("setMass", BIND(bind::set_mass_internal, void, uint32_t, float));
			builder->functions.emplace("addForce", BIND(bind::add_force_internal, void, uint32_t, uint8_t, float*));
		}

		// Navigation
		{
			builder->functions.emplace("initializeNavigationComponent", BIND(bind::initialize_navigation_internal, void, uint32_t, int8_t));
			builder->functions.emplace("setDestination", BIND(bind::set_destination_internal, void, uint32_t, float*));
		}

		// Debug
		{
			builder->functions.emplace("log_message", BIND(bind::log_message_internal, void, uint8_t, const char*));
		}
		
		// EEntity
		{
			builder->functions.emplace("createScript", BIND(bind::create_script_internal, void, uint32_t, const char*));
		}
	}
}
