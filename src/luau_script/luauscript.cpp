#include <luau_script/luauscript.h>
#include <luau_script/luau_language.h>

#include <luau_script/luau_common.h>

GDExtensionScriptInstanceInfo3 instance_info;

class LuauScriptInstance {
};

Ref<Script> LuauScript::_get_base_script() const {
	return nullptr;
}

StringName LuauScript::_get_instance_base_type() const {
	return LuauCommon::get_string_name();
}

void *LuauScript::_instance_create(Object *p_for_object) const {

	// LuauScriptInstance *internal = memnew(LuauScriptInstance(Ref<Script>(this), p_for_object, type));
	// void *godot_instance = internal::gdextension_interface_script_instance_create3(
	// 	&LuauScriptInstance::INSTANCE_INFO, 
	// 	internal);
	// return godot_instance;

	return internal::gdextension_interface_script_instance_create3(&instance_info, new LuauScriptInstance());
}

void *LuauScript::_placeholder_instance_create(Object *p_for_object) const {
	return internal::gdextension_interface_script_instance_create3(&instance_info, new LuauScriptInstance());
}

String LuauScript::_get_source_code() const {
	return source;
}

bool LuauScript::_is_tool() const {
	return false;
}

bool LuauScript::_is_valid() const {
	return true;
}

ScriptLanguage *LuauScript::_get_language() const {
	return LuauLanguage::get_singleton();
}

void LuauScript::_update_exports() {
}

LuauScript::LuauScript() {}