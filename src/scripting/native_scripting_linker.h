// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "EraScriptingCPPDecls/EraScriptingCPPDecls.h"

#include <scripting/dotnet_host.h>
#include <scene/scene.h>

typedef HMODULE elib; //.NET library instance
typedef enative_scripting_builder* (*get_builder)();

//Inject function into scripting endpoints
#define BIND(fn, result, ...) enative_scripting_builder::func{ new enative_scripting_builder::func_obj<result, __VA_ARGS__>(fn)}

struct enative_scripting_linker
{
	enative_scripting_linker(escene* scene) : runtimeScene(scene) {};
	~enative_scripting_linker();

	void init();
	void release();

	void start();
	void update(float dt);

	void handle_coll(int id1, int id2);
	void handle_exit_coll(int id1, int id2);
	void process_trs(intptr_t ptr, int id);

	void init_src();
	void release_src();
	void reload_src();

	void handleInput(uintptr_t input);

	static void createScript(int id, const char* comp);
	static void removeScript(int id, const char* comp);

	escene* runtimeScene = nullptr;

	static std::vector<std::string> scriptTypes;

private:
	void bindFunctions();

	dotnet_host host;

	elib lib = nullptr;

	enative_scripting_builder* builder = nullptr;
};