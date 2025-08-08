#ifndef LUAU_SCRIPT_H
#define LUAU_SCRIPT_H

#include <godot_cpp/classes/script_extension.hpp>
#include <godot_cpp/classes/script_language_extension.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/multiplayer_api.hpp>
#include <godot_cpp/classes/multiplayer_peer.hpp>
#include <godot_cpp/templates/self_list.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "nobind.h"
#include "luau_constants.h"
#include "luauscript/luau_engine.h"

namespace godot {
    class LuauScript;
    class LuauCache;


    //MARK: ThreadPermissions
    enum ThreadPermissions {
        PERMISSION_INHERIT = -1,
        PERMISSION_BASE = 0,
        // Default permission level. Restricted to core.
        PERMISSION_INTERNAL = 1 << 0,
        PERMISSION_OS = 1 << 1,
        PERMISSION_FILE = 1 << 2,
        PERMISSION_HTTP = 1 << 3,
    
        PERMISSIONS_ALL = PERMISSION_BASE | PERMISSION_INTERNAL | PERMISSION_OS | PERMISSION_FILE | PERMISSION_HTTP
    };



    //MARK: GDProperty
    struct GDProperty {
        GDExtensionVariantType type = GDEXTENSION_VARIANT_TYPE_NIL;
        BitField<PropertyUsageFlags> usage = PROPERTY_USAGE_DEFAULT;
    
        String name;
        StringName class_name;
    
        PropertyHint hint = PROPERTY_HINT_NONE;
        String hint_string;
    
        operator Dictionary() const;
        operator Variant() const;
    
        void set_variant_type() {
            type = GDEXTENSION_VARIANT_TYPE_NIL;
            usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT;
        }
    
        void set_object_type(const String &p_type, const String &p_base_type = "") {
            type = GDEXTENSION_VARIANT_TYPE_OBJECT;
            class_name = p_type; // For non-property usage
    
            const String &godot_type = p_base_type.is_empty() ? p_type : p_base_type;
    
            if (nobind::ClassDB::get_singleton()->is_parent_class(godot_type, "Resource")) {
                hint = PROPERTY_HINT_RESOURCE_TYPE;
                hint_string = p_type;
            } else if (nobind::ClassDB::get_singleton()->is_parent_class(godot_type, "Node")) {
                hint = PROPERTY_HINT_NODE_TYPE;
                hint_string = p_type;
            }
        }
    
        void set_typed_array_type(const GDProperty &p_type) {
            type = GDEXTENSION_VARIANT_TYPE_ARRAY;
            hint = PROPERTY_HINT_ARRAY_TYPE;
    
            if (p_type.type == GDEXTENSION_VARIANT_TYPE_OBJECT) {
                if (p_type.hint == PROPERTY_HINT_RESOURCE_TYPE) {
                    // see core/object/object.h
                    Array hint_values;
                    hint_values.resize(3);
                    hint_values[0] = Variant::OBJECT;
                    hint_values[1] = PROPERTY_HINT_RESOURCE_TYPE;
                    hint_values[2] = p_type.hint_string;
                    hint_string = String("{0}/{1}:{2}").format(hint_values);
                } else {
                    hint_string = p_type.class_name;
                }
            } else {
                hint_string = Variant::get_type_name(Variant::Type(p_type.type));
            }
        }
    
        GDExtensionVariantType get_arg_type() const { return type; }
        const StringName &get_arg_type_name() const { return class_name; }
    };
    

    //MARK: GDClassProperty
    struct GDClassProperty {
        GDProperty property;
    
        StringName getter;
        StringName setter;
    
        Variant default_value;
    };
    

    //MARK: GDMethod
    struct GDMethod {
        String name;
        GDProperty return_val;
        BitField<MethodFlags> flags = METHOD_FLAGS_DEFAULT;
        Vector<GDProperty> arguments;
        Vector<Variant> default_arguments;
    
        operator Dictionary() const;
        operator Variant() const;
    
        bool is_method_static() const { return false; }
        bool is_method_vararg() const { return true; }
    };
    
