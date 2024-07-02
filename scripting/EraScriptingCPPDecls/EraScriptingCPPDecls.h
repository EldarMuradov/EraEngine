// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include <functional>
#include <any>
#include <string>

#ifdef ERASCRIPTINGCPPDECLS_EXPORTS
#define ERASCRIPTINGCPPDECLS_API __declspec(dllexport)
#else
#define ERASCRIPTINGCPPDECLS_API __declspec(dllimport)
#endif

#ifndef EEXTERN
#define EEXTERN extern "C"
#endif

namespace era_engine
{
	template<typename... Args>
	using void_func = std::function<void(Args&&...)>;

	struct ERASCRIPTINGCPPDECLS_API enative_scripting_builder
	{
		enative_scripting_builder() = default;

		template<typename TResult, typename... Args>
		struct func_obj
		{
			func_obj(std::function<TResult(Args&&...)> f)
			{
				fn = f;
			}

			TResult operator() (Args&&... args)
			{
				return fn(args...);
			}

			std::function<TResult(Args&&...)> fn;
		};

		struct func
		{
			void* obj = nullptr;
		};

		std::unordered_map<std::string, func> functions;
	};

	struct ERASCRIPTINGCPPDECLS_API enative_scripting_factory
	{
		enative_scripting_factory() noexcept;
		~enative_scripting_factory();

		static enative_scripting_builder* builder;
	};

	struct ERASCRIPTINGCPPDECLS_API enative_caller
	{
		template<typename TResult, typename... Args>
		static TResult call(const char* name, Args&&... args) noexcept
		{
			return (*((enative_scripting_builder::func_obj<TResult, Args&&...>*)enative_scripting_factory::builder->functions[name].obj))(args...);
		}

		template<typename... Args>
		static void call(const char* name, Args&&... args) noexcept
		{
			(*((enative_scripting_builder::func_obj<void, Args&&...>*)enative_scripting_factory::builder->functions[name].obj))(args...);
		}

	private:
		enative_caller() = delete;
	};
}

EEXTERN ERASCRIPTINGCPPDECLS_API era_engine::enative_scripting_builder* createNativeScriptingBuilder(void) noexcept;

// Rigidbody
EEXTERN ERASCRIPTINGCPPDECLS_API void addForce(uint32_t id, uint8_t mode, float* force) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API void addTorque(uint32_t id, uint8_t mode, float* torque) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API float getMass(uint32_t id) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API void setMass(uint32_t id, float mass) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API void initializeRigidbody(uint32_t id, uint8_t type) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API float* getLinearVelocity(uint32_t id) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API float* getAngularVelocity(uint32_t id) noexcept;

//Navigation
EEXTERN ERASCRIPTINGCPPDECLS_API void setDestination(uint32_t id, float* destPtr) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API void initializeNavigationComponent(uint32_t id, uint8_t type) noexcept;

// Debug
EEXTERN ERASCRIPTINGCPPDECLS_API void logMessage(uint8_t mode, const char* message) noexcept;

// User Scripting
EEXTERN ERASCRIPTINGCPPDECLS_API void sendType(const char* type) noexcept;

// EEntity
EEXTERN ERASCRIPTINGCPPDECLS_API void createScript(uint32_t id, const char* name) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API void release(uint32_t id) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API void instantiate(uint32_t id, uint32_t newId, uint32_t parentId) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API void setActive(uint32_t id, bool active) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API void createComponent(uint32_t id, const char* name) noexcept;
EEXTERN ERASCRIPTINGCPPDECLS_API void removeComponent(uint32_t id, const char* name) noexcept;

EEXTERN ERASCRIPTINGCPPDECLS_API uint32_t createEntity(const char* name) noexcept;
