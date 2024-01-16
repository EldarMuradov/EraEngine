#ifdef ERASCRIPTINGCPPDECLS_EXPORTS
#define ERASCRIPTINGCPPDECLS_API __declspec(dllexport)
#else
#define ERASCRIPTINGCPPDECLS_API __declspec(dllimport)
#endif

#define EEXTERN extern "C"

#include <functional>
#include <any>
#include <string>

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
	enative_scripting_factory();
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

EEXTERN ERASCRIPTINGCPPDECLS_API enative_scripting_builder* createNativeScriptingBuilder(void);

// Rigidbody
EEXTERN ERASCRIPTINGCPPDECLS_API void addForce(uint32_t id, uint8_t mode, float* force);
EEXTERN ERASCRIPTINGCPPDECLS_API float getMass(uint32_t id);
EEXTERN ERASCRIPTINGCPPDECLS_API void setMass(uint32_t id, float mass);
EEXTERN ERASCRIPTINGCPPDECLS_API void initializeRigidbody(uint32_t id, uint8_t type);
EEXTERN ERASCRIPTINGCPPDECLS_API float* getLinearVelocity(uint32_t id);
EEXTERN ERASCRIPTINGCPPDECLS_API float* getAngularVelocity(uint32_t id);

//Navigation
EEXTERN ERASCRIPTINGCPPDECLS_API void setDestination(uint32_t id, float* destPtr);
EEXTERN ERASCRIPTINGCPPDECLS_API void initializeNavigationComponent(uint32_t id, uint8_t type);

// Debug
EEXTERN ERASCRIPTINGCPPDECLS_API void log_message(uint8_t mode, const char* message);

// User Scripting
EEXTERN ERASCRIPTINGCPPDECLS_API void sendType(const char* type);

// EEntity
EEXTERN ERASCRIPTINGCPPDECLS_API void createScript(uint32_t id, const char* name);
EEXTERN ERASCRIPTINGCPPDECLS_API void createComponent(uint32_t id, const char* name);
EEXTERN ERASCRIPTINGCPPDECLS_API void removeComponent(uint32_t id, const char* name);