    // ! Reference: modules/multiplayer/scene_rpc_interface.cpp _parse_rpc_config
    struct GDRpc {
        String name;
        MultiplayerAPI::RPCMode rpc_mode = MultiplayerAPI::RPC_MODE_AUTHORITY;
        MultiplayerPeer::TransferMode transfer_mode = MultiplayerPeer::TRANSFER_MODE_RELIABLE;
        bool call_local = false;
        int channel = 0;
    
        operator Dictionary() const;
        operator Variant() const;
    };



    //MARK: GDClassDefinition
    struct GDClassDefinition {
        String name;
        String extends = "RefCounted";
        LuauScript *base_script = nullptr;
    
        String icon_path;
    
        ThreadPermissions permissions = PERMISSION_BASE;
    
        bool is_tool = false;
    
        HashMap<StringName, GDMethod> methods;
        HashMap<StringName, uint64_t> property_indices;
        Vector<GDClassProperty> properties;
        HashMap<StringName, GDMethod> signals;
        HashMap<StringName, GDRpc> rpcs;
        HashMap<StringName, int> constants;
    
        int set_prop(const String &p_name, const GDClassProperty &p_prop);
    };


    //MARK: ScriptInstance
    class ScriptInstance {
    protected:
        static void copy_prop(const GDProperty &p_src, GDExtensionPropertyInfo &p_dst);
        static void free_prop(const GDExtensionPropertyInfo &p_prop);

    public:
        static void init_script_instance_info_common(GDExtensionScriptInstanceInfo3 &p_info);

        enum PropertySetGetError {
            PROP_OK,
            PROP_NOT_FOUND,
            PROP_WRONG_TYPE,
            PROP_READ_ONLY,
            PROP_WRITE_ONLY,
            PROP_GET_FAILED,
            PROP_SET_FAILED
        };

        void get_property_state(GDExtensionScriptInstancePropertyStateAdd p_add_func, void *p_userdata);
        void get_property_state(List<Pair<StringName, Variant>> &p_list);
        
        void free_property_list(const GDExtensionPropertyInfo *p_list, uint32_t p_count) const;
        void free_method_list(const GDExtensionMethodInfo *p_list, uint32_t p_count) const;

        ScriptLanguage *get_language() const;

    	virtual bool set(const StringName &p_name, const Variant &p_value, PropertySetGetError *r_err = nullptr) = 0;
	    virtual bool get(const StringName &p_name, Variant &r_ret, PropertySetGetError *r_err = nullptr) = 0;
        virtual GDExtensionPropertyInfo *get_property_list(uint32_t *r_count) = 0;
        virtual bool validate_property(GDExtensionPropertyInfo *p_property) const { return false; }
        virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid) const = 0;
        virtual GDExtensionMethodInfo *get_method_list(uint32_t *r_count) const;
        virtual bool has_method(const StringName &p_name) const = 0;
        virtual Object *get_owner() const = 0;
        virtual Ref<LuauScript> get_script() const = 0;
    };


    //MARK: LuauScriptInstance
    class LuauScriptInstance : public ScriptInstance {
        Object *owner = nullptr;
        Ref<LuauScript> script;
        LuauEngine::VMType vm_type;
        
        // Lua state management
        lua_State *L = nullptr; // Main VM state
        lua_State *T = nullptr; // Thread state for this instance
        int thread_ref = LUA_NOREF; // Reference to keep thread alive
        int self_ref = LUA_NOREF; // Reference to self table
        
        int call_internal(const StringName &p_method, lua_State *ET, int argc, int retc);
        
    public:
        static const GDExtensionScriptInstanceInfo3 INSTANCE_INFO;

        bool property_can_revert(const StringName &p_name);
        bool property_get_revert(const StringName &p_name, Variant *r_ret);
        void call(const StringName &p_method, const Variant *const *p_args, const GDExtensionInt p_argument_count, Variant *r_return, GDExtensionCallError *r_error);
        void notification(int32_t p_what);
        void to_string(GDExtensionBool *r_is_valid, String *r_out);    	
        
        bool set(const StringName &p_name, const Variant &p_value, PropertySetGetError *r_err = nullptr) override;
	    bool get(const StringName &p_name, Variant &r_ret, PropertySetGetError *r_err = nullptr) override;
        GDExtensionPropertyInfo *get_property_list(uint32_t *r_count) override;
        // virtual bool validate_property(GDExtensionPropertyInfo *p_property) const { return false; }
        Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid) const override;
        // virtual GDExtensionMethodInfo *get_method_list(uint32_t *r_count) const;
        bool has_method(const StringName &p_name) const override;
        virtual Object *get_owner() const override;
        Ref<LuauScript> get_script() const override;

    // Initialize the Lua state for this instance
    void initialize_lua_state(lua_State *p_L, lua_State *p_thread, int p_thread_ref, int p_self_ref) {
        L = p_L;
        T = p_thread;
        thread_ref = p_thread_ref;
        self_ref = p_self_ref;
    }
    
    int get_self_ref() const { return self_ref; }
    
    LuauScriptInstance(const Ref<LuauScript> &p_script, Object *p_owner, LuauEngine::VMType p_vmtype);
    ~LuauScriptInstance();
    };



    //MARK: PlaceHolderScriptInstance
