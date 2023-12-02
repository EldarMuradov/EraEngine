#ifdef ERASCRIPTINGCPPDECLS_EXPORTS
#define ERASCRIPTINGCPPDECLS_API __declspec(dllexport)
#else
#define ERASCRIPTINGCPPDECLS_API __declspec(dllimport)
#endif

#include <functional>
#include <any>
#include <string>

template<typename... Args>
using void_func = std::function<void(Args...)>;

struct ERASCRIPTINGCPPDECLS_API enative_scripting_builder
{
	enative_scripting_builder();

	template<typename TResult, typename... Args>
	struct func_obj
	{
		func_obj(std::function<TResult(Args...)> f)
		{
			fn = f;
		}

		TResult operator() (Args... args)
		{
			fn(args ...);
		}

		std::function<TResult(Args...)> fn;
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

extern "C" ERASCRIPTINGCPPDECLS_API enative_scripting_builder* createNativeScriptingBuilder(void);

extern "C" ERASCRIPTINGCPPDECLS_API void addForce(uint32_t id, uint32_t mode, float* force);