#include "pch.h"
#include "framework.h"
#include "EraScriptingCPPDecls.h"
#include <cstdint>

enative_scripting_builder* enative_scripting_factory::builder;

ERASCRIPTINGCPPDECLS_API enative_scripting_builder* createNativeScriptingBuilder(void)
{
    return enative_scripting_factory::builder;
}

ERASCRIPTINGCPPDECLS_API void addForce(uint32_t id, uint8_t mode, float* force)
{
    enative_caller::call("addForce", id, mode, force);
}

ERASCRIPTINGCPPDECLS_API float getMass(uint32_t id)
{
    return enative_caller::call<float>("getMass", id);
}

ERASCRIPTINGCPPDECLS_API void setMass(uint32_t id, float mass)
{
    enative_caller::call("setMass", id, mass);
}

ERASCRIPTINGCPPDECLS_API void createScript(uint32_t id, const char* name)
{
    enative_caller::call("createScript", id, name);
}

ERASCRIPTINGCPPDECLS_API void initializeRigidbody(uint32_t id, uint8_t type)
{
    enative_caller::call("initializeRigidbody", id, type);
}

ERASCRIPTINGCPPDECLS_API float* getLinearVelocity(uint32_t id)
{
    return enative_caller::call<float*>("getLinearVelocity", id);
}

ERASCRIPTINGCPPDECLS_API float* getAngularVelocity(uint32_t id)
{
    return enative_caller::call<float*>("getAngularVelocity", id);
}

ERASCRIPTINGCPPDECLS_API void setDestination(uint32_t id, float* destPtr)
{
    return enative_caller::call("setDestination", id, destPtr);
}

ERASCRIPTINGCPPDECLS_API void initializeNavigationComponent(uint32_t id, uint8_t type)
{
    return enative_caller::call("initializeNavigationComponent", id, type);
}

ERASCRIPTINGCPPDECLS_API void log_message(uint8_t mode, const char* message)
{
    enative_caller::call("log_message", mode, message);
}

enative_scripting_factory::enative_scripting_factory()
{
    if (!builder)
        builder = new enative_scripting_builder();
}

enative_scripting_factory::~enative_scripting_factory()
{
    if (builder)
        RELEASE_PTR(builder)
}