#ifdef TOOLS_ENABLED

    class PlaceHolderScriptInstance final : public ScriptInstance {
        Object *owner = nullptr;
        Ref<LuauScript> script;

        List<GDProperty> properties;
        HashMap<StringName, Variant> values;
        HashMap<StringName, Variant> constants;

    public:
        // Accessors for script reload functionality
        void update_properties(const Vector<GDClassProperty> &p_properties) {
            properties.clear();
            for (const GDClassProperty &prop : p_properties) {
                properties.push_back(prop.property);
            }
        }
        
        void update_constants(const HashMap<StringName, Variant> &p_constants) {
            constants = p_constants;
        }
        static const GDExtensionScriptInstanceInfo3 INSTANCE_INFO;

        bool property_set_fallback(const StringName &p_name, const Variant &p_value);
        bool property_get_fallback(const StringName &p_name, Variant &r_ret);

    	bool set(const StringName &p_name, const Variant &p_value, PropertySetGetError *r_err = nullptr) override;
	    bool get(const StringName &p_name, Variant &r_ret, PropertySetGetError *r_err = nullptr) override;
        GDExtensionPropertyInfo *get_property_list(uint32_t *r_count) override;
        // virtual bool validate_property(GDExtensionPropertyInfo *p_property) const { return false; }
        Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid) const override;
        // virtual GDExtensionMethodInfo *get_method_list(uint32_t *r_count) const;
        bool has_method(const StringName &p_name) const override;
        Object *get_owner() const override { return owner; };
        Ref<LuauScript> get_script() const override { return script; };

        PlaceHolderScriptInstance(const Ref<LuauScript> &p_script, Object *p_owner);
        ~PlaceHolderScriptInstance();
    };

