// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "EraScriptingCPPDecls/framework.h"

#include <functional>
#include <any>
#include <string>
#include <unordered_map>

namespace era_engine
{
	template<typename... Args_>
	using void_func = std::function<void(Args_&&...)>;

	struct enative_scripting_builder
	{
		enative_scripting_builder() = default;
		~enative_scripting_builder() = default;

		template<typename Result_, typename... Args_>
		struct func_obj
		{
			func_obj(std::function<Result_(Args_&&...)> f)
			{
				fn = f;
			}

			Result_ operator() (Args_&&... args)
			{
				return fn(args...);
			}

			std::function<Result_(Args_&&...)> fn;
		};

		struct func
		{
			void* obj = nullptr;
		};

		std::unordered_map<std::string, func> functions;
	};

	struct ERASCRIPTINGCPPDECLS_API enative_scripting_factory
	{
		enative_scripting_factory();
		~enative_scripting_factory();

		static enative_scripting_builder* builder;
	};

	struct ERASCRIPTINGCPPDECLS_API enative_caller
	{
		template<typename Result_, typename... Args_>
		static Result_ call(const char* name, Args_&&... args)
		{
			return (*((enative_scripting_builder::func_obj<Result_, Args_&&...>*)enative_scripting_factory::builder->functions[name].obj))(args...);
		}

		template<typename... Args_>
		static void call(const char* name, Args_&&... args)
		{
			(*((enative_scripting_builder::func_obj<void, Args_&&...>*)enative_scripting_factory::builder->functions[name].obj))(args...);
		}

	private:
		enative_caller() = delete;
	};
}

EEXTERN ERASCRIPTINGCPPDECLS_API era_engine::enative_scripting_builder* createNativeScriptingBuilder(void);

// Rigidbody
EEXTERN ERASCRIPTINGCPPDECLS_API void addForce(uint32_t id, uint8_t mode, float* force);
EEXTERN ERASCRIPTINGCPPDECLS_API void addTorque(uint32_t id, uint8_t mode, float* torque);
EEXTERN ERASCRIPTINGCPPDECLS_API float getMass(uint32_t id);
EEXTERN ERASCRIPTINGCPPDECLS_API void setMass(uint32_t id, float mass);
EEXTERN ERASCRIPTINGCPPDECLS_API void initializeRigidbody(uint32_t id, uint8_t type);
EEXTERN ERASCRIPTINGCPPDECLS_API float* getLinearVelocity(uint32_t id);
EEXTERN ERASCRIPTINGCPPDECLS_API float* getAngularVelocity(uint32_t id);

//Navigation
EEXTERN ERASCRIPTINGCPPDECLS_API void setDestination(uint32_t id, float* destPtr);
EEXTERN ERASCRIPTINGCPPDECLS_API void initializeNavigationComponent(uint32_t id, uint8_t type);

// Debug
EEXTERN ERASCRIPTINGCPPDECLS_API void logMessage(uint8_t mode, const char* message);

// User Scripting
EEXTERN ERASCRIPTINGCPPDECLS_API void sendType(const char* type);

// EEntity
EEXTERN ERASCRIPTINGCPPDECLS_API void createScript(uint32_t id, const char* name);
EEXTERN ERASCRIPTINGCPPDECLS_API void release(uint32_t id);
EEXTERN ERASCRIPTINGCPPDECLS_API void instantiate(uint32_t id, uint32_t newId, uint32_t parentId);
EEXTERN ERASCRIPTINGCPPDECLS_API void setActive(uint32_t id, bool active);
EEXTERN ERASCRIPTINGCPPDECLS_API void createComponent(uint32_t id, const char* name);
EEXTERN ERASCRIPTINGCPPDECLS_API void removeComponent(uint32_t id, const char* name);

EEXTERN ERASCRIPTINGCPPDECLS_API uint32_t createEntity(const char* name);
