#include <luau_script/luau_engine.h>

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

#include <luau_script/luauscript.h>
#include <luau_script/luau_language.h>
#include <luau_script/luauscript_resource_format.h>

void LuauEngine::init() {
    GDREGISTER_CLASS(LuauScript);
    GDREGISTER_CLASS(LuauLanguage);

    GDREGISTER_CLASS(LuauScriptLoader);
    GDREGISTER_CLASS(LuauScriptSaver);

    Engine::get_singleton()->register_script_language(LuauLanguage::get_singleton());
    ResourceLoader::get_singleton()->add_resource_format_loader(LuauScriptLoader::get_singleton());
    ResourceSaver::get_singleton()->add_resource_format_saver(LuauScriptSaver::get_singleton());
}

void LuauEngine::deinit() {}

// bool LuauEngine::is_valid_extension(const String &path) {
//     return path.get_extension().to_lower() == file_extensions.get(0)
//         || path.get_extension().to_lower() == file_extensions.get(1);
// }