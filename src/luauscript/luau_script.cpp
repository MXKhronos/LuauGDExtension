#include "luau_script.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/string.hpp>

#include "nobind.h"
#include "luau_engine.h"
#include "luau_constants.h"

using namespace godot;

void LuauScript::_set_source_code(const String &p_code) {
    source = p_code;
    source_changed_cache = true;
}

ScriptLanguage *LuauScript::_get_language() const {
    return LuauLanguage::get_singleton();
}

Error LuauScript::load_source_code(const String &p_path) {
    Error err = OK;

    Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::ModeFlags::READ);
	ERR_FAIL_COND_V_MSG(file.is_null(), FileAccess::get_open_error(), "Failed to read file at " + p_path);

	uint64_t len = file->get_length();
	PackedByteArray bytes = file->get_buffer(len);
	bytes.resize(len + 1);
	bytes[len] = 0; // EOF

    String src;
	src.parse_utf8(reinterpret_cast<const char *>(bytes.ptr()));

    _set_source_code(src);
    
    return err;
}

Error LuauScript::load(LoadStage p_load_stage, bool p_force) {
    Error err = OK;
    return err;
}

StringName LuauScript::_get_instance_base_type() const {
    return StringName();
}

bool godot::LuauScript::_can_instantiate() const {
    return false;
}

Ref<Script> LuauScript::_get_base_script() const
{
    return Ref<Script>();
}

LuauLanguage *LuauLanguage::singleton = nullptr;

void LuauLanguage::_init() {
   luau = memnew(LuauEngine);
}

PackedStringArray LuauLanguage::_get_recognized_extensions() const {
    PackedStringArray extension;
    extension.push_back(LUAUSCRIPT_EXTENSION);
    return extension;
}

Object *LuauLanguage::_create_script() const {
    return memnew(LuauScript);
}

void LuauLanguage::_add_named_global_constant(const StringName &p_name, const Variant &p_value)
{
    global_constants[p_name] = p_value;
}

void LuauLanguage::_frame() {
    uint64_t new_ticks = nobind::Time::get_singleton()->get_ticks_usec();
	double time_scale = nobind::Engine::get_singleton()->get_time_scale();

	double delta = 0;
	if (ticks_usec != 0)
		delta = (new_ticks - ticks_usec) / 1e6f;

	ticks_usec = new_ticks; 
}

bool LuauLanguage::_handles_global_class_type(const String &p_type) const {
    #ifdef TOOLS_ENABLED
        return p_type == _get_type();
    #else
        return false;
    #endif // TOOLS_ENABLED
}

Dictionary LuauLanguage::_get_global_class_name(const String &p_path) const {
    #ifdef TOOLS_ENABLED
    #endif // TOOLS_ENABLED

    return Dictionary();
}
