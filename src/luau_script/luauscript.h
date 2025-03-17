#pragma once

#include <godot_cpp/classes/script_language.hpp>
#include <godot_cpp/classes/script_extension.hpp>

using namespace godot;

class LuauScript : public ScriptExtension {
	GDCLASS(LuauScript, ScriptExtension);

	friend class LuauLanguage;
	String origin_path;
	String source;

public:

    // bool _editor_can_reload_from_file();
    // void _placeholder_erased(void *p_placeholder);
    // bool _can_instantiate() const;
    Ref<Script> _get_base_script() const override;
    // StringName _get_global_name() const;
    // bool _inherits_script(const Ref<Script> &p_script) const;
    // StringName _get_instance_base_type() const;
    void *_instance_create(Object *p_for_object) const override;
    void *_placeholder_instance_create(Object *p_for_object) const override;
    // bool _instance_has(Object *p_object) const;
    // bool _has_source_code() const;
    String _get_source_code() const override;
    // void _set_source_code(const String &p_code);
    // Error _reload(bool p_keep_state);
    // StringName _get_doc_class_name() const;
    // TypedArray<Dictionary> _get_documentation() const;
    // String _get_class_icon_path() const;
    // bool _has_method(const StringName &p_method) const;
    bool _has_static_method(const StringName &p_method) const override { return false; };
    // Variant _get_script_method_argument_count(const StringName &p_method) const;
    // Dictionary _get_method_info(const StringName &p_method) const;
    bool _is_tool() const override;
    // bool _is_valid() const;
    // bool _is_abstract() const;
    ScriptLanguage *_get_language() const override;
    // bool _has_script_signal(const StringName &p_signal) const;
    // TypedArray<Dictionary> _get_script_signal_list() const;
    // bool _has_property_default_value(const StringName &p_property) const;
    // Variant _get_property_default_value(const StringName &p_property) const;
    void _update_exports() override;
    // TypedArray<Dictionary> _get_script_method_list() const;
    // TypedArray<Dictionary> _get_script_property_list() const;
    // int32_t _get_member_line(const StringName &p_member) const;
    // Dictionary _get_constants() const;
    // TypedArray<StringName> _get_members() const;
    // bool _is_placeholder_fallback_enabled() const;
    // Variant _get_rpc_config() const;

    LuauScript();

protected:
    static void _bind_methods() {}
};