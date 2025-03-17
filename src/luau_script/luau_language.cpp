#include <luau_script/luauscript.h>
#include <luau_script/luau_common.h>
#include <luau_script/luau_language.h>

using namespace godot;

LuauLanguage *LuauLanguage::singleton = nullptr;

LuauLanguage *LuauLanguage::get_singleton() {
	if (singleton) return singleton;
	
	singleton = memnew(LuauLanguage);
	return singleton;
}

String LuauLanguage::_get_name() const {
	return LuauCommon::get_name();
}

String LuauLanguage::_get_type() const {
	return LuauCommon::get_name();
}

String LuauLanguage::_get_extension() const {
	return LuauCommon::get_primary_extension();
}

PackedStringArray LuauLanguage::_get_doc_comment_delimiters() const {
	PackedStringArray delimiters;
	delimiters.push_back("--");
	delimiters.push_back("--[[ ]]");

	return delimiters;
}

PackedStringArray LuauLanguage::_get_string_delimiters() const {
	PackedStringArray delimiters;
	delimiters.push_back("\" \"");
	delimiters.push_back("' '");
	delimiters.push_back("[[ ]]");

	return delimiters;
}

Ref<Script> LuauLanguage::_make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const {
	return Ref<Script>();
}

TypedArray<Dictionary> LuauLanguage::_get_built_in_templates(const StringName &p_object) const {
	#ifdef TOOLS_ENABLED
	UtilityFunctions::print("tools enabled");
	#endif // TOOLS_ENABLED
	return TypedArray<Dictionary>();
}

bool LuauLanguage::_is_using_templates() {
	return true;
}

Dictionary LuauLanguage::_validate(const String &p_script, const String &p_path, bool p_validate_functions, bool p_validate_errors, bool p_validate_warnings, bool p_validate_safe_lines) const {
	Dictionary output;

	output["valid"] = true;

	return output;
}

String LuauLanguage::_validate_path(const String &p_path) const {
	return "";
}

bool LuauLanguage::_supports_builtin_mode() const {
	return true;
}

bool LuauLanguage::_can_inherit_from_file() const {
	return true;
}

Error LuauLanguage::_open_in_external_editor(const Ref<Script> &p_script, int32_t p_line, int32_t p_column) {
	return Error();
}

PackedStringArray LuauLanguage::_get_recognized_extensions() const {
	return LuauCommon::get_extensions();
}

bool LuauLanguage::_handles_global_class_type(const String &p_type) const {
	return p_type == _get_type();
} 

LuauLanguage::LuauLanguage() {
	singleton = this;
	lock.instantiate();
}

LuauLanguage::~LuauLanguage() {
	singleton = nullptr;
}