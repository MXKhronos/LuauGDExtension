#ifndef LUAU_SCRIPT_H
#define LUAU_SCRIPT_H

#include <godot_cpp/classes/script_extension.hpp>
#include <godot_cpp/classes/script_language_extension.hpp>
#include <godot_cpp/templates/hash_map.hpp>

namespace {
    constexpr char const* LUAUSCRIPT_NAME = "Luau";
    constexpr char const* LUAUSCRIPT_EXTENSION = "luau";
    constexpr char const* LUAUSCRIPT_TYPE = "LuauScript";
}

namespace godot {
    class LuauEngine;

    class ScriptInstance {

    };

    class LuauScriptInstance : public ScriptInstance {

    };


    //MARK: LuauScript
    class LuauScript : public ScriptExtension {
        GDCLASS(LuauScript, ScriptExtension);

        friend class LuauLanguage;
        friend class LuauCache;

    public:
        enum LoadStage {
            LOAD_NONE,
            LOAD_COMPILE,
            LOAD_ANALYSIS,
            LOAD_FULL,
        };

    protected:
        LoadStage load_stage = LOAD_NONE;

        static void _bind_methods() {};
        
    private:
        Ref<LuauScript> base;

        String source;
        bool source_changed_cache;
        
    public:
        Error load_source_code(const String &p_path);
        Error load(LoadStage p_load_stage, bool p_force = false);

        // virtual bool _editor_can_reload_from_file();
        // virtual void _placeholder_erased(void *p_placeholder);
        // virtual bool _can_instantiate() const override;
        Ref<Script> _get_base_script() const override;
        // virtual StringName _get_global_name() const;
        // virtual bool _inherits_script(const Ref<Script> &p_script) const;
        StringName _get_instance_base_type() const override;
        // virtual void *_instance_create(Object *p_for_object) const;
        // virtual void *_placeholder_instance_create(Object *p_for_object) const;
        // virtual bool _instance_has(Object *p_object) const;
        // virtual bool _has_source_code() const;
        // virtual String _get_source_code() const;
        void _set_source_code(const String &p_code) override;
        // virtual Error _reload(bool p_keep_state);
        // virtual StringName _get_doc_class_name() const;
        // virtual TypedArray<Dictionary> _get_documentation() const;
        // virtual String _get_class_icon_path() const;
        // virtual bool _has_method(const StringName &p_method) const;
        // virtual bool _has_static_method(const StringName &p_method) const;
        // virtual Variant _get_script_method_argument_count(const StringName &p_method) const;
        // virtual Dictionary _get_method_info(const StringName &p_method) const;
        // virtual bool _is_tool() const;
        // virtual bool _is_valid() const;
        // virtual bool _is_abstract() const;
        ScriptLanguage *_get_language() const override;
        // virtual bool _has_script_signal(const StringName &p_signal) const;
        // virtual TypedArray<Dictionary> _get_script_signal_list() const;
        // virtual bool _has_property_default_value(const StringName &p_property) const;
        // virtual Variant _get_property_default_value(const StringName &p_property) const;
        // virtual void _update_exports();
        // virtual TypedArray<Dictionary> _get_script_method_list() const;
        // virtual TypedArray<Dictionary> _get_script_property_list() const;
        // virtual int32_t _get_member_line(const StringName &p_member) const;
        // virtual Dictionary _get_constants() const;
        // virtual TypedArray<StringName> _get_members() const;
        // virtual bool _is_placeholder_fallback_enabled() const;
        // virtual Variant _get_rpc_config() const;
    };


    //MARK: LuauLanguage 
    class LuauLanguage : public ScriptLanguageExtension {
        GDCLASS(LuauLanguage, ScriptLanguageExtension);
        
        friend class LuauScript;

        static LuauLanguage *singleton;
        LuauEngine *luau = nullptr;
        LuauCache *cache = nullptr;

        uint64_t ticks_usec = 0;
        HashMap<StringName, Variant> global_constants;

    protected:
        static void _bind_methods() {};

    public:
        static LuauLanguage *get_singleton() { return singleton; };
        
