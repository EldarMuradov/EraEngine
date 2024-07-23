// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "EraScriptingCPPDecls/EraScriptingCPPDecls.h"

#include <cstdint>

era_engine::enative_scripting_builder* era_engine::enative_scripting_factory::builder;

ERASCRIPTINGCPPDECLS_API era_engine::enative_scripting_builder* createNativeScriptingBuilder(void)
{
    return era_engine::enative_scripting_factory::builder;
}

ERASCRIPTINGCPPDECLS_API void addForce(uint32_t id, uint8_t mode, float* force)
{
    era_engine::enative_caller::call("addForce", id, mode, force);
}

ERASCRIPTINGCPPDECLS_API void addTorque(uint32_t id, uint8_t mode, float* torque)
{
    era_engine::enative_caller::call("addTorque", id, mode, torque);
}

ERASCRIPTINGCPPDECLS_API float getMass(uint32_t id)
{
    return era_engine::enative_caller::call<float>("getMass", id);
}

ERASCRIPTINGCPPDECLS_API void setMass(uint32_t id, float mass)
{
    era_engine::enative_caller::call("setMass", id, mass);
}

ERASCRIPTINGCPPDECLS_API void createScript(uint32_t id, const char* name)
{
    era_engine::enative_caller::call("createScript", id, name);
}

ERASCRIPTINGCPPDECLS_API void release(uint32_t id)
{
    era_engine::enative_caller::call("release", id);
}

ERASCRIPTINGCPPDECLS_API void setActive(uint32_t id, bool active)
{
    era_engine::enative_caller::call("setActive", id, active);
}

ERASCRIPTINGCPPDECLS_API void instantiate(uint32_t id, uint32_t newId, uint32_t parentId)
{
    era_engine::enative_caller::call("instantiate", id, newId, parentId);
}

ERASCRIPTINGCPPDECLS_API void createComponent(uint32_t id, const char* name)
{
    era_engine::enative_caller::call("createComponent", id, name);
}

ERASCRIPTINGCPPDECLS_API void removeComponent(uint32_t id, const char* name)
{
    era_engine::enative_caller::call("removeComponent", id, name);
}

ERASCRIPTINGCPPDECLS_API uint32_t createEntity(const char* name)
{
    return era_engine::enative_caller::call<uint32_t>("createEntity", name);
}

ERASCRIPTINGCPPDECLS_API void initializeRigidbody(uint32_t id, uint8_t type)
{
    era_engine::enative_caller::call("initializeRigidbody", id, type);
}

ERASCRIPTINGCPPDECLS_API float* getLinearVelocity(uint32_t id)
{
    return era_engine::enative_caller::call<float*>("getLinearVelocity", id);
}

ERASCRIPTINGCPPDECLS_API float* getAngularVelocity(uint32_t id)
{
    return era_engine::enative_caller::call<float*>("getAngularVelocity", id);
}

ERASCRIPTINGCPPDECLS_API void setDestination(uint32_t id, float* destPtr)
{
    return era_engine::enative_caller::call("setDestination", id, destPtr);
}

ERASCRIPTINGCPPDECLS_API void initializeNavigationComponent(uint32_t id, uint8_t type)
{
    return era_engine::enative_caller::call("initializeNavigationComponent", id, type);
}

ERASCRIPTINGCPPDECLS_API void logMessage(uint8_t mode, const char* message)
{
    era_engine::enative_caller::call("logMessage", mode, message);
}

ERASCRIPTINGCPPDECLS_API void sendType(const char* type)
{
    era_engine::enative_caller::call("sendType", type);
}

era_engine::enative_scripting_factory::enative_scripting_factory()
{
    if (!builder)
        builder = new era_engine::enative_scripting_builder();
}

era_engine::enative_scripting_factory::~enative_scripting_factory()
{
    if (builder)
        RELEASE_PTR(builder)
}
