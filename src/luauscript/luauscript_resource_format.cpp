#include "luauscript_resource_format.h"

#include <godot_cpp/classes/file_access.hpp>

#include "luau_script.h"
#include "luau_cache.h"
#include "luau_constants.h"

using namespace godot;

//MARK: Loader
String LuauResourceFormatLoader::get_resource_type(const String &p_path) {
	return p_path.get_extension().to_lower() == luau::LUAUSCRIPT_EXTENSION ? luau::LUAUSCRIPT_TYPE : "";
}

bool LuauResourceFormatLoader::_recognize_path(const String &p_path, const StringName &p_type) const {
    return p_path.get_extension().to_lower() == luau::LUAUSCRIPT_EXTENSION;
}

PackedStringArray LuauResourceFormatLoader::_get_recognized_extensions() const
{
    PackedStringArray extensions;
    extensions.push_back(luau::LUAUSCRIPT_EXTENSION);

    return extensions;
}

bool LuauResourceFormatLoader::_handles_type(const StringName &p_type) const {
    return p_type == StringName(luau::LUAUSCRIPT_TYPE);
}

String LuauResourceFormatLoader::_get_resource_type(const String &p_path) const {
    return get_resource_type(p_path);
}

Variant LuauResourceFormatLoader::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const {
	Error err = OK;
	return LuauCache::get_singleton()->get_script(p_path, err, p_cache_mode == CACHE_MODE_IGNORE);
}


//MARK: Saver
PackedStringArray LuauResourceFormatSaver::_get_recognized_extensions(const Ref<Resource> &p_resource) const {
	PackedStringArray extensions;
	Ref<LuauScript> ref = p_resource;
	if (ref.is_valid()) {
		extensions.push_back(luau::LUAUSCRIPT_EXTENSION);
	}
	return extensions;
}

bool LuauResourceFormatSaver::_recognize(const Ref<Resource> &p_resource) const {
	Ref<LuauScript> ref = p_resource;
	return ref.is_valid();
}

Error LuauResourceFormatSaver::_save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<LuauScript> script = p_resource;
	ERR_FAIL_COND_V(script.is_null(), ERR_INVALID_PARAMETER);

	String source = script->get_source_code();

	{
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::ModeFlags::WRITE);
		ERR_FAIL_COND_V_MSG(file.is_null(), FileAccess::get_open_error(), "Failed to save file at " + p_path);

		file->store_string(source);

		if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF)
			return ERR_CANT_CREATE;
	}

	// TODO: Godot's default language implementations have a check here. It isn't possible in extensions (yet).
	// if (ScriptServer::is_reload_scripts_on_save_enabled())
	LuauLanguage::get_singleton()->_reload_tool_script(p_resource, false);

	return OK;
}

bool LuauResourceFormatSaver::_recognize_path(const Ref<Resource> &p_resource, const String &p_path) const {
    return p_path.get_extension().to_lower() == luau::LUAUSCRIPT_EXTENSION;
}

// Error LuauResourceFormatSaver::_set_uid(const String &p_path, int64_t p_uid) {
//     return Error();
// }
