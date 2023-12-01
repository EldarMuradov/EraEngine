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

template<typename T, typename R, typename... Args>
struct Functor
{
    typedef R(T::* FT)(Args ...);
	Functor(FT fn = nullptr) : fn(fn) {  }

    R operator() (Args... args)
    {
        fn(args ...);
    }

    FT fn;
};

class ERASCRIPTINGCPPDECLS_API CEraScriptingCPPDecls 
{
public:
	CEraScriptingCPPDecls(void);
};

struct ERASCRIPTINGCPPDECLS_API enative_scripting_builder
{
	enative_scripting_builder();

	constexpr bool isValid() const noexcept { return true; }

	void runAll() 
	{ 
		uint32_t id = 0, mode = 1;
		float* force = new float[3] {0.0, 1.5, 0.0};
		functions["addForce"](id, mode, force); 
	}

	std::unordered_map<std::string, void_func<uint32_t, uint32_t, float*>> functions; //TODO: fix it to any types
};

struct ERASCRIPTINGCPPDECLS_API enative_scripting_factory
{
	enative_scripting_factory();
	~enative_scripting_factory();

	static enative_scripting_builder* builder;
};

extern "C" ERASCRIPTINGCPPDECLS_API enative_scripting_builder* createNativeScriptingBuilder(void);

extern "C" ERASCRIPTINGCPPDECLS_API void addForce(uint32_t id, uint32_t mode, float* force);