        String _get_name() const override { return LUAUSCRIPT_NAME; };
        String _get_type() const override { return LUAUSCRIPT_TYPE; };
        String _get_extension() const override { return LUAUSCRIPT_EXTENSION; };
        void _init() override;
        void _finish() override;
        // virtual PackedStringArray _get_reserved_words() const;
        // virtual bool _is_control_flow_keyword(const String &p_keyword) const;
        // virtual PackedStringArray _get_comment_delimiters() const;
        // virtual PackedStringArray _get_doc_comment_delimiters() const;
        // virtual PackedStringArray _get_string_delimiters() const;
        // virtual Ref<Script> _make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const;
        // virtual TypedArray<Dictionary> _get_built_in_templates(const StringName &p_object) const;
        // virtual bool _is_using_templates();
        // virtual Dictionary _validate(const String &p_script, const String &p_path, bool p_validate_functions, bool p_validate_errors, bool p_validate_warnings, bool p_validate_safe_lines) const;
        // virtual String _validate_path(const String &p_path) const;
        Object *_create_script() const override;
        // virtual bool _has_named_classes() const;
        // virtual bool _supports_builtin_mode() const;
        // virtual bool _supports_documentation() const;
        // virtual bool _can_inherit_from_file() const;
        // virtual int32_t _find_function(const String &p_function, const String &p_code) const;
        // virtual String _make_function(const String &p_class_name, const String &p_function_name, const PackedStringArray &p_function_args) const;
        // virtual bool _can_make_function() const;
        // virtual Error _open_in_external_editor(const Ref<Script> &p_script, int32_t p_line, int32_t p_column);
        // virtual bool _overrides_external_editor();
        // virtual ScriptLanguage::ScriptNameCasing _preferred_file_name_casing() const;
        // virtual Dictionary _complete_code(const String &p_code, const String &p_path, Object *p_owner) const;
        // virtual Dictionary _lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner) const;
        // virtual String _auto_indent_code(const String &p_code, int32_t p_from_line, int32_t p_to_line) const;
        // virtual void _add_global_constant(const StringName &p_name, const Variant &p_value);
        void _add_named_global_constant(const StringName &p_name, const Variant &p_value) override;
        // virtual void _remove_named_global_constant(const StringName &p_name);
        void _thread_enter() override {};
        void _thread_exit() override {};
        // virtual String _debug_get_error() const;
        // virtual int32_t _debug_get_stack_level_count() const;
        // virtual int32_t _debug_get_stack_level_line(int32_t p_level) const;
        // virtual String _debug_get_stack_level_function(int32_t p_level) const;
        // virtual String _debug_get_stack_level_source(int32_t p_level) const;
        // virtual Dictionary _debug_get_stack_level_locals(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth);
        // virtual Dictionary _debug_get_stack_level_members(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth);
        // virtual void *_debug_get_stack_level_instance(int32_t p_level);
        // virtual Dictionary _debug_get_globals(int32_t p_max_subitems, int32_t p_max_depth);
        // virtual String _debug_parse_stack_level_expression(int32_t p_level, const String &p_expression, int32_t p_max_subitems, int32_t p_max_depth);
        // virtual TypedArray<Dictionary> _debug_get_current_stack_info();
        // virtual void _reload_all_scripts();
        // virtual void _reload_scripts(const Array &p_scripts, bool p_soft_reload);
        // virtual void _reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload);
        PackedStringArray _get_recognized_extensions() const override;
        // virtual TypedArray<Dictionary> _get_public_functions() const;
        // virtual Dictionary _get_public_constants() const;
        // virtual TypedArray<Dictionary> _get_public_annotations() const;
        // virtual void _profiling_start();
        // virtual void _profiling_stop();
        // virtual void _profiling_set_save_native_calls(bool p_enable);
        // virtual int32_t _profiling_get_accumulated_data(ScriptLanguageExtensionProfilingInfo *p_info_array, int32_t p_info_max);
        // virtual int32_t _profiling_get_frame_data(ScriptLanguageExtensionProfilingInfo *p_info_array, int32_t p_info_max);
        void _frame();
        bool _handles_global_class_type(const String &p_type) const;
        Dictionary _get_global_class_name(const String &p_path) const;
    };
    
}

#endif