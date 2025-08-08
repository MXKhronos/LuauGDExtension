#include "luauscript_register.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

#include "nobind.h"

#include "luau_constants.h"
#include "luauscript_resource_format.h"
#include "luauscript_syntax_highlighter.h"
#include "luau_engine.h"
#include "luau_script.h"
#include "luau_plugin.h"

using namespace godot;

LuauLanguage *script_language_luau = nullptr;
Ref<ResourceFormatLoaderLuau> resource_loader_luau;
Ref<ResourceFormatSaverLuau> resource_saver_luau;

void godot::initialize_luau_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
        WARN_PRINT("[LuauGDExtension] Initializing Extension");
        GDREGISTER_CLASS(LuauScript);

        GDREGISTER_CLASS(LuauLanguage);
        script_language_luau = memnew(LuauLanguage);
        Error reg_script_lang = nobind::Engine::get_singleton()->register_script_language(script_language_luau);
        ERR_FAIL_COND_MSG(reg_script_lang != OK,
            "Failed to register Luau language.");
        WARN_PRINT("[LuauGDExtension] Luau Script Language registered successfully");
    } 
    else if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {

        GDREGISTER_CLASS(ResourceFormatLoaderLuau);
        resource_loader_luau.instantiate();
        ERR_FAIL_COND_MSG(!resource_loader_luau.is_valid(), 
            "Failed to instantiate Luau resource loader.");
        nobind::ResourceLoader::get_singleton()->add_resource_format_loader(resource_loader_luau);
        WARN_PRINT("[LuauGDExtension] Resource Loader registered successfully");

        GDREGISTER_CLASS(ResourceFormatSaverLuau);
        resource_saver_luau.instantiate();
        ERR_FAIL_COND_MSG(!resource_saver_luau.is_valid(), 
            "Failed to instantiate Luau resource saver.");
        nobind::ResourceSaver::get_singleton()->add_resource_format_saver(resource_saver_luau);
        WARN_PRINT("[LuauGDExtension] Resource Saver registered successfully");
    }

#ifdef TOOLS_ENABLED
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        GDREGISTER_CLASS(LuauPlugin);
        GDREGISTER_CLASS(LuauSyntaxHighlighter);

        EditorPlugins::add_by_type<LuauPlugin>();
        WARN_PRINT("[LuauGDExtension] Editor Plugin registered successfully");
    }
#endif
}

void godot::uninitialize_luau_module(ModuleInitializationLevel p_level) {
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
