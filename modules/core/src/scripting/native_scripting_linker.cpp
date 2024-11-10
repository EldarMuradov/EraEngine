// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "scripting/native_scripting_linker.h"

#include "core/project.h"
#include "core/string.h"

#include "scene/scene.h"

#include "ai/navigation_component.h"

#include "scripting/script.h"
#include "scripting/dotnet_utils.h"
#include "scripting/dotnet_types.h"

#include "application.h"

#include "ecs/world.h"

#include <EraScriptingCPPDecls/EraScriptingCPPDecls.h>

#include <fstream>

#include <semaphore>

namespace era_engine::dotnet
{
	std::vector<std::string> enative_scripting_linker::scriptTypes;

	namespace
	{
		start_fn startF;
		update_fn updateF;
		handle_collisions_fn handleCollisionsF;
		handle_collisions_fn handleExitCollisionsF;
		handle_trs_fn handleTrsF;

		scr_fn initScrF;
		scr_fn releaseSrcF;
		scr_fn reloadSrcF;

		handle_input_fn inputF;

		comp_fn createCompF;
		comp_fn removeCompF;
	}

	void getAllNativeFunctionsAndStart()
	{
		// Log config
		{
#if _DEBUG
			std::wofstream file((eproject::enginePath + L"/bin/Debug/core.cfg"));
#else
			std::wofstream file(eproject::enginePath + L"/_build/Release/core.cfg");
#endif
			//std::wofstream file_rt(eproject::enginePath + L"\\runtime\\x64\\Release\\bin\\Release_x86_64\\core.cfg");
			//std::wofstream file_rt_bin(eproject::enginePath + L"\\runtime\\EraRuntime\\bin\\Release_x86_64\\core.cfg");

			file << eproject::name << "," << eproject::path << "," << eproject::enginePath;
			file.close();

			//file_rt << eproject::name << "," << eproject::path << "," << eproject::enginePath;
			//file_rt.close();

			//file_rt_bin << eproject::name << "," << eproject::path << "," << eproject::enginePath;
			//file_rt_bin.close();
		}

		const char_t* scriptingCoreLibName = STR("EraScriptingCore");
		const char_t* defaultDelegateNativeName = STR("EraEngine.Call, EraScriptingCore");

		{
			const char_t* scriptingCoreClassName = STR("EraEngine.ScriptingCore");
			const char_t* scriptingCoreFullTypeName = STR("EraEngine.ScriptingCore, EraScriptingCore");

			dotnet::type_name scriptingCoreTypeName{ scriptingCoreClassName, scriptingCoreLibName };

			const char_t* scriptingCoreMainFuncName = STR("MainFunc");
			init_fn mainFunc = dotnet_host::getStaticMethod<init_fn>(scriptingCoreFullTypeName, scriptingCoreMainFuncName, defaultDelegateNativeName);

			dotnet::delegate mainFuncDelegate{ scriptingCoreTypeName, scriptingCoreMainFuncName, defaultDelegateNativeName, (void*)mainFunc };

			const char_t* initializeScriptingFuncName = STR("InitializeScripting");
			initScrF = dotnet_host::getStaticMethod<scr_fn>(scriptingCoreFullTypeName, initializeScriptingFuncName, defaultDelegateNativeName);
			dotnet::delegate initializeScriptingDelegate{ scriptingCoreTypeName, initializeScriptingFuncName, defaultDelegateNativeName, (void*)initScrF };

			const char_t* shutdownScriptingFuncName = STR("ShutdownScripting");
			releaseSrcF = dotnet_host::getStaticMethod<scr_fn>(scriptingCoreFullTypeName, shutdownScriptingFuncName, defaultDelegateNativeName);
			dotnet::delegate shutdownScriptingDelegate{ scriptingCoreTypeName, shutdownScriptingFuncName, defaultDelegateNativeName, (void*)releaseSrcF };

			const char_t* reloadScriptingFuncName = STR("ReloadScripting");
			reloadSrcF = dotnet_host::getStaticMethod<scr_fn>(scriptingCoreFullTypeName, reloadScriptingFuncName, defaultDelegateNativeName);
			dotnet::delegate reloadScriptingDelegate{ scriptingCoreTypeName, reloadScriptingFuncName, defaultDelegateNativeName, (void*)reloadSrcF };

			dotnet::core_type scriptingCoreType;
			scriptingCoreType.typeName = scriptingCoreTypeName;
			scriptingCoreType.methods.emplace(scriptingCoreMainFuncName, mainFuncDelegate);
			scriptingCoreType.methods.emplace(initializeScriptingFuncName, initializeScriptingDelegate);
			scriptingCoreType.methods.emplace(shutdownScriptingFuncName, shutdownScriptingDelegate);
			scriptingCoreType.methods.emplace(reloadScriptingFuncName, reloadScriptingDelegate);

			dotnet::storage.types.emplace(scriptingCoreClassName, scriptingCoreType);

			// Startup
			mainFunc();
		}

		{
			const char_t* elevelClassName = STR("EraEngine.ELevel");
			const char_t* elevelFullTypeName = STR("EraEngine.ELevel, EraScriptingCore");

			dotnet::type_name elevelTypeName{ elevelClassName, scriptingCoreLibName };

			const char_t* elevelStartFuncName = STR("Start");
			startF = dotnet_host::getStaticMethod<start_fn>(elevelFullTypeName, elevelStartFuncName, defaultDelegateNativeName);
			dotnet::delegate elevelStartDelegate{ elevelTypeName, elevelStartFuncName, defaultDelegateNativeName, (void*)startF };

			const char_t* elevelUpdateFuncName = STR("Update");
			updateF = dotnet_host::getStaticMethod<update_fn, float>(elevelFullTypeName, elevelUpdateFuncName, STR("EraEngine.CallUpdate, EraScriptingCore"));
			dotnet::delegate elevelUpdateDelegate{ elevelTypeName, elevelUpdateFuncName, STR("EraEngine.CallUpdate, EraScriptingCore"), (void*)updateF };

			dotnet::core_type elevelType;
			elevelType.typeName = elevelTypeName;
			elevelType.methods.emplace(elevelStartFuncName, elevelStartDelegate);
			elevelType.methods.emplace(elevelUpdateFuncName, elevelUpdateDelegate);

			dotnet::storage.types.emplace(elevelClassName, elevelType);
		}

		{
			const char_t* collisionHandlerClassName = STR("EraEngine.Core.CollisionHandler");
			const char_t* collisionHandlerFullTypeName = STR("EraEngine.Core.CollisionHandler, EraScriptingCore");
			dotnet::type_name collisionHandlerTypeName{ collisionHandlerClassName, scriptingCoreLibName };

			const char_t* chDelegateNativeName = STR("EraEngine.CallHandleColls, EraScriptingCore");

			const char_t* chHandleCollisionFuncName = STR("HandleCollision");
			handleCollisionsF = dotnet_host::getStaticMethod<handle_collisions_fn, int, int>(collisionHandlerFullTypeName, chHandleCollisionFuncName, chDelegateNativeName);
			dotnet::delegate chHandleCollisionDelegate{ collisionHandlerTypeName, chHandleCollisionFuncName, chDelegateNativeName, (void*)handleCollisionsF };

			const char_t* chHandleCollisionExitFuncName = STR("HandleExitCollision");
			handleExitCollisionsF = dotnet_host::getStaticMethod<handle_collisions_fn, int, int>(collisionHandlerFullTypeName, chHandleCollisionExitFuncName, chDelegateNativeName);
			dotnet::delegate chHandleCollisionExitDelegate{ collisionHandlerTypeName, chHandleCollisionExitFuncName, chDelegateNativeName, (void*)handleExitCollisionsF };

			dotnet::core_type collisionHandlerType;
			collisionHandlerType.typeName = collisionHandlerTypeName;
			collisionHandlerType.methods.emplace(chHandleCollisionFuncName, chHandleCollisionDelegate);
			collisionHandlerType.methods.emplace(chHandleCollisionExitFuncName, chHandleCollisionExitDelegate);

			dotnet::storage.types.emplace(collisionHandlerClassName, collisionHandlerType);
		}

		{
			const char_t* transformHandlerClassName = STR("EraEngine.Core.TransformHandler");
			const char_t* transformHandlerFullTypeName = STR("EraEngine.Core.TransformHandler, EraScriptingCore");
			dotnet::type_name transformHandlerTypeName{ transformHandlerClassName, scriptingCoreLibName };

			const char_t* thProcessTransformFuncName = STR("ProcessTransform");
			handleTrsF = dotnet_host::getStaticMethod<handle_trs_fn, intptr_t, int>(transformHandlerFullTypeName, thProcessTransformFuncName, STR("EraEngine.CallHandleTrs, EraScriptingCore"));
			dotnet::delegate thProcessTransformDelegate{ transformHandlerTypeName, thProcessTransformFuncName, STR("EraEngine.CallHandleTrs, EraScriptingCore"), (void*)handleTrsF };

			dotnet::core_type transformHandlerType;
			transformHandlerType.typeName = transformHandlerTypeName;
			transformHandlerType.methods.emplace(thProcessTransformFuncName, thProcessTransformDelegate);

			dotnet::storage.types.emplace(transformHandlerClassName, transformHandlerType);
		}

		{
			const char_t* inputHandlerFullTypeName = STR("EraEngine.Core.InputHandler, EraScriptingCore");
			const char_t* inputHandlerClassName = STR("EraEngine.Core.InputHandler");
			dotnet::type_name inputHandlerTypeName{ inputHandlerClassName, scriptingCoreLibName };

			const char_t* ihHandleInputFuncName = STR("HandleInput");
			inputF = dotnet_host::getStaticMethod<handle_input_fn, intptr_t>(inputHandlerFullTypeName, ihHandleInputFuncName, STR("EraEngine.CallHandleInput, EraScriptingCore"));
			dotnet::delegate ihHandleInputDelegate{ inputHandlerTypeName, ihHandleInputFuncName, STR("EraEngine.CallHandleInput, EraScriptingCore"), (void*)inputF };

			dotnet::core_type inputHandlerType;
			inputHandlerType.typeName = inputHandlerTypeName;
			inputHandlerType.methods.emplace(ihHandleInputFuncName, ihHandleInputDelegate);

			dotnet::storage.types.emplace(inputHandlerClassName, inputHandlerType);
		}

		{
			const char_t* componentHandlerClassName = STR("EraEngine.Runtime.ComponentHandler");
			const char_t* componentHandlerFullTypeName = STR("EraEngine.Runtime.ComponentHandler, EraScriptingCore");
			dotnet::type_name componentHandlerTypeName{ componentHandlerClassName, scriptingCoreLibName };

			const char_t* chAddComponentFuncName = STR("AddComponent");
			createCompF = dotnet_host::getStaticMethod<comp_fn, int, uintptr_t>(componentHandlerFullTypeName, chAddComponentFuncName, STR("EraEngine.CallAddComp, EraScriptingCore"));
			dotnet::delegate chAddComponentDelegate{ componentHandlerTypeName, chAddComponentFuncName, STR("EraEngine.CallAddComp, EraScriptingCore"), (void*)createCompF };

			const char_t* chRemoveComponentFuncName = STR("RemoveComponent");
			removeCompF = dotnet_host::getStaticMethod<comp_fn, int, uintptr_t>(componentHandlerFullTypeName, chRemoveComponentFuncName, STR("EraEngine.CallRemoveComp, EraScriptingCore"));
			dotnet::delegate chRemoveComponentDelegate{ componentHandlerTypeName, chRemoveComponentFuncName, STR("EraEngine.CallRemoveComp, EraScriptingCore"), (void*)removeCompF };

			dotnet::core_type componentHandlerType;
			componentHandlerType.typeName = componentHandlerTypeName;
			componentHandlerType.methods.emplace(chAddComponentFuncName, chAddComponentDelegate);
			componentHandlerType.methods.emplace(chRemoveComponentFuncName, chRemoveComponentDelegate);

			dotnet::storage.types.emplace(componentHandlerClassName, componentHandlerType);
		}
	}

