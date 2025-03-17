
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/file_access.hpp>

#include <luau_script/luau_common.h>
#include <luau_script/luau_engine.h>
#include <luau_script/luau_language.h>
#include <luau_script/luauscript_resource_format.h>

using namespace godot;

//MARK: LuauScriptLoader
LuauScriptLoader *LuauScriptLoader::singleton;

LuauScriptLoader *LuauScriptLoader::get_singleton() {
	if (singleton) return singleton;
	
	singleton = memnew(LuauScriptLoader);
	return singleton;
}

PackedStringArray LuauScriptLoader::_get_recognized_extensions() const {
    return LuauCommon::get_extensions();
}

bool LuauScriptLoader::_recognize_path(const String &p_path, const StringName &p_type) const {
	return LuauCommon::is_luau_file(p_path);
}

bool LuauScriptLoader::_handles_type(const StringName &p_type) const {
	return p_type == StringName("Luau");
}

String LuauScriptLoader::_get_resource_type(const String &p_path) const {
	if (LuauCommon::is_luau_file(p_path)) return LuauLanguage::get_singleton()->_get_type();
	return "";
}

String LuauScriptLoader::_get_resource_script_class(const String &p_path) const {
	return p_path.get_file().split(".")[1];
}

int64_t LuauScriptLoader::_get_resource_uid(const String &p_path) const {
	return ResourceUID::get_singleton()->text_to_id(p_path);
}

Variant LuauScriptLoader::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const {
	Ref<LuauScript> script;

	if (script.is_null()) {
		script.instantiate();
	}

	String source_code = FileAccess::get_file_as_string(p_original_path);
	script->_set_source_code(source_code);

	return script;
}

//MARK: LuauScriptSavor
LuauScriptSaver *LuauScriptSaver::singleton;

LuauScriptSaver *LuauScriptSaver::get_singleton() {
	if (singleton) return singleton;
	
	singleton = memnew(LuauScriptSaver);
	return singleton;
}

Error LuauScriptSaver::_save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<LuauScript> script = p_resource;

	String source = script->get_source_code();

	{
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::ModeFlags::WRITE);

		file->store_string(source);
	}

	return OK;
}

Error LuauScriptSaver::_set_uid(const String &p_path, int64_t p_uid) {
	return OK;
}

bool LuauScriptSaver::_recognize(const Ref<Resource> &p_resource) const {
	Ref<LuauScript> ref = p_resource;
	return ref.is_valid();
}

PackedStringArray LuauScriptSaver::_get_recognized_extensions(const Ref<Resource> &p_resource) const {
    return LuauCommon::get_extensions();
}

bool LuauScriptSaver::_recognize_path(const Ref<Resource> &p_resource, const String &p_path) const {
	return LuauCommon::is_luau_file(p_path);
}