#endif



    //MARK: LuauScript
    class LuauScript : public ScriptExtension {
        GDCLASS(LuauScript, ScriptExtension);

        friend class LuauLanguage;
        friend class LuauCache;
        friend class LuauScriptInstance;
#ifdef TOOLS_ENABLED
        friend class PlaceHolderScriptInstance;
#endif // TOOLS_ENABLED

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
        PackedByteArray bytecode;

        GDClassDefinition definition;
        HashMap<StringName, Variant> constants;

        HashMap<uint64_t, LuauScriptInstance *> instances;
        
#ifdef TOOLS_ENABLED
        HashMap<uint64_t, PlaceHolderScriptInstance *> placeholders;
        bool placeholder_fallback_enabled = false;
#endif // TOOLS_ENABLED 
        
    public:
        Error load_source_code(const String &p_path);
        Error load(LoadStage p_load_stage, bool p_force = false);

        const GDClassDefinition &get_definition() const { return definition; }
        Ref<LuauScript> get_base() const { return base; }

        bool _editor_can_reload_from_file() override;
        void _placeholder_erased(void *p_placeholder) override;
        bool _can_instantiate() const override; 
        Ref<Script> _get_base_script() const override;
        StringName _get_global_name() const override;
        bool _inherits_script(const Ref<Script> &p_script) const override;
        StringName _get_instance_base_type() const override;
        void *_instance_create(Object *p_for_object) const override;
        void *_placeholder_instance_create(Object *p_for_object) const override;

        bool instance_has(uint64_t p_obj_id) const;
        bool _instance_has(Object *p_object) const override;

        bool _has_source_code() const override;
        String _get_source_code() const override;
        void _set_source_code(const String &p_code) override;
        Error _reload(bool p_keep_state) override;
        // virtual StringName _get_doc_class_name() const;
        TypedArray<Dictionary> _get_documentation() const override;
        // virtual String _get_class_icon_path() const;
        // virtual bool _has_method(const StringName &p_method) const;
        bool _has_static_method(const StringName &p_method) const override;
        // virtual Variant _get_script_method_argument_count(const StringName &p_method) const;
        // virtual Dictionary _get_method_info(const StringName &p_method) const;
        bool _is_tool() const override;
        bool _is_valid() const override;
        // virtual bool _is_abstract() const;
        ScriptLanguage *_get_language() const override;
        // virtual bool _has_script_signal(const StringName &p_signal) const;
        TypedArray<Dictionary> _get_script_signal_list() const override;
        bool _has_property_default_value(const StringName &p_property) const override;
        Variant _get_property_default_value(const StringName &p_property) const override;
        void _update_exports() override;
        // virtual TypedArray<Dictionary> _get_script_method_list() const;
        TypedArray<Dictionary> _get_script_property_list() const override;
        // virtual int32_t _get_member_line(const StringName &p_member) const;
        Dictionary _get_constants() const override;
        // virtual TypedArray<StringName> _get_members() const;
        bool _is_placeholder_fallback_enabled() const override;
        // virtual Variant _get_rpc_config() const;
    };


    //MARK: LuauLanguage 
    class LuauLanguage : public ScriptLanguageExtension {
        GDCLASS(LuauLanguage, ScriptLanguageExtension);
        
        friend class LuauScript;

        Ref<Mutex> mutex;

        static LuauLanguage *singleton;
        LuauEngine *luau = nullptr;
        LuauCache *cache = nullptr;

        uint64_t ticks_usec = 0;
        HashMap<StringName, Variant> global_constants;
        
        SelfList<LuauScript>::List script_list;

#ifdef TOOLS_ENABLED   
        HashMap<LuauScriptInstance *, void *> instance_to_godot;
    
        struct DebugInfo {
            struct StackInfo {
                // These are presumably unstable pointers, but they should be read
                // by Godot by the time they are collected/changed.
                const char *source;
                const char *name;
                int line = 0;
    
                operator Dictionary() const;
            };
    
            struct BreakStackInfo : public StackInfo {
                HashMap<String, Variant> members;
                HashMap<String, Variant> locals;
    
                void *instance = nullptr;
            };
    
            Ref<Mutex> call_lock;
            // STL is faster. The call_stack path especially is extremely hot (made
            // on every engine call in the editor).
            std::vector<StackInfo> call_stack;
            std::vector<BreakStackInfo> break_call_stack;
    
            int thread_ref = 0;
            lua_State *L = nullptr;
            String error;
            int break_depth = -1;
    
            String base_break_source;
            int base_break_line = -1;
    
            int breakhits[LuauEngine::VM_MAX] = { 0 };
    
            uint64_t interrupt_reset = 0;
        } debug;
    
        // void debug_init();
        // void debug_reset();
    
        // static void lua_interrupt(lua_State *L, int p_gc);
        // static void lua_debuginterrupt(lua_State *L, lua_Debug *ar);
        // static void lua_debugbreak(lua_State *L, lua_Debug *ar);
        // static void lua_debugstep(lua_State *L, lua_Debug *ar);
        // static void lua_debugprotectederror(lua_State *L);
    
        // static bool ar_to_si(lua_Debug &p_ar, DebugInfo::StackInfo &p_si);
#endif // TOOLS_ENABLED

    protected:
        static void _bind_methods() {};

    public:
        static LuauLanguage *get_singleton() { return singleton; };
        
#ifdef TOOLS_ENABLED
        List<Ref<LuauScript>> get_scripts() const;

#endif

	/* LANGUAGE FUNCTIONS */
        void _init() override;
        String _get_name() const override { return luau::LUAUSCRIPT_NAME; };
        String _get_type() const override { return luau::LUAUSCRIPT_TYPE; };
        String _get_extension() const override { return luau::LUAUSCRIPT_EXTENSION; };
        void _finish() override;
        
	/* EDITOR FUNCTIONS */
        PackedStringArray _get_reserved_words() const override;
        bool _is_control_flow_keyword(const String &p_keyword) const override;
        PackedStringArray _get_comment_delimiters() const override;
        // virtual PackedStringArray _get_doc_comment_delimiters() const;
        PackedStringArray _get_string_delimiters() const override;
        bool _is_using_templates() override;
        Ref<Script> _make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const override;
        TypedArray<Dictionary> _get_built_in_templates(const StringName &p_object) const override;
        Dictionary _validate(const String &p_script, const String &p_path, bool p_validate_functions, bool p_validate_errors, bool p_validate_warnings, bool p_validate_safe_lines) const override;
        String _validate_path(const String &p_path) const override;
        Object *_create_script() const override;
        // virtual bool _has_named_classes() const;
        bool _supports_builtin_mode() const override;
        bool _supports_documentation() const override;
        bool _can_inherit_from_file() const override;
        int32_t _find_function(const String &p_function, const String &p_code) const override;
        String _make_function(const String &p_class_name, const String &p_function_name, const PackedStringArray &p_function_args) const override;
        // virtual bool _can_make_function() const;
        // virtual Error _open_in_external_editor(const Ref<Script> &p_script, int32_t p_line, int32_t p_column);
        bool _overrides_external_editor() override;
        // virtual ScriptLanguage::ScriptNameCasing _preferred_file_name_casing() const;
        
        Dictionary _complete_code(const String &p_code, const String &p_path, Object *p_owner) const override;
        Dictionary _lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner) const override;
        String _auto_indent_code(const String &p_code, int32_t p_from_line, int32_t p_to_line) const override;

        void _add_global_constant(const StringName &p_name, const Variant &p_value);
        void _add_named_global_constant(const StringName &p_name, const Variant &p_value) override;
        void _remove_named_global_constant(const StringName &p_name) override;

        void _thread_enter() override {};
        void _thread_exit() override {};
        String _debug_get_error() const override;

        // virtual int32_t _debug_get_stack_level_count() const;
        // virtual int32_t _debug_get_stack_level_line(int32_t p_level) const;
        // virtual String _debug_get_stack_level_function(int32_t p_level) const;
        // virtual String _debug_get_stack_level_source(int32_t p_level) const;
        // virtual Dictionary _debug_get_stack_level_locals(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth);
        // virtual Dictionary _debug_get_stack_level_members(int32_t p_level, int32_t p_max_subitems, int32_t p_max_depth);
        // virtual void *_debug_get_stack_level_instance(int32_t p_level);
        // virtual Dictionary _debug_get_globals(int32_t p_max_subitems, int32_t p_max_depth);
        // virtual String _debug_parse_stack_level_expression(int32_t p_level, const String &p_expression, int32_t p_max_subitems, int32_t p_max_depth);
        TypedArray<Dictionary> _debug_get_current_stack_info() const;
        
        void _reload_all_scripts() override;

        // virtual void _reload_scripts(const Array &p_scripts, bool p_soft_reload);
        void _reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) override;
        
        PackedStringArray _get_recognized_extensions() const override;
        TypedArray<Dictionary> _get_public_functions() const override;
        Dictionary _get_public_constants() const override;
        TypedArray<Dictionary> _get_public_annotations() const override;

        // virtual void _profiling_start();
        // virtual void _profiling_stop();
        // virtual void _profiling_set_save_native_calls(bool p_enable);
        // virtual int32_t _profiling_get_accumulated_data(ScriptLanguageExtensionProfilingInfo *p_info_array, int32_t p_info_max);
        // virtual int32_t _profiling_get_frame_data(ScriptLanguageExtensionProfilingInfo *p_info_array, int32_t p_info_max);
        
        void _frame();
        bool _handles_global_class_type(const String &p_type) const;
        Dictionary _get_global_class_name(const String &p_path) const;

        LuauLanguage();
        ~LuauLanguage();
    };
    
}

#endif