	namespace bind
	{
		static escene* scene = nullptr;

		static World* world = nullptr;

		static void add_force_internal(uint32_t id, uint8_t mode, float* force)
		{
			entity_handle hid = (entity_handle)id;
			eentity entity{ hid, &scene->registry };

			if (auto rb = entity.getComponentIfExists<physics::px_dynamic_body_component>())
			{
				vec3 f = vec3(force[0], force[1], force[2]);
				rb->addForce(f, (physics::px_force_mode)mode);
				std::cout << "Force" << "\n";
			}
			else
				std::cerr << "Failed to call function addForce";
		}

		static void add_torque_internal(uint32_t id, uint8_t mode, float* torque)
		{
			entity_handle hid = (entity_handle)id;
			eentity entity{ hid, &scene->registry };

			if (auto rb = entity.getComponentIfExists<physics::px_dynamic_body_component>())
			{
				vec3 f = vec3(torque[0], torque[1], torque[2]);
				rb->addTorque(f, (physics::px_force_mode)mode);
				std::cout << "Torque" << "\n";
			}
			else
				std::cerr << "Failed to call function addTorque";
		}

		static void initialize_rigidbody_internal(uint32_t id, uint8_t type)
		{
			entity_handle hid = (entity_handle)id;
			eentity entity{ hid, &scene->registry };

			if ((physics::px_rigidbody_type)type == physics::px_rigidbody_type::rigidbody_type_dynamic)
			{
				if (!entity.hasComponent<physics::px_dynamic_body_component>())
					entity.addComponent<physics::px_dynamic_body_component>();
			}
			else if ((physics::px_rigidbody_type)type == physics::px_rigidbody_type::rigidbody_type_static)
			{
				if (!entity.hasComponent<physics::px_static_body_component>())
					entity.addComponent<physics::px_static_body_component>();
			}
			else if ((physics::px_rigidbody_type)type == physics::px_rigidbody_type::rigidbody_type_kinematic)
			{
				if (!entity.hasComponent<physics::px_dynamic_body_component>())
					entity.addComponent<physics::px_dynamic_body_component>();

				entity.getComponent<physics::px_dynamic_body_component>().setKinematic(true);
			}
		}

