#include <luau_script/luau_engine.h>

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

#include <luau_script/luauscript.h>
#include <luau_script/luau_language.h>
#include <luau_script/luauscript_resource_format.h>

void LuauEngine::init() {
    #ifdef TOOLS_ENABLED
        print_line("Luau dev initialized");
    #else
        print_line("Luau initialized");
    #endif

    GDREGISTER_CLASS(LuauScript);
    GDREGISTER_CLASS(LuauLanguage);

    GDREGISTER_CLASS(LuauScriptLoader);
    GDREGISTER_CLASS(LuauScriptSaver);

    Engine::get_singleton()->register_script_language(memnew(LuauLanguage));
    ResourceLoader::get_singleton()->add_resource_format_loader(memnew(LuauScriptLoader));
    ResourceSaver::get_singleton()->add_resource_format_saver(memnew(LuauScriptSaver));
}

void LuauEngine::deinit() {}