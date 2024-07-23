include(${CMAKE_SOURCE_DIR}/cmake/era_engine_common.cmake)

era_begin(EraScriptingCPPDecls)
    declare_module(EraScriptingCPPDecls)

    target_compile_definitions(EraScriptingCPPDecls PRIVATE ERA_SCRIPTING_CPP_DECLS)
    target_compile_definitions(EraScriptingCPPDecls PRIVATE ERASCRIPTINGCPPDECLS_EXPORTS)
era_end(EraScriptingCPPDecls)