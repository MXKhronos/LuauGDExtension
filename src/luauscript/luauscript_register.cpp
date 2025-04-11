#include "luauscript_register.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/memory.hpp>

#include "nobind.h"

#include "luau_constants.h"
#include "luauscript_resource_format.h"
#include "luau_engine.h"
#include "luau_script.h"

using namespace godot;

LuauLanguage *script_language_luau = nullptr;
Ref<LuauResourceFormatLoader> resource_loader_luau;
Ref<LuauResourceFormatSaver> resource_saver_luau;

void godot::luauGDExt_initialize() {
    // Register classes first
    GDREGISTER_CLASS(LuauScript);
    GDREGISTER_CLASS(LuauLanguage);
    GDREGISTER_CLASS(LuauResourceFormatLoader);
    GDREGISTER_CLASS(LuauResourceFormatSaver);

    // Create and register the script language
    script_language_luau = memnew(LuauLanguage);
    ERR_FAIL_COND_MSG(nobind::Engine::get_singleton()->register_script_language(script_language_luau) != OK, 
        "Failed to register Luau language.");

    // Create and register the resource loader
    resource_loader_luau.instantiate();
    ERR_FAIL_COND_MSG(!resource_loader_luau.is_valid(), 
        "Failed to instantiate Luau resource loader.");
    nobind::ResourceLoader::get_singleton()->add_resource_format_loader(resource_loader_luau);

    // Create and register the resource saver
    resource_saver_luau.instantiate();
    ERR_FAIL_COND_MSG(!resource_saver_luau.is_valid(), 
        "Failed to instantiate Luau resource saver.");
    nobind::ResourceSaver::get_singleton()->add_resource_format_saver(resource_saver_luau);
}

void godot::luauGDExt_uninitialize() {
    nobind::Engine::get_singleton()->unregister_script_language(script_language_luau);
    
    if (script_language_luau) {
        memdelete(script_language_luau);
    }

    nobind::ResourceLoader::get_singleton()->remove_resource_format_loader(resource_loader_luau);
    resource_loader_luau.unref();

    nobind::ResourceSaver::get_singleton()->remove_resource_format_saver(resource_saver_luau);
    resource_saver_luau.unref();
}