		static void overlap_sphere_internal(uint32_t id, uint8_t type)
		{

		}

		static float get_mass_internal(uint32_t id)
		{
			entity_handle hid = (entity_handle)id;
			eentity entity{ hid, &scene->registry };

			if (auto rb = entity.getComponentIfExists<physics::px_dynamic_body_component>())
				return rb->getMass();
			else if (auto rb = entity.getComponentIfExists<physics::px_static_body_component>())
				return rb->getMass();
			else
				return -1.0f;
		}

		static void set_mass_internal(uint32_t id, float mass)
		{
			entity_handle hid = (entity_handle)id;
			eentity entity{ hid, &scene->registry };

			if (auto rb = entity.getComponentIfExists<physics::px_dynamic_body_component>())
				rb->setMass(mass);
			else if (auto rb = entity.getComponentIfExists<physics::px_static_body_component>())
				rb->setMass(mass);
			else
				std::cerr << "Failed to call function";
		}

		static float* get_linear_velocity_internal(uint32_t id)
		{
			entity_handle hid = (entity_handle)id;
			eentity entity{ hid, &scene->registry };

			if (auto rb = entity.getComponentIfExists<physics::px_dynamic_body_component>())
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

		static uint32_t create_entity_internal(const char* name)
		{
			return (uint32_t)scene->createEntity(name)
				.addComponent<transform_component>(vec3(0.f), quat::identity)
				.handle;
		}

		static float* get_angular_velocity_internal(uint32_t id)
		{
			entity_handle hid = (entity_handle)id;
			eentity entity{ hid, &scene->registry };

			if (auto rb = entity.getComponentIfExists<physics::px_dynamic_body_component>())
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
			if (!world)
			{
				return;
			}

			Entity::Handle hid = (Entity::Handle)id;
			Entity entity = world->get_entity(hid);

			if (entity == Entity::Null)
			{
				return;
			}

			if (auto script = entity.get_component_if_exists<ecs::ScriptsComponent>())
			{
				script->type_names.emplace(name);
			}
			else
			{
				entity.add_component<ecs::ScriptsComponent>(name);
			}
		}

		static void remove_component_internal(uint32_t id, const char* name)
		{
			if (!world)
			{
				return;
			}

			Entity::Handle hid = (Entity::Handle)id;
			Entity entity = world->get_entity(hid);

			if (entity == Entity::Null)
			{
				return;
			}

			// TODO: Removing all internal components
			if (auto comp = entity.get_component_if_exists<ecs::ScriptsComponent>())
			{
				comp->type_names.clear();
			}
		}

		static void create_component_internal(uint32_t id, const char* name)
		{

		}

		static void instantiate_internal(uint32_t id, uint32_t newId, uint32_t parentId)
		{

		}

		static void release_internal(uint32_t id)
		{
			entity_handle hid = (entity_handle)id;

			scene->deleteEntity(hid);
		}

		static void setActive_internal(uint32_t id, bool active)
		{

		}

		static void initialize_navigation_internal(uint32_t id, uint8_t type)
		{
			using namespace era_engine::ai;

			entity_handle hid = (entity_handle)id;
			eentity entity{ hid, &scene->registry };

			if (!entity.hasComponent<navigation_component>())
				entity.addComponent<navigation_component>((nav_type)type);
		}

		static void set_destination_internal(uint32_t id, float* destPtr)
		{
			using namespace era_engine::ai;

			entity_handle hid = (entity_handle)id;
			eentity entity{ hid, &scene->registry };

			if (auto nav_comp = entity.getComponentIfExists<navigation_component>())
			{
				vec3 pos = vec3(destPtr[0], 0.0f, destPtr[2]);
				nav_comp->destination = pos;
			}
			else
			{
				std::cerr << "Failed to call function\n";
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

		static void send_type_internal(const char* type)
		{
			std::cout << "Type found: " << type << "\n";
			enative_scripting_linker::scriptTypes.push_back(type);
		}
	}

	enative_scripting_linker::~enative_scripting_linker()
	{
		release();
	}

	void enative_scripting_linker::init()
	{
		lib = LoadLibraryA("EraScriptingCPPDecls.dll");
		if (!lib || lib == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Failed to load scripting impl library";
			return;
		}

		get_builder getBuilderFuncInstance = (get_builder)GetProcAddress(lib, "createNativeScriptingBuilder");
		builder = ref<enative_scripting_builder>{ getBuilderFuncInstance() };

		bindFunctions();

		host.initHost();
	}

	void enative_scripting_linker::release()
	{
		if (lib)
			FreeLibrary(lib);
		host.releaseHost();
	}

	void enative_scripting_linker::start()
	{
		if (startF && !started)
		{
			startF();
			started = true;
		}
	}

	void enative_scripting_linker::update(float dt)
	{
		if (updateF)
			updateF(dt);
	}

	void enative_scripting_linker::handle_coll(int id1, int id2)
	{
		if (handleCollisionsF)
			handleCollisionsF(id1, id2);
	}

	void enative_scripting_linker::handle_exit_coll(int id1, int id2)
	{
		if (handleExitCollisionsF)
			handleExitCollisionsF(id1, id2);
	}

	void enative_scripting_linker::process_trs(intptr_t ptr, int id)
	{
		if (handleTrsF)
			handleTrsF(ptr, id);
	}

	void enative_scripting_linker::init_src()
	{
		if (initScrF)
			initScrF();
	}

	void enative_scripting_linker::release_src()
	{
		if (!started)
			return;
		if (releaseSrcF)
			releaseSrcF();
	}

	void enative_scripting_linker::reload_src()
	{
		scriptTypes.clear();
		if (reloadSrcF)
			reloadSrcF();
	}

	void enative_scripting_linker::handleInput(uintptr_t input)
	{
		try
		{
			if (inputF)
				inputF(input);
		}
		catch (const std::exception& ex)
		{
			std::cout << ex.what() << "\n";
		}
	}

	void enative_scripting_linker::createScript(int id, const char* comp)
	{
		if (createCompF)
			createCompF(id, reinterpret_cast<uintptr_t>(comp));
	}

	void enative_scripting_linker::removeScript(int id, const char* comp)
	{
		if (removeCompF)
			removeCompF(id, reinterpret_cast<uintptr_t>(comp));
	}

	void enative_scripting_linker::bindFunctions()
	{
		if (builder)
		{
			bind::scene = runtimeScene;

			// Rigidbody
			{
				builder->functions.emplace("getLinearVelocity", BIND(bind::get_linear_velocity_internal, float*, uint32_t));
				builder->functions.emplace("getAngularVelocity", BIND(bind::get_angular_velocity_internal, float*, uint32_t));
				builder->functions.emplace("initializeRigidbody", BIND(bind::initialize_rigidbody_internal, void, uint32_t, uint8_t));
				builder->functions.emplace("getMass", BIND(bind::get_mass_internal, float, uint32_t));
				builder->functions.emplace("setMass", BIND(bind::set_mass_internal, void, uint32_t, float));
				builder->functions.emplace("addForce", BIND(bind::add_force_internal, void, uint32_t, uint8_t, float*));
				builder->functions.emplace("addTorque", BIND(bind::add_torque_internal, void, uint32_t, uint8_t, float*));
			}

			// Navigation
			{
				builder->functions.emplace("initializeNavigationComponent", BIND(bind::initialize_navigation_internal, void, uint32_t, int8_t));
				builder->functions.emplace("setDestination", BIND(bind::set_destination_internal, void, uint32_t, float*));
			}

			// Debug
			{
				builder->functions.emplace("logMessage", BIND(bind::log_message_internal, void, uint8_t, const char*));
			}

			// User Scripting
			{
				builder->functions.emplace("sendType", BIND(bind::send_type_internal, void, const char*));
			}

			// EEntity
			{
				builder->functions.emplace("createScript", BIND(bind::create_script_internal, void, uint32_t, const char*));
				builder->functions.emplace("createComponent", BIND(bind::create_component_internal, void, uint32_t, const char*));
				builder->functions.emplace("removeComponent", BIND(bind::remove_component_internal, void, uint32_t, const char*));
				builder->functions.emplace("instantiate", BIND(bind::instantiate_internal, void, uint32_t, uint32_t, uint32_t));
				builder->functions.emplace("release", BIND(bind::release_internal, void, uint32_t));
				builder->functions.emplace("setActive", BIND(bind::setActive_internal, void, uint32_t, bool));
			}

			// EEntityManager
			{
				builder->functions.emplace("createEntity", BIND(bind::create_entity_internal, uint32_t, const char*));
			}
		}
	}

}