#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#include "luauscript_register.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "nobind.h"

#include "luau_constants.h"
#include "luauscript_resource_format.h"
#include "luauscript_syntax_highlighter.h"
#include "luau_engine.h"
#include "luau_script.h"
#include "luau_plugin.h"
#include "lamda_wrapper.h"

using namespace godot;

LuauLanguage *script_language_luau = nullptr;
Ref<ResourceFormatLoaderLuau> resource_loader_luau;
Ref<ResourceFormatSaverLuau> resource_saver_luau;

void initialize_luau_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
        GDREGISTER_INTERNAL_CLASS(LambdaWrapper);
        GDREGISTER_INTERNAL_CLASS(LuaFunctionWrapper);

        UtilityFunctions::print("[LuauGDExtension] Initializing Extension");
        GDREGISTER_INTERNAL_CLASS(LuauScript);

        GDREGISTER_INTERNAL_CLASS(LuauLanguage);
        script_language_luau = memnew(LuauLanguage);
        Error reg_script_lang = nobind::Engine::get_singleton()->register_script_language(script_language_luau);
        ERR_FAIL_COND_MSG(reg_script_lang != OK,
            "Failed to register Luau language.");
        UtilityFunctions::print("[LuauGDExtension] Luau Script Language registered successfully");
    } 
    else if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {

        GDREGISTER_INTERNAL_CLASS(ResourceFormatLoaderLuau);
        resource_loader_luau.instantiate();
        ERR_FAIL_COND_MSG(!resource_loader_luau.is_valid(), 
            "Failed to instantiate Luau resource loader.");
        nobind::ResourceLoader::get_singleton()->add_resource_format_loader(resource_loader_luau);
        UtilityFunctions::print("[LuauGDExtension] Resource Loader registered successfully");

        GDREGISTER_INTERNAL_CLASS(ResourceFormatSaverLuau);
        resource_saver_luau.instantiate();
        ERR_FAIL_COND_MSG(!resource_saver_luau.is_valid(), 
            "Failed to instantiate Luau resource saver.");
        nobind::ResourceSaver::get_singleton()->add_resource_format_saver(resource_saver_luau);
        UtilityFunctions::print("[LuauGDExtension] Resource Saver registered successfully");
    }

    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        GDREGISTER_INTERNAL_CLASS(LuauPlugin);
        GDREGISTER_INTERNAL_CLASS(LuauSyntaxHighlighter);

        EditorPlugins::add_by_type<LuauPlugin>();
        UtilityFunctions::print("[LuauGDExtension] Editor Plugin registered successfully");

    }
}

void uninitialize_luau_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        nobind::Engine::get_singleton()->unregister_script_language(script_language_luau);

        if (script_language_luau) {
            memdelete(script_language_luau);
        }

        nobind::ResourceLoader::get_singleton()->remove_resource_format_loader(resource_loader_luau);
        resource_loader_luau.unref();
    
        nobind::ResourceSaver::get_singleton()->remove_resource_format_saver(resource_saver_luau);
        resource_saver_luau.unref();
    }
}

void startup_luau_module() {
    PackedStringArray args = OS::get_singleton()->get_cmdline_args();
    bool run_tests = false;
    PackedStringArray doctest_args;
    for (int i = 0; i < args.size(); i++) {
        if (args[i] == "--run-extension-tests") {
            run_tests = true;
            continue;
        }
        if (run_tests) {
            doctest_args.append(args[i]);
        }
    }
    if (run_tests) {
        UtilityFunctions::print("[LuauGDExtension] Running tests");

        Vector<CharString> storage;
        for (int i = 0; i < doctest_args.size(); i++) {
            storage.push_back(doctest_args[i].utf8());
        }

        Vector<const char *> argv;
        argv.push_back("godot");
        for (int i = 0; i < storage.size(); i++) {
            argv.push_back(storage[i].get_data());
        }

        doctest::Context ctx(argv.size(), argv.ptr());
        ctx.run();

        UtilityFunctions::print("[LuauGDExtension] Tests finished");
    }
}