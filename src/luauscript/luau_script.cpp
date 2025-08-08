#include "luau_script.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/self_list.hpp>
#include <godot_cpp/templates/pair.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "nobind.h"
#include "luau_engine.h"
#include "luau_cache.h"
#include "luau_constants.h"
#include "luau_marshal.h"
#include "luauscript_resource_format.h"

#include <Luau/Compiler.h>
#include <Luau/Parser.h>
#include <Luau/ParseResult.h>
#include <Luau/Ast.h>

using namespace godot;

//MARK: GDProperty
GDProperty::operator Dictionary() const {
	Dictionary dict;

	dict["type"] = type;
	dict["usage"] = usage;

	dict["name"] = name;
	dict["class_name"] = class_name;

	dict["hint"] = hint;
	dict["hint_string"] = hint_string;

	return dict;
}

GDProperty::operator Variant() const {
	return operator Dictionary();
}

//MARK: GDMethod
GDMethod::operator Dictionary() const {
	Dictionary dict;

	dict["name"] = name;
	dict["return"] = return_val;
	dict["flags"] = flags;

	Array args;
	for (const GDProperty &arg : arguments)
		args.push_back(arg);

	dict["args"] = args;

	Array default_args;
	for (const Variant &default_arg : default_arguments)
		default_args.push_back(default_arg);

	dict["default_args"] = default_args;

	return dict;
}

GDMethod::operator Variant() const {
	return operator Dictionary();
}


//MARK: ScriptInstance
#define COMMON_SELF ((ScriptInstance *)p_self)

void ScriptInstance::init_script_instance_info_common(GDExtensionScriptInstanceInfo3 &p_info) {
	// Must initialize potentially unused struct fields to nullptr
	// (if not, causes segfault on MSVC).
	p_info.property_can_revert_func = nullptr;
	p_info.property_get_revert_func = nullptr;

	p_info.call_func = nullptr;
	p_info.notification_func = nullptr;

	p_info.to_string_func = nullptr;

	p_info.refcount_incremented_func = nullptr;
	p_info.refcount_decremented_func = nullptr;

	p_info.is_placeholder_func = nullptr;

	p_info.set_fallback_func = nullptr;
	p_info.get_fallback_func = nullptr;

	p_info.set_func = [](void *p_self, GDExtensionConstStringNamePtr p_name, GDExtensionConstVariantPtr p_value) -> GDExtensionBool {
		return COMMON_SELF->set(*(const StringName *)p_name, *(const Variant *)p_value);
	};

	p_info.get_func = [](void *p_self, GDExtensionConstStringNamePtr p_name, GDExtensionVariantPtr r_ret) -> GDExtensionBool {
		return COMMON_SELF->get(*(const StringName *)p_name, *(Variant *)r_ret);
	};

	p_info.get_property_list_func = [](void *p_self, uint32_t *r_count) -> const GDExtensionPropertyInfo * {
		return COMMON_SELF->get_property_list(r_count);
	};

	p_info.free_property_list_func = [](void *p_self, const GDExtensionPropertyInfo *p_list, uint32_t p_count) {
		COMMON_SELF->free_property_list(p_list, p_count);
	};

	p_info.validate_property_func = [](void *p_self, GDExtensionPropertyInfo *p_property) -> GDExtensionBool {
		return COMMON_SELF->validate_property(p_property);
	};

	p_info.get_owner_func = [](void *p_self) {
		return COMMON_SELF->get_owner()->_owner;
	};

	p_info.get_property_state_func = [](void *p_self, GDExtensionScriptInstancePropertyStateAdd p_add_func, void *p_userdata) {
		COMMON_SELF->get_property_state(p_add_func, p_userdata);
	};

	p_info.get_method_list_func = [](void *p_self, uint32_t *r_count) -> const GDExtensionMethodInfo * {
		return COMMON_SELF->get_method_list(r_count);
	};

	p_info.free_method_list_func = [](void *p_self, const GDExtensionMethodInfo *p_list, uint32_t p_count) {
		COMMON_SELF->free_method_list(p_list, p_count);
	};

	p_info.get_property_type_func = [](void *p_self, GDExtensionConstStringNamePtr p_name, GDExtensionBool *r_is_valid) -> GDExtensionVariantType {
		return (GDExtensionVariantType)COMMON_SELF->get_property_type(*(const StringName *)p_name, (bool *)r_is_valid);
	};

	p_info.has_method_func = [](void *p_self, GDExtensionConstStringNamePtr p_name) -> GDExtensionBool {
		return COMMON_SELF->has_method(*(const StringName *)p_name);
	};

	p_info.get_script_func = [](void *p_self) {
		return COMMON_SELF->get_script().ptr()->_owner;
	};

	p_info.get_language_func = [](void *p_self) {
		return COMMON_SELF->get_language()->_owner;
	};
}

static String *string_alloc(const String &p_str) {
	String *ptr = memnew(String);
	*ptr = p_str;

	return ptr;
}

static StringName *stringname_alloc(const String &p_str) {
	StringName *ptr = memnew(StringName);
	*ptr = p_str;

	return ptr;
}

void ScriptInstance::copy_prop(const GDProperty &p_src, GDExtensionPropertyInfo &p_dst) {
	p_dst.type = p_src.type;
	p_dst.name = stringname_alloc(p_src.name);
	p_dst.class_name = stringname_alloc(p_src.class_name);
	p_dst.hint = p_src.hint;
	p_dst.hint_string = string_alloc(p_src.hint_string);
	p_dst.usage = p_src.usage;
}

void ScriptInstance::free_prop(const GDExtensionPropertyInfo &p_prop) {
	// smelly
	memdelete((StringName *)p_prop.name);
	memdelete((StringName *)p_prop.class_name);
	memdelete((String *)p_prop.hint_string);
}

void ScriptInstance::get_property_state(GDExtensionScriptInstancePropertyStateAdd p_add_func, void *p_userdata) {
	// ! refer to script_language.cpp get_property_state
	// the default implementation is not carried over to GDExtension

	uint32_t count = 0;
	GDExtensionPropertyInfo *props = get_property_list(&count);

	for (int i = 0; i < count; i++) {
		StringName *name = (StringName *)props[i].name;

		if (props[i].usage & PROPERTY_USAGE_STORAGE) {
			Variant value;
			bool is_valid = get(*name, value);

			if (is_valid)
				p_add_func(name, &value, p_userdata);
		}
	}

	free_property_list(props, count);
}

static void add_to_state(GDExtensionConstStringNamePtr p_name, GDExtensionConstVariantPtr p_value, void *p_userdata) {
	List<Pair<StringName, Variant>> *list = reinterpret_cast<List<Pair<StringName, Variant>> *>(p_userdata);
	list->push_back({ *(const StringName *)p_name, *(const Variant *)p_value });
}

void ScriptInstance::get_property_state(List<Pair<StringName, Variant>> &p_list) {
	get_property_state(add_to_state, &p_list);
}

void ScriptInstance::free_property_list(const GDExtensionPropertyInfo *p_list, uint32_t p_count) const {
	if (!p_list)
		return;

	for (int i = 0; i < p_count; i++)
		free_prop(p_list[i]);

	memfree((GDExtensionPropertyInfo *)p_list);
}

GDExtensionMethodInfo *ScriptInstance::get_method_list(uint32_t *r_count) const {
	LocalVector<GDExtensionMethodInfo> methods;
	HashSet<StringName> defined;

	const LuauScript *s = get_script().ptr();

	while (s) {
		for (const KeyValue<StringName, GDMethod> &pair : s->get_definition().methods) {
			if (defined.has(pair.key))
				continue;

			defined.insert(pair.key);

			const GDMethod &src = pair.value;

			GDExtensionMethodInfo dst;

			dst.name = stringname_alloc(src.name);
			copy_prop(src.return_val, dst.return_value);
			dst.flags = src.flags;
			dst.argument_count = src.arguments.size();

			if (dst.argument_count > 0) {
				GDExtensionPropertyInfo *arg_list = memnew_arr(GDExtensionPropertyInfo, dst.argument_count);

				for (int j = 0; j < dst.argument_count; j++)
					copy_prop(src.arguments[j], arg_list[j]);

				dst.arguments = arg_list;
			}

			dst.default_argument_count = src.default_arguments.size();

			if (dst.default_argument_count > 0) {
				Variant *defargs = memnew_arr(Variant, dst.default_argument_count);

				for (int j = 0; j < dst.default_argument_count; j++)
					defargs[j] = src.default_arguments[j];

				dst.default_arguments = (GDExtensionVariantPtr *)defargs;
			}

			methods.push_back(dst);
		}

		s = s->get_base().ptr();
	}

	int size = methods.size();
	*r_count = size;

	GDExtensionMethodInfo *list = (GDExtensionMethodInfo *)memalloc(sizeof(GDExtensionMethodInfo) * size);
	memcpy(list, methods.ptr(), sizeof(GDExtensionMethodInfo) * size);

	return list;
}

void ScriptInstance::free_method_list(const GDExtensionMethodInfo *p_list, uint32_t p_count) const {
	if (!p_list)
		return;

	for (int i = 0; i < p_count; i++) {
		const GDExtensionMethodInfo &method = p_list[i];

		memdelete((StringName *)method.name);

		free_prop(method.return_value);

		if (method.argument_count > 0) {
			for (int i = 0; i < method.argument_count; i++)
				free_prop(method.arguments[i]);

			memdelete(method.arguments);
		}

		if (method.default_argument_count > 0)
			memdelete((Variant *)method.default_arguments);
	}

	memdelete((GDExtensionMethodInfo *)p_list);
}

ScriptLanguage *ScriptInstance::get_language() const {
	return LuauLanguage::get_singleton();
}




//MARK: LuauScriptInstance
#define INSTANCE_SELF ((LuauScriptInstance *)p_self)

static GDExtensionScriptInstanceInfo3 init_script_instance_info() {
	GDExtensionScriptInstanceInfo3 info;
	ScriptInstance::init_script_instance_info_common(info);

	info.property_can_revert_func = [](void *p_self, GDExtensionConstStringNamePtr p_name) -> GDExtensionBool {
		return INSTANCE_SELF->property_can_revert(*((StringName *)p_name));
	};

	info.property_get_revert_func = [](void *p_self, GDExtensionConstStringNamePtr p_name, GDExtensionVariantPtr r_ret) -> GDExtensionBool {
		return INSTANCE_SELF->property_get_revert(*((StringName *)p_name), (Variant *)r_ret);
	};

	info.call_func = [](void *p_self, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr *p_args, GDExtensionInt p_argument_count, GDExtensionVariantPtr r_return, GDExtensionCallError *r_error) {
		return INSTANCE_SELF->call(*((StringName *)p_method), (const Variant **)p_args, p_argument_count, (Variant *)r_return, r_error);
	};

	info.notification_func = [](void *p_self, int32_t p_what, GDExtensionBool p_reversed) {
		INSTANCE_SELF->notification(p_what);
	};

	info.to_string_func = [](void *p_self, GDExtensionBool *r_is_valid, GDExtensionStringPtr r_out) {
		INSTANCE_SELF->to_string(r_is_valid, (String *)r_out);
	};

	info.free_func = [](void *p_self) {
		memdelete(INSTANCE_SELF);
	};

	info.refcount_decremented_func = [](void *) -> GDExtensionBool {
		// If false (default), object cannot die
		return true;
	};

	return info;
}
const GDExtensionScriptInstanceInfo3 LuauScriptInstance::INSTANCE_INFO = init_script_instance_info();

bool LuauScriptInstance::property_can_revert(const StringName &p_name) {
    #define PROPERTY_CAN_REVERT_NAME "_PropertyCanRevert"

    const LuauScript *s = script.ptr();

    // while (s) {
    //     if (s->methods.has(PROPERTY_CAN_REVERT_NAME)) {
    //         lua_State *ET = lua_newthread(T);

    //         LuaStackOp<String>::push(ET, p_name);
    //         int status = call_internal(PROPERTY_CAN_REVERT_NAME, ET, 1, 1);

    //         if (status != OK) {
    //             lua_pop(T, 1); // thread
    //             return false;
    //         }

    //         if (lua_type(ET, -1) != LUA_TBOOLEAN) {
    //             s->error("LuauScriptInstance::property_can_revert", "Expected " PROPERTY_CAN_REVERT_NAME " to return a boolean", 1);
    //             lua_pop(T, 1); // thread
    //             return false;
    //         }

    //         bool ret = lua_toboolean(ET, -1);
    //         lua_pop(T, 1); // thread
    //         return ret;
    //     }

    //     s = s->base.ptr();
    // }

    return false;
}

bool LuauScriptInstance::property_get_revert(const StringName &p_name, Variant *r_ret) {
    #define PROPERTY_GET_REVERT_NAME "_PropertyGetRevert"

    const LuauScript *s = script.ptr();

    // while (s) {
    //     if (s->methods.has(PROPERTY_GET_REVERT_NAME)) {
    //         lua_State *ET = lua_newthread(T);

    //         LuaStackOp<String>::push(ET, p_name);
    //         int status = call_internal(PROPERTY_GET_REVERT_NAME, ET, 1, 1);

    //         if (status != OK) {
    //             lua_pop(T, 1); // thread
    //             return false;
    //         }

    //         if (!LuaStackOp<Variant>::is(ET, -1)) {
    //             s->error("LuauScriptInstance::property_get_revert", "Expected " PROPERTY_GET_REVERT_NAME " to return a Variant", 1);
    //             lua_pop(T, 1); // thread
    //             return false;
    //         }

    //         *r_ret = LuaStackOp<Variant>::get(ET, -1);
    //         lua_pop(T, 1); // thread
    //         return true;
    //     }

    //     s = s->base.ptr();
    // }

    return false;
}

void LuauScriptInstance::call(
    const StringName &p_method,
    const Variant *const *p_args, const GDExtensionInt p_argument_count,
    Variant *r_return, GDExtensionCallError *r_error) {
    
    if (!L || !T || self_ref == LUA_NOREF) {
        r_error->error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
        return;
    }
    
    const LuauScript *s = script.ptr();
    
    // Look for the method in the script hierarchy
    while (s) {
        if (s->definition.methods.has(p_method)) {
            const GDMethod &method = s->definition.methods[p_method];
            
            // Check argument count
            int args_allowed = method.arguments.size();
            int args_default = method.default_arguments.size();
            int args_required = args_allowed - args_default;
            
            if (p_argument_count < args_required) {
                r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
                r_error->argument = args_required;
                return;
            }
            
            if (p_argument_count > args_allowed && !method.flags.has_flag(METHOD_FLAG_VARARG)) {
                r_error->error = GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS;
                r_error->argument = args_allowed;
                return;
            }
            
            // Create execution thread
            lua_State *ET = lua_newthread(T);
            
            // Push arguments onto the stack
            for (int i = 0; i < p_argument_count; i++) {
                const Variant &arg = *p_args[i];
                LuauMarshal::push_variant(ET, arg);
            }
            
            // Add default arguments if needed
            for (int i = p_argument_count; i < args_allowed; i++) {
                int default_idx = i - (args_allowed - args_default);
                if (default_idx >= 0 && default_idx < args_default) {
                    LuauMarshal::push_variant(ET, method.default_arguments[default_idx]);
                }
            }
            
            // Call the method
            r_error->error = GDEXTENSION_CALL_OK;
            int status = call_internal(p_method, ET, args_allowed, 1);
            
            if (status == LUA_OK) {
                // Get return value
                *r_return = LuauMarshal::get_variant(ET, -1);
            } else {
                *r_return = Variant();
                r_error->error = GDEXTENSION_CALL_ERROR_METHOD_NOT_CONST;
            }
            
            lua_pop(T, 1); // Remove thread
            return;
        }
        
        s = s->base.ptr();
    }
    
    r_error->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
}

void LuauScriptInstance::notification(int32_t p_what) {
    if (!L || !T || self_ref == LUA_NOREF) {
        WARN_PRINT(vformat("Notification %d skipped - invalid Lua state", p_what));
        return;
    }
    
    // Map common notifications to Luau method names
    StringName method_name;
    switch (p_what) {
        case 13: // NOTIFICATION_READY
            method_name = "_ready";
            WARN_PRINT("NOTIFICATION_READY received");
            break;
        case 17: // NOTIFICATION_PROCESS
            method_name = "_process";
            break;
        case 16: // NOTIFICATION_PHYSICS_PROCESS
            method_name = "_physics_process";
            break;
        case 10: // NOTIFICATION_ENTER_TREE
            method_name = "_enter_tree";
            break;
        case 11: // NOTIFICATION_EXIT_TREE
            method_name = "_exit_tree";
            break;
        case 25: // NOTIFICATION_UNPAUSED
            method_name = "_unpaused";
            break;
        case 14: // NOTIFICATION_PAUSED  
            method_name = "_paused";
            break;
        default:
            // For other notifications, try the generic _notification method
            method_name = "_notification";
            break;
    }
    
    // First try the specific method
    if (method_name != StringName("_notification") && has_method(method_name)) {
		if (method_name != StringName("_process")) {
        	WARN_PRINT(vformat("Calling method: %s", String(method_name)));
		}
        lua_State *ET = lua_newthread(T);
        
        // Get the self table from the main state
        lua_getref(L, self_ref);
        lua_xmove(L, ET, 1);
        
        // Get the method from the self table
        String method_str = String(method_name);
        lua_getfield(ET, -1, method_str.utf8().get_data());
        
        if (!lua_isfunction(ET, -1)) {
            lua_pop(ET, 2); // Remove non-function and self table
            lua_pop(T, 1); // Remove thread
            return;
        }
        
        // Swap function and self table so self is first argument
        lua_insert(ET, -2);
        
        // Now push additional arguments if needed
        int argc = 0;
        if (p_what == 17 || p_what == 16) { // NOTIFICATION_PROCESS or NOTIFICATION_PHYSICS_PROCESS
            // Get delta time - we'll pass a default for now
            // TODO: Get actual delta from the engine when available
            double delta = 1.0 / 60.0; // Default to 60 FPS
            lua_pushnumber(ET, delta);
            argc = 1;
        }
        
        // Call: function, self, [args...]
        int call_result = lua_pcall(ET, argc + 1, 0, 0); // +1 for self
        
        if (call_result != LUA_OK) {
            const char* error_msg = lua_tostring(ET, -1);
            if (error_msg) {
                ERR_PRINT(vformat("Luau script error in %s: %s", 
                    method_str, error_msg));
            }
            lua_pop(ET, 1); // Remove error message
        }
        
        lua_pop(T, 1); // Remove thread
    }
    // Then try the generic _notification method
    else if (has_method("_notification")) {
        lua_State *ET = lua_newthread(T);
        
        // Get the self table from the main state
        lua_getref(L, self_ref);
        lua_xmove(L, ET, 1);
        
        // Get the method from the self table
        lua_getfield(ET, -1, "_notification");
        
        if (!lua_isfunction(ET, -1)) {
            lua_pop(ET, 2); // Remove non-function and self table
            lua_pop(T, 1); // Remove thread
            return;
        }
        
        // Swap function and self table so self is first argument
        lua_insert(ET, -2);
        
        // Push the notification code as argument
        lua_pushinteger(ET, p_what);
        
        // Call: function, self, notification_code
        int call_result = lua_pcall(ET, 2, 0, 0); // 1 for self + 1 for notification code
        
        if (call_result != LUA_OK) {
            const char* error_msg = lua_tostring(ET, -1);
            if (error_msg) {
                ERR_PRINT(vformat("Luau script error in _notification: %s", error_msg));
            }
            lua_pop(ET, 1); // Remove error message
        }
        
        lua_pop(T, 1); // Remove thread
    }
}

void LuauScriptInstance::to_string(GDExtensionBool *r_is_valid, String *r_out) {
#define TO_STRING_NAME "_ToString"

    // const LuauScript *s = script.ptr();

    // while (s) {
    //     if (s->methods.has(TO_STRING_NAME)) {
    //         lua_State *ET = lua_newthread(T);

    //         int status = call_internal(TO_STRING_NAME, ET, 0, 1);

    //         if (status == LUA_OK)
    //             *r_out = LuaStackOp<String>::get(ET, -1);

    //         if (r_is_valid)
    //             *r_is_valid = status == LUA_OK;

    //         lua_pop(T, 1); // thread
    //         return;
    //     }

    //     s = s->base.ptr();
    // }
}

bool LuauScriptInstance::set(const StringName &p_name, const Variant &p_value, PropertySetGetError *r_err) {
	if (!L || self_ref == LUA_NOREF) {
		if (r_err) *r_err = PROP_NOT_FOUND;
		return false;
	}
	
	// Get the self table
	lua_getref(L, self_ref);
	
	// Set the value in the self table
	String prop_str = String(p_name);
	LuauMarshal::push_variant(L, p_value);
	lua_setfield(L, -2, prop_str.utf8().get_data());
	
	lua_pop(L, 1); // Remove self table
	
	if (r_err) *r_err = PROP_OK;
	return true;
}

bool LuauScriptInstance::get(const StringName &p_name, Variant &r_ret, PropertySetGetError *r_err) {
    if (!L || self_ref == LUA_NOREF) {
        if (r_err) *r_err = PROP_NOT_FOUND;
        return false;
    }
    
    // Get the self table
    lua_getref(L, self_ref);
    
    // Get the value from the self table
    String prop_str = String(p_name);
    lua_getfield(L, -1, prop_str.utf8().get_data());
    
    // Check if the value exists (not nil)
    if (lua_isnil(L, -1)) {
        lua_pop(L, 2); // Remove nil and self table
        
        // Try to get from script constants as fallback
        const LuauScript *s = script.ptr();
        while (s) {
            if (s->constants.has(p_name)) {
                r_ret = s->constants[p_name];
                if (r_err) *r_err = PROP_OK;
                return true;
            }
            s = s->base.ptr();
        }
        
        if (r_err) *r_err = PROP_NOT_FOUND;
        return false;
    }
    
    // Convert the Lua value to Variant
    r_ret = LuauMarshal::get_variant(L, -1);
    lua_pop(L, 2); // Remove value and self table
    
    if (r_err) *r_err = PROP_OK;
    return true;
}

GDExtensionPropertyInfo *LuauScriptInstance::get_property_list(uint32_t *r_count) {
    // First, get properties from script definition (static properties)
    LocalVector<GDExtensionPropertyInfo> properties;
    HashSet<StringName> seen;
    
    // Add properties from script definition (if any)
    const LuauScript *s = script.ptr();
    while (s) {
        for (const GDClassProperty &script_prop : s->definition.properties) {
            if (!seen.has(script_prop.property.name)) {
                seen.insert(script_prop.property.name);
                
                GDExtensionPropertyInfo prop_info;
                copy_prop(script_prop.property, prop_info);
                properties.push_back(prop_info);
            }
        }
        s = s->base.ptr();
    }
    
    // Only try to get properties from Lua state if it's properly initialized
    if (L && self_ref != LUA_NOREF) {
        // Get properties from the self table
        lua_getref(L, self_ref);
        
        // Check if we got a valid table
        if (lua_istable(L, -1)) {
            // Iterate through the self table
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                // Key is at -2, value is at -1
                if (lua_type(L, -2) == LUA_TSTRING) {
                    const char* key = lua_tostring(L, -2);
                    if (key) {
                        String key_str = String(key);
                        
                        // Skip internal fields (starting with __)
                        if (!key_str.begins_with("__")) {
                            // Skip if it's a function (methods are not properties)
                            if (!lua_isfunction(L, -1)) {
                                StringName prop_name(key_str);
                                
                                if (!seen.has(prop_name)) {
                                    seen.insert(prop_name);
                                    
                                    GDExtensionPropertyInfo prop_info;
                                    prop_info.type = GDEXTENSION_VARIANT_TYPE_NIL;
                                    prop_info.name = stringname_alloc(prop_name);
                                    prop_info.class_name = stringname_alloc("");
                                    prop_info.hint = PROPERTY_HINT_NONE;
                                    prop_info.hint_string = string_alloc("");
                                    prop_info.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE;
                                    
                                    // Try to determine the type from the current value
                                    int lua_type_id = lua_type(L, -1);
                                    switch (lua_type_id) {
                                        case LUA_TBOOLEAN:
                                            prop_info.type = GDEXTENSION_VARIANT_TYPE_BOOL;
                                            break;
                                        case LUA_TNUMBER:
                                            prop_info.type = GDEXTENSION_VARIANT_TYPE_FLOAT;
                                            break;
                                        case LUA_TSTRING:
                                            prop_info.type = GDEXTENSION_VARIANT_TYPE_STRING;
                                            break;
                                        case LUA_TTABLE:
                                            prop_info.type = GDEXTENSION_VARIANT_TYPE_DICTIONARY;
                                            break;
                                        default:
                                            prop_info.type = GDEXTENSION_VARIANT_TYPE_NIL;
                                            prop_info.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
                                            break;
                                    }
                                    
                                    properties.push_back(prop_info);
                                }
                            }
                        }
                    }
                }
                lua_pop(L, 1); // Remove value, keep key for next iteration
            }
        }
        
        lua_pop(L, 1); // Remove self table
    }
    
    // Copy to allocated memory
    int size = properties.size();
    *r_count = size;
    
    if (size == 0) {
        return nullptr;
    }
    
    GDExtensionPropertyInfo *list = (GDExtensionPropertyInfo *)memalloc(sizeof(GDExtensionPropertyInfo) * size);
    memcpy(list, properties.ptr(), sizeof(GDExtensionPropertyInfo) * size);
    
    return list;
}

Variant::Type godot::LuauScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
    if (!L || self_ref == LUA_NOREF) {
        if (r_is_valid) *r_is_valid = false;
        return Variant::NIL;
    }
    
    // Get the self table
    lua_getref(L, self_ref);
    
    // Get the value from the self table
    String prop_str = String(p_name);
    lua_getfield(L, -1, prop_str.utf8().get_data());
    
    Variant::Type type = Variant::NIL;
    bool valid = false;
    
    if (!lua_isnil(L, -1)) {
        valid = true;
        
        int lua_type_id = lua_type(L, -1);
        switch (lua_type_id) {
            case LUA_TBOOLEAN:
                type = Variant::BOOL;
                break;
            case LUA_TNUMBER:
                type = Variant::FLOAT;
                break;
            case LUA_TSTRING:
                type = Variant::STRING;
                break;
            case LUA_TTABLE:
                type = Variant::DICTIONARY;
                break;
            default:
                type = Variant::NIL;
                break;
        }
    }
    
    lua_pop(L, 2); // Remove value and self table
    
    if (r_is_valid) *r_is_valid = valid;
    return type;
}

bool LuauScriptInstance::has_method(const StringName &p_name) const {
    // First check the actual self table for runtime methods
    if (L && self_ref != LUA_NOREF) {
        lua_getref(L, self_ref);
        String method_str = String(p_name);
        lua_getfield(L, -1, method_str.utf8().get_data());
        bool is_func = lua_isfunction(L, -1);
        lua_pop(L, 2); // Remove function/nil and self table
        if (is_func) {
            return true;
        }
    }
    
    // Then fall back to checking the script metadata
    const LuauScript *s = script.ptr();
    while (s) {
        if (s->definition.methods.has(p_name)) {
            return true;
        }
        s = s->base.ptr();
    }
    return false;
}

Object *LuauScriptInstance::get_owner() const
{
    return owner;
}

Ref<LuauScript> LuauScriptInstance::get_script() const
{
    return script;
}

int LuauScriptInstance::call_internal(const StringName &p_method, lua_State *ET, int argc, int retc) {
    if (!L || !T || self_ref == LUA_NOREF) {
        return LUA_ERRRUN;
    }
    
    // Get the self table from the main state
    lua_getref(L, self_ref);
    lua_xmove(L, ET, 1);
    
    // Get the method from the self table
    String method_str = String(p_method);
    lua_getfield(ET, -1, method_str.utf8().get_data());
    
    if (!lua_isfunction(ET, -1)) {
        lua_pop(ET, 2); // Remove non-function and self table
        return LUA_ERRRUN;
    }
    
    // Swap function and self table so self is first argument
    lua_insert(ET, -2);
    
    // The arguments are already on the stack, placed by the caller
    // So we have: function, self, [args...]
    int call_result = lua_pcall(ET, argc + 1, retc, 0); // +1 for self
    
    if (call_result != LUA_OK) {
        const char* error_msg = lua_tostring(ET, -1);
        if (error_msg) {
            ERR_PRINT(vformat("Luau script error in %s: %s", 
                p_method, error_msg));
        }
        lua_pop(ET, 1); // Remove error message
    }
    
    return call_result;
}

LuauScriptInstance::LuauScriptInstance(
	const Ref<LuauScript> &p_script, 
	Object *p_owner, 
	LuauEngine::VMType p_vmtype) 
	: 
	script(p_script), 
	owner(p_owner), 
	vm_type(p_vmtype) {

}

LuauScriptInstance::~LuauScriptInstance() {
	// Clean up Lua state
	if (L && thread_ref != LUA_NOREF) {
		lua_unref(L, thread_ref);
		thread_ref = LUA_NOREF;
	}
	
	if (L && self_ref != LUA_NOREF) {
		lua_unref(L, self_ref);
		self_ref = LUA_NOREF;
	}
	
	L = nullptr;
	T = nullptr;
}

//MARK: PlaceholderScriptInstance
#ifdef TOOLS_ENABLED
#define PLACEHOLDER_SELF ((PlaceHolderScriptInstance *)p_self)

static GDExtensionScriptInstanceInfo3 init_placeholder_instance_info() {
	// Methods which essentially have no utility (e.g. call) are implemented here instead of in the class.

	GDExtensionScriptInstanceInfo3 info;
	ScriptInstance::init_script_instance_info_common(info);

	info.property_can_revert_func = [](void *, GDExtensionConstStringNamePtr) -> GDExtensionBool {
		return false;
	};

	info.property_get_revert_func = [](void *, GDExtensionConstStringNamePtr, GDExtensionVariantPtr) -> GDExtensionBool {
		return false;
	};

	info.call_func = [](void *p_self, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr *p_args, GDExtensionInt p_argument_count, GDExtensionVariantPtr r_return, GDExtensionCallError *r_error) {
		r_error->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
		*(Variant *)r_return = Variant();
	};

	info.is_placeholder_func = [](void *) -> GDExtensionBool {
		return true;
	};

	info.set_fallback_func = [](void *p_self, GDExtensionConstStringNamePtr p_name, GDExtensionConstVariantPtr p_value) -> GDExtensionBool {
		return PLACEHOLDER_SELF->property_set_fallback(*(const StringName *)p_name, *(const Variant *)p_value);
	};

	info.get_fallback_func = [](void *p_self, GDExtensionConstStringNamePtr p_name, GDExtensionVariantPtr r_ret) -> GDExtensionBool {
		return PLACEHOLDER_SELF->property_get_fallback(*(const StringName *)p_name, *(Variant *)r_ret);
	};

	info.free_func = [](void *p_self) {
		memdelete(PLACEHOLDER_SELF);
	};

	return info;
}

const GDExtensionScriptInstanceInfo3 PlaceHolderScriptInstance::INSTANCE_INFO = init_placeholder_instance_info();

bool PlaceHolderScriptInstance::set(const StringName &p_name, const Variant &p_value, PropertySetGetError *r_err) {
	if (script->_is_placeholder_fallback_enabled()) {
		if (r_err)
			*r_err = PROP_NOT_FOUND;

		return false;
	}

	if (values.has(p_name)) {
		if (script->_has_property_default_value(p_name)) {
			Variant defval = script->_get_property_default_value(p_name);

			Variant op_result;
			bool op_valid = false;
			Variant::evaluate(Variant::OP_EQUAL, defval, p_value, op_result, op_valid);

			if (op_valid && op_result.operator bool()) {
				values.erase(p_name);
				return true;
			}
		}

		values[p_name] = p_value;
		return true;
	} else {
		if (script->_has_property_default_value(p_name)) {
			Variant defval = script->get_property_default_value(p_name);

			Variant op_result;
			bool op_valid = false;
			Variant::evaluate(Variant::OP_NOT_EQUAL, defval, p_value, op_result, op_valid);

			if (op_valid && op_result.operator bool())
				values[p_name] = p_value;

			return true;
		}
	}

	if (r_err)
		*r_err = PROP_NOT_FOUND;

	return false;
}

bool PlaceHolderScriptInstance::get(const StringName &p_name, Variant &r_ret, PropertySetGetError *r_err) {
	if (values.has(p_name)) {
		r_ret = values[p_name];
		return true;
	}

	if (constants.has(p_name)) {
		r_ret = constants[p_name];
		return true;
	}

	if (!script->_is_placeholder_fallback_enabled() && script->_has_property_default_value(p_name)) {
		r_ret = script->_get_property_default_value(p_name);
		return true;
	}

	if (r_err)
		*r_err = PROP_NOT_FOUND;

	return false;
}

bool PlaceHolderScriptInstance::property_set_fallback(const StringName &p_name, const Variant &p_value) {
	if (script->_is_placeholder_fallback_enabled()) {
		HashMap<StringName, Variant>::Iterator E = values.find(p_name);

		if (E) {
			E->value = p_value;
		} else {
			values.insert(p_name, p_value);
		}

		bool found = false;
		for (const GDProperty &F : properties) {
			if (F.name == p_name) {
				found = true;
				break;
			}
		}

		if (!found) {
			GDProperty pinfo;

			pinfo.type = (GDExtensionVariantType)p_value.get_type();
			pinfo.name = p_name;
			pinfo.usage = PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_SCRIPT_VARIABLE;

			properties.push_back(pinfo);
		}
	}

	return false;
}

bool PlaceHolderScriptInstance::property_get_fallback(const StringName &p_name, Variant &r_ret) {
	if (script->_is_placeholder_fallback_enabled()) {
		HashMap<StringName, Variant>::ConstIterator E = values.find(p_name);

		if (E) {
			r_ret = E->value;
			return true;
		}

		E = constants.find(p_name);

		if (E) {
			r_ret = E->value;
			return true;
		}
	}

	r_ret = Variant();
	return false;
}

GDExtensionPropertyInfo *PlaceHolderScriptInstance::get_property_list(uint32_t *r_count) {
	LocalVector<GDExtensionPropertyInfo> props;

	int size = properties.size();
	props.resize(size);

	if (script->_is_placeholder_fallback_enabled()) {
		for (int i = 0; i < size; i++) {
			GDExtensionPropertyInfo dst;
			copy_prop(properties[i], dst);

			props[i] = dst;
		}
	} else {
		for (int i = 0; i < size; i++) {
			GDExtensionPropertyInfo &pinfo = props[i];
			copy_prop(properties[i], pinfo);

			if (!values.has(properties[i].name))
				pinfo.usage |= PROPERTY_USAGE_SCRIPT_DEFAULT_VALUE;
		}
	}

	*r_count = size;

	GDExtensionPropertyInfo *list = (GDExtensionPropertyInfo *)memalloc(sizeof(GDExtensionPropertyInfo) * size);
	memcpy(list, props.ptr(), sizeof(GDExtensionPropertyInfo) * size);

	return list;
}

Variant::Type PlaceHolderScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
	if (values.has(p_name)) {
		if (r_is_valid)
			*r_is_valid = true;

		return values[p_name].get_type();
	}

	if (constants.has(p_name)) {
		if (r_is_valid)
			*r_is_valid = true;

		return constants[p_name].get_type();
	}

	if (r_is_valid)
		*r_is_valid = false;

	return Variant::NIL;
}

bool PlaceHolderScriptInstance::has_method(const StringName &p_name) const {
	if (script->_is_placeholder_fallback_enabled())
		return false;

	if (script.is_valid())
		return script->_has_method(p_name);

	return false;
}

PlaceHolderScriptInstance::PlaceHolderScriptInstance(const Ref<LuauScript> &p_script, Object *p_owner) {
    script = p_script;
    owner = p_owner;

	// Placeholder instance creation takes place in a const method.
	script->placeholders.insert(p_owner->get_instance_id(), this);
	//script->update_exports_internal(this);
}

PlaceHolderScriptInstance::~PlaceHolderScriptInstance() {
    if (script.is_valid()) {
		script->_placeholder_erased(this);
	}
}

#endif // TOOLS_ENABLED

bool LuauScript::_has_source_code() const {
    return !source.is_empty();
}

String LuauScript::_get_source_code() const {
    return source;
}

// MARK: LuauScript
void LuauScript::_set_source_code(const String &p_code) {
    source = p_code;
    source_changed_cache = true;
    
    // Clear compilation state when source changes
    load_stage = LOAD_NONE;
    bytecode.clear();
}

Error LuauScript::_reload(bool p_keep_state) {
	// if (_is_module)
	// 	return OK;

	Error reload_err = OK;
	
#ifdef TOOLS_ENABLED
	// In the editor, handle existing instances appropriately
	if (!p_keep_state && instances.size() > 0) {
		MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
		
		// In editor mode, we allow reload with existing instances
		// The instances will be updated after the script is reloaded
		placeholder_fallback_enabled = true;
		
		// Store instance IDs for later update
		LocalVector<uint64_t> instance_ids;
		for (const KeyValue<uint64_t, LuauScriptInstance *> &E : instances) {
			instance_ids.push_back(E.key);
		}
		
		// Note: We keep the instances alive during reload
		// They will be updated after the script is successfully reloaded
	}
#else
	{
		MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
		ERR_FAIL_COND_V(!p_keep_state && instances.size() > 0, ERR_ALREADY_IN_USE);
	}
#endif // TOOLS_ENABLED

	// Reload source code from file if path is set
	String path = get_path();
	if (!path.is_empty()) {
		Error err = load_source_code(path);
		if (err != OK) {
			ERR_PRINT(vformat("Failed to reload source code from %s", path));
			return err;
		}
	}

	// Clear cached data to force recompilation
	load_stage = LOAD_NONE;
	bytecode.clear();
	
	// Reload and recompile the script
	reload_err = load(LOAD_FULL, true);
	
#ifdef TOOLS_ENABLED
	// After successful reload, update instances if needed
	if (reload_err == OK) {
		if (placeholder_fallback_enabled) {
			// Update placeholder instances with new script information
			MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
			
			// Update all placeholder instances
			for (const KeyValue<uint64_t, PlaceHolderScriptInstance *> &E : placeholders) {
				PlaceHolderScriptInstance *placeholder = E.value;
				if (placeholder) {
					// Update the property list and constants using the accessor methods
					placeholder->update_properties(definition.properties);
					placeholder->update_constants(constants);
				}
			}
		}
		
		// Update regular instances if keeping state
		if (p_keep_state) {
			MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
			
			// Notify all instances that the script has been reloaded
			for (const KeyValue<uint64_t, LuauScriptInstance *> &E : instances) {
				LuauScriptInstance *instance = E.value;
				if (instance) {
					// Re-initialize the instance with the new bytecode
					// This would involve re-running the script in the instance's Lua state
					// For now, we just mark that a reload happened
					// Full implementation would reload the bytecode into the instance
				}
			}
		}
		
		// Emit changed signal to notify the editor
		emit_changed();
	}
#endif // TOOLS_ENABLED
	
	return reload_err;
}

TypedArray<Dictionary> LuauScript::_get_documentation() const {
	return TypedArray<Dictionary>();
}

bool LuauScript::_has_static_method(const StringName &p_method) const {
    return false;
}

bool LuauScript::_is_tool() const {
	return definition.is_tool;
}

bool LuauScript::_is_valid() const {
	if (source.is_empty()) {
		return false;
	}
	
	if (load_stage < LOAD_COMPILE) {
		const_cast<LuauScript*>(this)->load(LOAD_COMPILE, false);
	}
	
	return load_stage >= LOAD_COMPILE && bytecode.size() > 0;
}

ScriptLanguage *LuauScript::_get_language() const {
    return LuauLanguage::get_singleton();
}

TypedArray<Dictionary> LuauScript::_get_script_signal_list() const {
	TypedArray<Dictionary> signals;

	const LuauScript *s = this;

	while (s) {
		for (const KeyValue<StringName, GDMethod> &pair : s->definition.signals)
			signals.push_back(pair.value);

		s = s->base.ptr();
	}

	return signals;
}

bool LuauScript::_has_property_default_value(const StringName &p_property) const {
	HashMap<StringName, uint64_t>::ConstIterator E = definition.property_indices.find(p_property);

	if (E && definition.properties[E->value].default_value != Variant())
		return true;

	if (base.is_valid())
		return base->_has_property_default_value(p_property);

	return false;
}

Variant LuauScript::_get_property_default_value(const StringName &p_property) const {
	HashMap<StringName, uint64_t>::ConstIterator E = definition.property_indices.find(p_property);

	if (E && definition.properties[E->value].default_value != Variant())
		return definition.properties[E->value].default_value;

	if (base.is_valid())
		return base->_get_property_default_value(p_property);

	return Variant();
}

void LuauScript::_update_exports() {
#ifdef TOOLS_ENABLED
	// if (_is_module)
	// 	return;

	// update_exports_internal(nullptr);

	// Update old dependent scripts.
	List<Ref<LuauScript>> scripts = LuauLanguage::get_singleton()->get_scripts();

	// for (Ref<LuauScript> &scr : scripts) {
	// 	// Check dependent to avoid endless loop.
	// 	if (scr->has_dependency(this) && !this->has_dependency(scr))
	// 		scr->_update_exports();
	// }
#endif // TOOLS_ENABLED
}

TypedArray<Dictionary> LuauScript::_get_script_property_list() const {
	TypedArray<Dictionary> properties;

	const LuauScript *s = this;

	// while (s) {
	// 	// Reverse to add properties from base scripts first.
	// 	for (int i = definition.properties.size() - 1; i >= 0; i--) {
	// 		const GDClassProperty &prop = definition.properties[i];
	// 		properties.push_front(prop.property.operator Dictionary());
	// 	}

	// 	s = s->base.ptr();
	// }

	return properties;
}

Dictionary LuauScript::_get_constants() const {
	Dictionary constants_dict;

	for (const KeyValue<StringName, Variant> &pair : constants)
		constants_dict[pair.key] = pair.value;

	return constants_dict;
}

bool LuauScript::_is_placeholder_fallback_enabled() const {
#ifdef TOOLS_ENABLED
	return placeholder_fallback_enabled;
#else
	return false;
#endif // TOOLS_ENABLED
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
    if (!p_force && load_stage >= p_load_stage) {
        return OK;
    }
    
    // Check if source is empty
    if (source.is_empty()) {
        ERR_FAIL_V_MSG(ERR_INVALID_DATA, "Script source is empty");
    }
    
    Error err = OK;
    
    // Stage 1: Compile the script
    if (p_load_stage >= LOAD_COMPILE) {
        // Convert Godot String to std::string for Luau
        CharString utf8 = source.utf8();
        std::string source_str(utf8.get_data(), utf8.length());
        
        // Set up compile options
        Luau::CompileOptions compile_opts;
        compile_opts.optimizationLevel = 2; // Full optimization
        compile_opts.debugLevel = 2; // Full debug info
        compile_opts.typeInfoLevel = 1; // Generate type info for all modules
        compile_opts.coverageLevel = 0; // No coverage by default
        
        try {
            // Compile the source code to bytecode
            std::string compiled = Luau::compile(source_str, compile_opts);
            
            // Store the compiled bytecode
            bytecode.resize(compiled.size());
            memcpy(bytecode.ptrw(), compiled.data(), compiled.size());
            
            load_stage = LOAD_COMPILE;
        } catch (const Luau::CompileError &e) {
            ERR_FAIL_V_MSG(ERR_COMPILATION_FAILED, 
                vformat("Luau compilation failed at line %d: %s", 
                    e.getLocation().begin.line + 1, e.what()));
        } catch (...) {
            ERR_FAIL_V_MSG(ERR_COMPILATION_FAILED, "Unknown compilation error");
        }
    }
    
    // Stage 2: Parse and analyze the script for metadata
    if (p_load_stage >= LOAD_ANALYSIS) {
        // Parse the source to extract metadata
        CharString utf8 = source.utf8();
        std::string source_str(utf8.get_data(), utf8.length());
        
        // Create allocator and name table for AST
        Luau::Allocator allocator;
        Luau::AstNameTable names(allocator);
        
        // Parse the source
        Luau::ParseOptions parse_opts;
        Luau::ParseResult parse_result = Luau::Parser::parse(
            source_str.c_str(), source_str.size(), names, allocator, parse_opts);
        
        if (!parse_result.errors.empty()) {
            // Report first parse error
            const auto &error = parse_result.errors[0];
            ERR_FAIL_V_MSG(ERR_PARSE_ERROR,
                vformat("Parse error at line %d: %s",
                    error.getLocation().begin.line + 1, error.getMessage().c_str()));
        }
        
        // Extract metadata from AST
        if (parse_result.root) {
            // Clear existing metadata
            definition.methods.clear();
            definition.properties.clear();
            definition.property_indices.clear();
            definition.signals.clear();
            definition.constants.clear();
            constants.clear();
            
            // Parse comment annotations first
            // Look for annotations like @extends, @class, @tool, etc.
            // These are typically in comments at the top of the file
            {
                // Parse source line by line to find comment annotations
                PackedStringArray lines_packed = source.split("\n");
                for (int i = 0; i < lines_packed.size(); i++) {
                    String line = lines_packed[i];
                    String trimmed = line.strip_edges();
                    
                    // Look for comment lines starting with --- or --
                    if (trimmed.begins_with("---") || trimmed.begins_with("--")) {
                        // Remove comment prefix
                        String comment = trimmed.substr(trimmed.begins_with("---") ? 3 : 2).strip_edges();
                        
                        // Check for @extends annotation
                        if (comment.begins_with("@extends ")) {
                            String base_class = comment.substr(9).strip_edges();
                            if (!base_class.is_empty()) {
                                definition.extends = base_class;
                            }
                        }
                        // Check for @class annotation
                        else if (comment.begins_with("@class ")) {
                            String class_name = comment.substr(7).strip_edges();
                            if (!class_name.is_empty()) {
                                definition.name = class_name;
                            }
                        }
                        // Check for @tool annotation
                        else if (comment == "@tool") {
                            definition.is_tool = true;
                        }
                    }
                    // Stop parsing after we hit non-comment content (optimization)
                    else if (!trimmed.is_empty() && !trimmed.begins_with("--")) {
                        // We've hit actual code, annotations should be at the top
                        break;
                    }
                }
            }
            
            // Walk through the AST to extract additional metadata
            // This handles actual code structure (functions, constants, etc.)
            
            // For now, just extract basic information from the AST
            for (Luau::AstStat* stat : parse_result.root->body) {
                // Check for global variable assignments (e.g., ACONST = 123)
                if (auto* assign = stat->as<Luau::AstStatAssign>()) {
                    // Process global assignments
                    for (size_t i = 0; i < assign->vars.size; i++) {
                        if (i < assign->values.size && assign->values.data[i]) {
                            // Check if it's a simple global variable (not a table access)
                            if (auto* global = assign->vars.data[i]->as<Luau::AstExprGlobal>()) {
                                Luau::AstExpr* value = assign->values.data[i];
                                
                                // Get the variable name
                                String var_name = String(global->name.value);
                                bool is_constant = true;
                                
                                // Check if all alphabetic characters are uppercase (constant convention)
                                for (int j = 0; j < var_name.length(); j++) {
                                    char32_t c = var_name[j];
                                    if ((c >= 'a' && c <= 'z')) {
                                        is_constant = false;
                                        break;
                                    }
                                }
                                
                                // Extract the value
                                Variant var_value;
                                bool has_value = false;
                                
                                if (auto* num = value->as<Luau::AstExprConstantNumber>()) {
                                    var_value = num->value;
                                    has_value = true;
                                } else if (auto* str = value->as<Luau::AstExprConstantString>()) {
                                    var_value = String::utf8(str->value.data, str->value.size);
                                    has_value = true;
                                } else if (auto* bool_val = value->as<Luau::AstExprConstantBool>()) {
                                    var_value = bool_val->value;
                                    has_value = true;
                                } else if (value->as<Luau::AstExprConstantNil>()) {
                                    var_value = Variant();
                                    has_value = true;
                                }
                                
                                if (has_value) {
                                    if (is_constant) {
                                        // ALL_CAPS variables are constants
                                        constants[StringName(var_name)] = var_value;
                                        definition.constants[StringName(var_name)] = var_value;
                                    } else {
                                        // Regular global variables are properties
                                        GDClassProperty prop;
                                        prop.property.name = StringName(var_name);
                                        prop.property.class_name = "";
                                        prop.property.hint = PROPERTY_HINT_NONE;
                                        prop.property.hint_string = "";
                                        prop.property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE;
                                        prop.default_value = var_value;
                                        
                                        // Determine type from value
                                        switch (var_value.get_type()) {
                                            case Variant::BOOL:
                                                prop.property.type = GDEXTENSION_VARIANT_TYPE_BOOL;
                                                break;
                                            case Variant::INT:
                                                prop.property.type = GDEXTENSION_VARIANT_TYPE_INT;
                                                break;
                                            case Variant::FLOAT:
                                                prop.property.type = GDEXTENSION_VARIANT_TYPE_FLOAT;
                                                break;
                                            case Variant::STRING:
                                                prop.property.type = GDEXTENSION_VARIANT_TYPE_STRING;
                                                break;
                                            default:
                                                prop.property.type = GDEXTENSION_VARIANT_TYPE_NIL;
                                                prop.property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE | PROPERTY_USAGE_NIL_IS_VARIANT;
                                                break;
                                        }
                                        
                                        // Add to properties list
                                        definition.properties.push_back(prop);
                                        definition.property_indices[StringName(var_name)] = definition.properties.size() - 1;
                                    }
                                }
                            }
                        }
                    }
                }
                // Check for local variable assignments
                else if (auto* local = stat->as<Luau::AstStatLocal>()) {
                    // Process local variables
                    for (size_t i = 0; i < local->vars.size; i++) {
                        if (i < local->values.size && local->values.data[i]) {
                            Luau::AstLocal* var = local->vars.data[i];
                            Luau::AstExpr* value = local->values.data[i];
                            
                            // Check if variable name is all caps (constant) or regular (property)
                            String var_name = String(var->name.value);
                            bool is_constant = true;
                            
                            // Check if all alphabetic characters are uppercase
                            for (int j = 0; j < var_name.length(); j++) {
                                char32_t c = var_name[j];
                                if ((c >= 'a' && c <= 'z')) {
                                    is_constant = false;
                                    break;
                                }
                            }
                            
                            // Extract the value
                            Variant var_value;
                            bool has_value = false;
                            
                            if (auto* num = value->as<Luau::AstExprConstantNumber>()) {
                                var_value = num->value;
                                has_value = true;
                            } else if (auto* str = value->as<Luau::AstExprConstantString>()) {
                                var_value = String::utf8(str->value.data, str->value.size);
                                has_value = true;
                            } else if (auto* bool_val = value->as<Luau::AstExprConstantBool>()) {
                                var_value = bool_val->value;
                                has_value = true;
                            } else if (value->as<Luau::AstExprConstantNil>()) {
                                var_value = Variant();
                                has_value = true;
                            }
                            
                            if (has_value) {
                                if (is_constant) {
                                    // ALL_CAPS variables are constants
                                    constants[StringName(var_name)] = var_value;
                                    definition.constants[StringName(var_name)] = var_value;
                                } else {
                                    // Regular variables are properties
                                    GDClassProperty prop;
                                    prop.property.name = StringName(var_name);
                                    prop.property.class_name = "";
                                    prop.property.hint = PROPERTY_HINT_NONE;
                                    prop.property.hint_string = "";
                                    prop.property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE;
                                    prop.default_value = var_value;
                                    
                                    // Determine type from value
                                    switch (var_value.get_type()) {
                                        case Variant::BOOL:
                                            prop.property.type = GDEXTENSION_VARIANT_TYPE_BOOL;
                                            break;
                                        case Variant::INT:
                                            prop.property.type = GDEXTENSION_VARIANT_TYPE_INT;
                                            break;
                                        case Variant::FLOAT:
                                            prop.property.type = GDEXTENSION_VARIANT_TYPE_FLOAT;
                                            break;
                                        case Variant::STRING:
                                            prop.property.type = GDEXTENSION_VARIANT_TYPE_STRING;
                                            break;
                                        default:
                                            prop.property.type = GDEXTENSION_VARIANT_TYPE_NIL;
                                            prop.property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE | PROPERTY_USAGE_NIL_IS_VARIANT;
                                            break;
                                    }
                                    
                                    // Add to properties list
                                    definition.properties.push_back(prop);
                                    definition.property_indices[StringName(var_name)] = definition.properties.size() - 1;
                                }
                            }
                        }
                    }
                }
                
                // Check for function definitions
                if (auto* func = stat->as<Luau::AstStatFunction>()) {
                    // Extract function name
                    if (func->name) {
                        String func_name;
                        String method_name;
                        
                        // Build the function name from the expression
                        if (auto* index = func->name->as<Luau::AstExprIndexName>()) {
                            // Table.method format (e.g., MyClass._init)
                            if (auto* local = index->expr->as<Luau::AstExprLocal>()) {
                                func_name = String(local->local->name.value) + "." + String(index->index.value);
                                method_name = String(index->index.value);
                            } else if (auto* global = index->expr->as<Luau::AstExprGlobal>()) {
                                func_name = String(global->name.value) + "." + String(index->index.value);
                                method_name = String(index->index.value);
                            }
                        } else if (auto* local = func->name->as<Luau::AstExprLocal>()) {
                            func_name = String(local->local->name.value);
                            method_name = func_name;
                        } else if (auto* global = func->name->as<Luau::AstExprGlobal>()) {
                            func_name = String(global->name.value);
                            method_name = func_name;
                        }
                        
                        // Check if this is a special method (starts with underscore) or common Godot method
                        if (!method_name.is_empty()) {
                            bool should_register = false;
                            
                            // Check for underscore methods or common Godot methods
                            if (method_name.begins_with("_")) {
                                should_register = true;
                            } else if (method_name == "ready" || method_name == "process" || 
                                     method_name == "physics_process" || method_name == "input" ||
                                     method_name == "unhandled_input" || method_name == "draw") {
                                // Also register common Godot methods without underscore
                                should_register = true;
                                method_name = "_" + method_name; // Add underscore prefix
                            }
                            
                            if (should_register) {
                                // Register as a method
                                GDMethod method;
                                method.name = method_name;
                                method.flags = METHOD_FLAGS_DEFAULT;
                                
                                // Extract parameters
                                if (func->func) {
                                    for (size_t i = 0; i < func->func->args.size; i++) {
                                        // Skip 'self' parameter if it's explicitly marked
                                        if (i == 0 && func->func->self) {
                                            continue;
                                        }
                                        
                                        GDProperty arg;
                                        arg.name = String(func->func->args.data[i]->name.value);
                                        arg.type = GDEXTENSION_VARIANT_TYPE_NIL; // Default to Variant
                                        arg.usage = PROPERTY_USAGE_NIL_IS_VARIANT;
                                        method.arguments.push_back(arg);
                                    }
                                    
                                    // Check for vararg
                                    if (func->func->vararg) {
                                        method.flags.set_flag(METHOD_FLAG_VARARG);
                                    }
                                }
                                
                                definition.methods[StringName(method_name)] = method;
                            }
                        }
                    }
                }
            }
            
            // Look for special comments/annotations at the top of the file
            // In a full implementation, we would parse comments for:
            // --- @class ClassName
            // --- @extends BaseClass
            // --- @tool
            // etc.
            
            // For now, set some defaults
            if (definition.name.is_empty()) {
                // Try to extract name from path
                String path = get_path();
                if (!path.is_empty()) {
                    definition.name = path.get_file().get_basename();
                }
            }
            
            if (definition.extends.is_empty()) {
                definition.extends = "RefCounted"; // Default base class
            }
        }
        
        load_stage = LOAD_ANALYSIS;
    }
    
    // Stage 3: Full loading (link base scripts, validate, etc.)
    if (p_load_stage >= LOAD_FULL) {
        // Resolve base script references if extends another Luau script
        if (!definition.extends.is_empty()) {
            // Check if the base is a script path (starts with res://)
            if (definition.extends.begins_with("res://")) {
                // Load the base script
                Ref<LuauScript> base_script = ResourceLoader::get_singleton()->load(definition.extends);
                if (base_script.is_valid()) {
                    // Ensure base script is fully loaded
                    base_script->load(LOAD_FULL, false);
                    base = base_script;
                } else {
                    WARN_PRINT(vformat("Failed to load base script: %s", definition.extends));
                }
            }
            // Otherwise it's a built-in class name, which is handled by ClassDB
        }
        
        // Validate that all methods are properly formed
        for (const KeyValue<StringName, GDMethod> &E : definition.methods) {
            const GDMethod &method = E.value;
            
            // Validate method arguments
            for (const GDProperty &arg : method.arguments) {
                // For now, we accept all argument types as Variant
                // In the future, we could perform type checking here
            }
        }
        
        // Process signals if any were defined
        for (const KeyValue<StringName, GDMethod> &E : definition.signals) {
            const GDMethod &signal = E.value;
            // Signals are validated during registration
        }
        
        // Process properties if any were defined  
        for (const GDClassProperty &prop : definition.properties) {
            // Properties are validated during registration
        }
        
        // Mark as fully loaded
        load_stage = LOAD_FULL;
        
#ifdef TOOLS_ENABLED
        // In editor, print summary of what was loaded
        if (Engine::get_singleton()->is_editor_hint()) {
            int method_count = definition.methods.size();
            int property_count = definition.properties.size();
            int signal_count = definition.signals.size();
            int constant_count = definition.constants.size();
            
            if (method_count > 0 || property_count > 0 || signal_count > 0) {
                print_verbose(vformat("LuauScript loaded: %s (extends %s) - %d methods, %d properties, %d signals, %d constants",
                    definition.name.is_empty() ? get_path() : definition.name,
                    definition.extends.is_empty() ? "RefCounted" : definition.extends,
                    method_count, property_count, signal_count, constant_count));
            }
        }
#endif
    }
    
    return err;
}

StringName LuauScript::_get_instance_base_type() const {
	StringName extends = StringName(definition.extends);

	if (extends != StringName() && nobind::ClassDB::get_singleton()->class_exists(extends))
		return extends;

	if (base.is_valid() && base->_is_valid())
		return base->_get_instance_base_type();

	return StringName();
}

void *LuauScript::_instance_create(Object *p_for_object) const {
#ifdef TOOLS_ENABLED
	WARN_PRINT(vformat("Creating LuauScript instance for object: %s", p_for_object->get_class()));
	// In the editor, check if we should create a placeholder instance instead
	bool should_create_placeholder = false;
	
	// Check if script can be instantiated
	if (!_can_instantiate()) {
		should_create_placeholder = true;
	}
	
	// Check if script is properly loaded
	if (!should_create_placeholder && load_stage != LOAD_FULL) {
		// Try to load the script if not already loaded
		const_cast<LuauScript*>(this)->load(LOAD_FULL, false);
		if (load_stage != LOAD_FULL) {
			// Script failed to load fully, use placeholder
			should_create_placeholder = true;
		}
	}
	
	// Check base type compatibility
	if (!should_create_placeholder) {
		StringName base_type = _get_instance_base_type();
		if (base_type != StringName()) {
			if (!nobind::ClassDB::get_singleton()->is_parent_class(p_for_object->get_class(), base_type)) {
				// Type mismatch, use placeholder
				should_create_placeholder = true;
			}
		}
	}
	
	// If we should create a placeholder, do so
	if (should_create_placeholder) {
		// Enable placeholder fallback mode
		const_cast<LuauScript*>(this)->placeholder_fallback_enabled = true;
		
		// Create and return a placeholder instance
		return _placeholder_instance_create(p_for_object);
	}
#else
	// In non-editor builds, fail if script cannot be instantiated
	if (!_can_instantiate()) {
		ERR_FAIL_V_MSG(nullptr, "Script cannot be instantiated");
	}
	
	// Check if script is properly loaded
	if (load_stage != LOAD_FULL) {
		// Try to load the script if not already loaded
		const_cast<LuauScript*>(this)->load(LOAD_FULL, false);
		if (load_stage != LOAD_FULL) {
			ERR_FAIL_V_MSG(nullptr, "Script is not fully loaded and cannot be instantiated");
		}
	}
	
	StringName base_type = _get_instance_base_type();
	if (base_type != StringName()) {
		if (!nobind::ClassDB::get_singleton()->is_parent_class(p_for_object->get_class(), base_type)) {
			ERR_FAIL_V_MSG(nullptr, 
				vformat("Script inherits from '%s', so it can't be assigned to an object of type '%s'", 
					base_type, p_for_object->get_class()));
		}
	}
#endif // TOOLS_ENABLED
	
	LuauEngine::VMType vm_type = LuauEngine::VM_USER;
	
	LuauScriptInstance *instance = memnew(LuauScriptInstance(Ref<LuauScript>(this), p_for_object, vm_type));
	
	// Register the instance with the script
	{
		MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
		const_cast<LuauScript*>(this)->instances[p_for_object->get_instance_id()] = instance;
	}

	String script_name = get_path();
	if (script_name.is_empty()) {
		script_name = definition.name;
	}

	if (LuauLanguage::singleton->luau) {
		// Get VM for this instance
		lua_State* L = LuauLanguage::singleton->luau->get_vm(vm_type);
		if (L) {
			lua_State* thread = lua_newthread(L);
			
			// Store thread reference to prevent GC
			int thread_ref = lua_ref(L, -1);
			
			// Create self table for instance
			lua_newtable(thread);
			
			// Create metatable for the self table
			lua_newtable(thread);
			
			// Store pointer to C++ instance
			lua_pushlightuserdata(thread, instance);
			lua_setfield(thread, -2, "__instance");
			
			// Store pointer to owner object
			lua_pushlightuserdata(thread, p_for_object);
			lua_setfield(thread, -2, "__owner");
			
			// Set up __index to access Godot properties and methods
			lua_pushcfunction(thread, [](lua_State *L) -> int {
				lua_getmetatable(L, 1);
				lua_getfield(L, -1, "__owner");
				Object *owner = (Object*)lua_touserdata(L, -1);
				lua_pop(L, 2);
				
				if (!owner) {
					lua_pushnil(L);
					return 1;
				}
				
				const char *key = lua_tostring(L, 2);
				if (!key) {
					lua_pushnil(L);
					return 1;
				}
				
				StringName prop_name(key);
				
				// Try to get property
				Variant value = owner->get(prop_name);
				if (value.get_type() != Variant::NIL) {
					LuauMarshal::push_variant(L, value);
					return 1;
				}
				
				lua_pushnil(L);
				return 1;
			}, "__index");
			lua_setfield(thread, -2, "__index");
			
			// Set up __newindex to set Godot properties
			lua_pushcfunction(thread, [](lua_State *L) -> int {
				lua_getmetatable(L, 1);
				lua_getfield(L, -1, "__owner");
				Object *owner = (Object*)lua_touserdata(L, -1);
				lua_pop(L, 2);
				
				if (!owner) {
					return 0;
				}
				
				const char *key = lua_tostring(L, 2);
				if (!key) {
					return 0;
				}
				
				StringName prop_name(key);
				Variant value = LuauMarshal::get_variant(L, 3);
				owner->set(prop_name, value);
				
				return 0;
			}, "__newindex");
			lua_setfield(thread, -2, "__newindex");
			
			// Set metatable
			lua_setmetatable(thread, -2);
			
			// Store self table reference on main state, not thread
			// Move self table from thread to main state
			lua_xmove(thread, L, 1);
			int self_ref = lua_ref(L, -1);
			
			// Initialize the instance's Lua state
			instance->initialize_lua_state(L, thread, thread_ref, self_ref);
			
			// Load bytecode if available
			if (bytecode.size() > 0) {
				WARN_PRINT(vformat("Loading bytecode for script: %s (size: %d)", script_name, bytecode.size()));
				// Load the bytecode into the thread
			
				int load_result = luau_load(thread, script_name.utf8().get_data(), 
					(const char*)bytecode.ptr(), bytecode.size(), 0);
				
				if (load_result == 0) {
					// The loaded function is now on the stack
					// Get the self table to use as environment
					lua_getref(L, instance->get_self_ref());
					lua_xmove(L, thread, 1);
					
					// Create a metatable for the environment that redirects global writes to self
					lua_newtable(thread); // Create metatable
					
					// Set __index to look up in self first, then _G
					lua_pushcfunction(thread, [](lua_State *L) -> int {
						// Stack: env_table, key
						const char* key = lua_tostring(L, 2);
						
						// First check in the table itself
						lua_pushvalue(L, 2); // Push key
						lua_rawget(L, 1); // Get from table
						if (!lua_isnil(L, -1)) {
							return 1; // Found in table
						}
						lua_pop(L, 1); // Remove nil
						
						// Then check in global environment
						lua_getglobal(L, key);
						return 1;
					}, "__index");
					lua_setfield(thread, -2, "__index");
					
					// Set __newindex to write to self
					lua_pushcfunction(thread, [](lua_State *L) -> int {
						// Stack: env_table, key, value
						// Write directly to the table
						lua_pushvalue(L, 2); // Push key
						lua_pushvalue(L, 3); // Push value
						lua_rawset(L, 1); // Set in table
						return 0;
					}, "__newindex");
					lua_setfield(thread, -2, "__newindex");
					
					// Set the metatable on self
					lua_setmetatable(thread, -2);
					
					// Set self table as the environment for the loaded function
					lua_setfenv(thread, -2);
					
					// Execute the script with no arguments
					int call_result = lua_pcall(thread, 0, 0, 0);
					
					if (call_result != 0) {
						WARN_PRINT(vformat("Script execution failed for: %s", script_name));
						// Get error message
						const char* error_msg = lua_tostring(thread, -1);
						if (error_msg) {
							String error_str = String(error_msg);
							// Check for common error patterns and provide more helpful messages
							if (error_str.contains("attempt to call a nil value")) {
								// Extract the line number
								PackedStringArray parts = error_str.split(":");
								if (parts.size() >= 3) {
									String line_num = parts[2];
									ERR_PRINT(vformat(
										"Script error in %s at line %s: Function calls are not allowed in the class body. "
										"Only function definitions and property assignments are allowed at the class level. "
										"Move function calls like 'print()' inside a method like _ready() or _init().",
										script_name, line_num));
								} else {
									ERR_PRINT(vformat(
										"Script error in %s: Function calls are not allowed in the class body. "
										"Move function calls like 'print()' inside a method.",
										script_name));
								}
							} else if (error_str.contains("attempt to index a nil value")) {
								PackedStringArray parts = error_str.split(":");
								if (parts.size() >= 3) {
									String line_num = parts[2];
									ERR_PRINT(vformat(
										"Script error in %s at line %s: Attempting to access a property or method on a nil value. "
										"Global objects are not available in the class body context.",
										script_name, line_num));
								} else {
									ERR_PRINT(error_msg);
								}
							} else {
								// For other errors, just pass through the original message
								ERR_PRINT(error_msg);
							}
						} else {
							ERR_PRINT(vformat("Failed to execute Luau script %s: unknown error", script_name));
						}
					lua_pop(thread, 1); // Remove error message
#ifdef TOOLS_ENABLED
						// In the editor, clean up and create a placeholder instance instead
						// Clean up the failed instance
						if (L && thread_ref != LUA_NOREF) {
							lua_unref(L, thread_ref);
						}
						if (L && self_ref != LUA_NOREF) {
							lua_unref(L, self_ref);
						}
						
						// Remove the failed instance from the instances map
						{
							MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
							const_cast<LuauScript*>(this)->instances.erase(p_for_object->get_instance_id());
						}
						
						// Delete the failed instance
						memdelete(instance);
						
						// Enable placeholder fallback mode
						const_cast<LuauScript*>(this)->placeholder_fallback_enabled = true;
						
						// Create and return a placeholder instance
						return _placeholder_instance_create(p_for_object);
#endif // TOOLS_ENABLED
					} else {
						WARN_PRINT(vformat("Script executed successfully for: %s", script_name));
						// Script has been executed and populated the self table
						// The functions are now defined in the self table (which was the environment)
						
						// Debug: First let's check what's in the environment after script execution
						WARN_PRINT("Checking environment contents after script execution:");
						
						// Get the self table from main state
						lua_getref(L, instance->get_self_ref());
						lua_xmove(L, thread, 1);
						
						// Iterate through the self table to see what was added
						lua_pushnil(thread);
						int func_count = 0;
						while (lua_next(thread, -2) != 0) {
							int type = lua_type(thread, -1);
							if (lua_type(thread, -2) == LUA_TSTRING) {
								const char* key = lua_tostring(thread, -2);
								if (key) {
									const char* type_name = lua_typename(thread, type);
									WARN_PRINT(vformat("  Self['%s'] = %s", key, type_name));
									if (type == LUA_TFUNCTION) {
										func_count++;
									}
								}
							}
							lua_pop(thread, 1); // Remove value, keep key for next iteration
						}
						WARN_PRINT(vformat("Total functions found in self table: %d", func_count));
						
                        // Now try to call _init if it exists
                        // Stack currently has: self_table
                        lua_pushvalue(thread, -1); // Duplicate self table for later
                        // Stack: self_table, self_table_copy
                        lua_getfield(thread, -2, "_init"); // Get _init from original self table
                        // Stack: self_table, self_table_copy, _init_function (or nil)
                        int init_type = lua_type(thread, -1);
                        
                        if (lua_isfunction(thread, -1)) {
                            WARN_PRINT("Found _init function, calling it");
                            // Stack: self_table, self_table_copy, _init_function
                            // Swap the function and the copy of self
                            lua_insert(thread, -2); // Move function below self_copy
                            // Stack: self_table, _init_function, self_table_copy
                            // Now call with self_table_copy as first argument
                            int init_result = lua_pcall(thread, 1, 0, 0); // 1 argument (self)
							
							if (init_result != 0) {
								const char* error_msg = lua_tostring(thread, -1);
								ERR_PRINT(vformat("Failed to call _init for %s: %s", 
									script_name, error_msg ? error_msg : "unknown error"));
								lua_pop(thread, 1); // Remove error message
							} else {
								WARN_PRINT("_init called successfully");
							}
						} else {
							// _init is not a function or doesn't exist
							lua_pop(thread, 1); // Remove non-function value
							WARN_PRINT(vformat("_init not found as function in self table (type: %s)", lua_typename(thread, init_type)));
						}
						
						lua_pop(thread, 1); // Remove self table
					}
				}
			}
		}
	}
	
	// Create and return the GDExtension script instance
	return internal::gdextension_interface_script_instance_create3(&LuauScriptInstance::INSTANCE_INFO, instance);
}

void *LuauScript::_placeholder_instance_create(Object *p_for_object) const {
    #ifdef TOOLS_ENABLED
	    PlaceHolderScriptInstance *internal = memnew(PlaceHolderScriptInstance(Ref<LuauScript>(this), p_for_object));
	    return internal::gdextension_interface_script_instance_create3(&PlaceHolderScriptInstance::INSTANCE_INFO, internal);
    #else
        return nullptr;
    #endif // TOOLS_ENABLED
}

bool LuauScript::instance_has(uint64_t p_obj_id) const {
	MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
	return instances.has(p_obj_id);
}

bool LuauScript::_instance_has(Object *p_object) const {
	return instance_has(p_object->get_instance_id());
}

bool LuauScript::_editor_can_reload_from_file() {
	return true;
}

void LuauScript::_placeholder_erased(void *p_placeholder) {
#ifdef TOOLS_ENABLED
	placeholders.erase(((PlaceHolderScriptInstance *)p_placeholder)->get_owner()->get_instance_id());
#endif // TOOLS_ENABLED
}

bool LuauScript::_can_instantiate() const {
    return true;
}

Ref<Script> LuauScript::_get_base_script() const {
    return base;
}

StringName LuauScript::_get_global_name() const {
	return definition.name;
}

bool LuauScript::_inherits_script(const Ref<Script> &p_script) const {
	Ref<LuauScript> script = p_script;
	if (script.is_null())
		return false;

	const LuauScript *s = this;

	while (s) {
		if (s == script.ptr())
			return true;

		s = s->base.ptr();
	}

	return false;
}




//MARK: LuauLanguage
LuauLanguage *LuauLanguage::singleton = nullptr;

#ifdef TOOLS_ENABLED
List<Ref<LuauScript>> LuauLanguage::get_scripts() const {
    List<Ref<LuauScript>> scripts;
    
	{
		MutexLock lock(*this->mutex.ptr());

		const SelfList<LuauScript> *item = script_list.first();

		while (item) {
			String path = item->self()->get_path();

			if (ResourceFormatLoaderLuau::get_resource_type(path) == luau::LUAUSCRIPT_TYPE) {
				scripts.push_back(Ref<LuauScript>(item->self()));
			}

			item = item->next();
		}
	}

	return scripts;
}
#endif //TOOLS_ENABLED

void LuauLanguage::_init() {
    luau = memnew(LuauEngine);
    cache = memnew(LuauCache);

}

void LuauLanguage::_finish() {
    if (luau) {
        memdelete(luau);
    }
    if (cache) {
        memdelete(cache);    
    }
}

PackedStringArray LuauLanguage::_get_reserved_words() const {
    static const char *_reserved_words[] = {
		"and",
		"break",
		"do",
		"else",
		"elseif",
		"end",
		"false",
		"for",
		"function",
		"if",
		"in",
		"local",
		"nil",
		"not",
		"or",
		"repeat",
		"return",
		"then",
		"true",
		"until",
		"while",
		"continue",
		nullptr
	};

	PackedStringArray keywords;
	const char **w = _reserved_words;

	while (*w) {
		keywords.push_back(*w);
		w++;
	}

	return keywords;
}

bool LuauLanguage::_is_control_flow_keyword(const String &p_keyword) const {
    return p_keyword == "break" 
		|| p_keyword == "else"
		|| p_keyword == "elseif"
		|| p_keyword == "for"
		|| p_keyword == "if"
		|| p_keyword == "repeat" 
		|| p_keyword == "return"
		|| p_keyword == "until"
		|| p_keyword == "while";
}

PackedStringArray LuauLanguage::_get_comment_delimiters() const {
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
	delimiters.push_back("` `");

	return delimiters;
}

bool LuauLanguage::_is_using_templates() {
	return true;
}

Ref<Script> LuauLanguage::_make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const {
#ifdef TOOLS_ENABLED
	Ref<LuauScript> scr;
	scr.instantiate();

	Ref<EditorSettings> settings = nobind::EditorInterface::get_singleton()->get_editor_settings();
	bool indent_spaces = settings->get_setting("text_editor/behavior/indent/type");
	int indent_size = settings->get_setting("text_editor/behavior/indent/size");
	String indent = indent_spaces ? String(" ").repeat(indent_size) : "\t";

	String contents = p_template.replace("_CLASS_NAME_", p_class_name)
								.replace("_BASE_CLASS_", p_base_class_name)
								.replace("_I_", indent);

	scr->_set_source_code(contents);

	return scr;
#else
	return Ref<Script>();
#endif
}

TypedArray<Dictionary> LuauLanguage::_get_built_in_templates(const StringName &p_object) const {
#ifdef TOOLS_ENABLED

	TypedArray<Dictionary> templates;

	if (p_object == StringName("Object")) {
		Dictionary t;
		t["inherit"] = "Object";
		t["name"] = "Default";
		t["description"] = "Default template for Objects";
		t["content"] = R"TEMPLATE(--- @class
--- @extends _BASE_CLASS_
local _CLASS_NAME_ = {}
local _CLASS_NAME_C = gdclass(_CLASS_NAME_)

export type _CLASS_NAME_ = _BASE_CLASS_ & typeof(_CLASS_NAME_) & {
_I_-- Put properties, signals, and non-registered table fields here
}

return _CLASS_NAME_C
)TEMPLATE";

		t["id"] = 0;
		t["origin"] = 0; // TEMPLATE_BUILT_IN

		templates.push_back(t);
	}

	if (p_object == StringName("Node")) {
		Dictionary t;
		t["inherit"] = "Node";
		t["name"] = "Default";
		t["description"] = "Default template for Nodes with _Ready and _Process callbacks";
		t["content"] = R"TEMPLATE(--- @class
--- @extends _BASE_CLASS_
local _CLASS_NAME_ = {}
local _CLASS_NAME_C = gdclass(_CLASS_NAME_)

export type _CLASS_NAME_ = _BASE_CLASS_ & typeof(_CLASS_NAME_) & {
_I_-- Put properties, signals, and non-registered table fields here
}

--- @registerMethod
function _CLASS_NAME_._Ready(self: _CLASS_NAME_)
_I_-- Called when the node enters the scene tree
end

--- @registerMethod
function _CLASS_NAME_._Process(self: _CLASS_NAME_, delta: number)
_I_-- Called every frame
end

return _CLASS_NAME_C
)TEMPLATE";

		t["id"] = 0;
		t["origin"] = 0; // TEMPLATE_BUILT_IN

		templates.push_back(t);
	}

	return templates;
#else
	return TypedArray<Dictionary>();
#endif
}
		
void LuauLanguage::_reload_all_scripts() {
#ifdef TOOLS_ENABLED
	List<Ref<LuauScript>> scripts = get_scripts();
	
	// First, clear all cached bytecode to force recompilation
	for (Ref<LuauScript> &script : scripts) {
		script->load_stage = LuauScript::LOAD_NONE;
		script->bytecode.clear();
	}

	// Then reload all scripts
	for (Ref<LuauScript> &script : scripts) {
		String path = script->get_path();
		if (!path.is_empty()) {
			script->load_source_code(path);
		}
		script->_reload(true);
	}
#endif // TOOLS_ENABLED
}

void LuauLanguage::_reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) {
#ifdef TOOLS_ENABLED
	Ref<LuauScript> script = p_script;
	if (script.is_null()) {
		return;
	}
	
	// Clear cached bytecode to force recompilation
	script->load_stage = LuauScript::LOAD_NONE;
	script->bytecode.clear();
	
	// Reload source from file
	String path = script->get_path();
	if (!path.is_empty()) {
		script->load_source_code(path);
	}
	
	// Reload the script
	script->_reload(p_soft_reload);
#endif
}


PackedStringArray LuauLanguage::_get_recognized_extensions() const {
    PackedStringArray extension;
    extension.push_back(luau::LUAUSCRIPT_EXTENSION);
    return extension;
}

TypedArray<Dictionary> LuauLanguage::_get_public_functions() const {
    return TypedArray<Dictionary>();
}

Dictionary LuauLanguage::_get_public_constants() const {
    return Dictionary();
}

TypedArray<Dictionary> LuauLanguage::_get_public_annotations() const {
    return TypedArray<Dictionary>();
}

Dictionary LuauLanguage::_validate(
	const String &p_script, 
	const String &p_path, 
	bool p_validate_functions, 
	bool p_validate_errors, 
	bool p_validate_warnings, 
	bool p_validate_safe_lines
) const {
	Dictionary ret;

	ret["valid"] = true;

	return ret;
}

String LuauLanguage::_validate_path(const String &p_path) const {
	return "";
}

Object *LuauLanguage::_create_script() const {
    return memnew(LuauScript);
}

bool LuauLanguage::_supports_documentation() const {
    return false;
}

bool LuauLanguage::_can_inherit_from_file() const {
	return true;
}

int32_t LuauLanguage::_find_function(const String &p_function, const String &p_code) const {
	return -1;
}

String LuauLanguage::_make_function(const String &p_class_name, const String &p_function_name, const PackedStringArray &p_function_args) const {
	return String();
}

bool LuauLanguage::_supports_builtin_mode() const {
	return false;
}

bool LuauLanguage::_overrides_external_editor() {
    return false;
}

Dictionary LuauLanguage::_complete_code(const String &p_code, const String &p_path, Object *p_owner) const {
	return Dictionary();
}

Dictionary LuauLanguage::_lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner) const {
	Dictionary ret;
	
	// Default to no result
	ret["result"] = 0; // LOOKUP_RESULT_SCRIPT_LOCATION = 0, LOOKUP_RESULT_CLASS = 1, LOOKUP_RESULT_CLASS_CONSTANT = 2, etc.
	
	// Try to find information about the symbol
	String symbol = p_symbol.strip_edges();
	
	// Check if it's a known Godot class
	if (nobind::ClassDB::get_singleton()->class_exists(symbol)) {
		ret["result"] = 1; // LOOKUP_RESULT_CLASS
		ret["type"] = 0; // TYPE_CLASS
		ret["class_name"] = symbol;
		ret["class_path"] = String(); // Built-in class, no path
		
		// Get class documentation if available
		String class_doc = String("Godot built-in class: ") + symbol;
		ret["description"] = class_doc;
		ret["is_deprecated"] = false;
		
		return ret;
	}
	
	// Check if it's a method of a known class (format: ClassName.method_name)
	if (symbol.contains(".")) {
		PackedStringArray parts = symbol.split(".");
		if (parts.size() == 2) {
			String class_name = parts[0];
			String member_name = parts[1];
			
				if (nobind::ClassDB::get_singleton()->class_exists(class_name)) {
					// Check if it's a method
					if (nobind::ClassDB::get_singleton()->class_has_method(class_name, member_name, false)) {
						ret["result"] = 3; // LOOKUP_RESULT_CLASS_METHOD
						ret["type"] = 2; // TYPE_FUNCTION
						ret["class_name"] = class_name;
						ret["class_member"] = member_name;
						ret["description"] = String("Method of class ") + class_name;
						ret["is_deprecated"] = false;
						
						return ret;
					}
					
					// Check if it's a property
					TypedArray<Dictionary> property_list = nobind::ClassDB::get_singleton()->class_get_property_list(class_name, false);
					for (int i = 0; i < property_list.size(); i++) {
						Dictionary prop = property_list[i];
						if (prop.has("name") && String(prop["name"]) == member_name) {
							ret["result"] = 5; // LOOKUP_RESULT_CLASS_PROPERTY
							ret["type"] = 1; // TYPE_MEMBER
							ret["class_name"] = class_name;
							ret["class_member"] = member_name;
							ret["description"] = String("Property of class ") + class_name;
							ret["is_deprecated"] = false;
							
							return ret;
						}
					}
					
					// Check if it's a constant
					if (nobind::ClassDB::get_singleton()->class_has_integer_constant(class_name, member_name)) {
						ret["result"] = 2; // LOOKUP_RESULT_CLASS_CONSTANT
						ret["type"] = 3; // TYPE_CONSTANT
						ret["class_name"] = class_name;
						ret["class_member"] = member_name;
						ret["description"] = String("Constant of class ") + class_name;
						ret["is_deprecated"] = false;
						
						return ret;
					}
					
					// Check if it's an enum
					if (nobind::ClassDB::get_singleton()->class_has_enum(class_name, member_name, false)) {
						ret["result"] = 4; // LOOKUP_RESULT_CLASS_ENUM
						ret["type"] = 4; // TYPE_ENUM
						ret["class_name"] = class_name;
						ret["class_member"] = member_name;
						ret["description"] = String("Enum of class ") + class_name;
						ret["is_deprecated"] = false;
						
						return ret;
					}
			}
		}
	}
	
	// Check if it's a Luau script in the project
	if (p_owner != nullptr) {
		// Try to get the script from the owner
		Ref<Script> script = p_owner->get_script();
		if (script.is_valid()) {
			Ref<LuauScript> luau_script = script;
			if (luau_script.is_valid()) {
				// Check if the symbol is a method in this script
				if (luau_script->definition.methods.has(symbol)) {
					const GDMethod &method = luau_script->definition.methods[symbol];
					
					ret["result"] = 0; // LOOKUP_RESULT_SCRIPT_LOCATION
					ret["type"] = 2; // TYPE_FUNCTION
					ret["class_name"] = luau_script->definition.name;
					ret["class_member"] = symbol;
					ret["class_path"] = luau_script->get_path();
					ret["location"] = 0; // Would need to parse to find actual line number
					ret["description"] = String("Method in script: ") + symbol;
					ret["is_deprecated"] = false;
					
					return ret;
				}
				
				// Check if the symbol is a constant in this script
				if (luau_script->constants.has(symbol)) {
					Variant value = luau_script->constants[symbol];
					
					ret["result"] = 0; // LOOKUP_RESULT_SCRIPT_LOCATION
					ret["type"] = 3; // TYPE_CONSTANT
					ret["class_name"] = luau_script->definition.name;
					ret["class_member"] = symbol;
					ret["class_path"] = luau_script->get_path();
					ret["location"] = 0; // Would need to parse to find actual line number
					ret["description"] = String("Constant in script: ") + symbol + " = " + String(value);
					ret["is_deprecated"] = false;
					
					return ret;
				}
				
				// Check base scripts recursively
				LuauScript *base = luau_script->base.ptr();
				while (base) {
					if (base->definition.methods.has(symbol)) {
						const GDMethod &method = base->definition.methods[symbol];
						
						ret["result"] = 0; // LOOKUP_RESULT_SCRIPT_LOCATION
						ret["type"] = 2; // TYPE_FUNCTION
						ret["class_name"] = base->definition.name;
						ret["class_member"] = symbol;
						ret["class_path"] = base->get_path();
						ret["location"] = 0;
						ret["description"] = String("Inherited method from: ") + base->definition.name;
						ret["is_deprecated"] = false;
						
						return ret;
					}
					
					if (base->constants.has(symbol)) {
						Variant value = base->constants[symbol];
						
						ret["result"] = 0; // LOOKUP_RESULT_SCRIPT_LOCATION
						ret["type"] = 3; // TYPE_CONSTANT
						ret["class_name"] = base->definition.name;
						ret["class_member"] = symbol;
						ret["class_path"] = base->get_path();
						ret["location"] = 0;
						ret["description"] = String("Inherited constant from: ") + base->definition.name + " = " + String(value);
						ret["is_deprecated"] = false;
						
						return ret;
					}
					
					base = base->base.ptr();
				}
			}
		}
	}
	
	// Check global constants registered with the language
	if (global_constants.has(symbol)) {
		Variant value = global_constants[symbol];
		
		ret["result"] = 2; // LOOKUP_RESULT_CLASS_CONSTANT
		ret["type"] = 3; // TYPE_CONSTANT
		ret["class_name"] = "@GlobalScope";
		ret["class_member"] = symbol;
		ret["description"] = String("Global constant: ") + symbol + " = " + String(value);
		ret["is_deprecated"] = false;
		
		return ret;
	}
	
	// Check if it's a Luau keyword
	PackedStringArray keywords = _get_reserved_words();
	if (keywords.has(symbol)) {
		ret["result"] = 7; // LOOKUP_RESULT_CLASS_ANNOTATION (using for keywords)
		ret["type"] = 5; // TYPE_SIGNAL (repurposing for keyword)
		ret["class_name"] = "Luau";
		ret["class_member"] = symbol;
		ret["description"] = String("Luau keyword: ") + symbol;
		ret["is_deprecated"] = false;
		
		return ret;
	}
	
	// No information found
	ret["result"] = 0;
	ret["type"] = 0;
	ret["description"] = String("Unknown symbol: ") + symbol;
	ret["is_deprecated"] = false;
	
	return ret;
}

String LuauLanguage::_auto_indent_code(const String &p_code, int32_t p_from_line, int32_t p_to_line) const {
	return String();
}


void LuauLanguage::_add_global_constant(const StringName &p_name, const Variant &p_value) {
	_add_named_global_constant(p_name, p_value);
}

void LuauLanguage::_add_named_global_constant(const StringName &p_name, const Variant &p_value) {
    global_constants[p_name] = p_value;
}

void LuauLanguage::_remove_named_global_constant(const StringName &p_name) {
	global_constants.erase(p_name);
}


String LuauLanguage::_debug_get_error() const {
#ifdef TOOLS_ENABLED
	return debug.error;
#else
	return "";
#endif // TOOLS_ENABLED
}

TypedArray<Dictionary> LuauLanguage::_debug_get_current_stack_info() const {
	TypedArray<Dictionary> stack_info;

#ifdef TOOLS_ENABLED
	if (debug.call_lock.is_valid()) {
		MutexLock lock(*debug.call_lock.ptr());
		
		for (const auto &si : debug.call_stack) {
			stack_info.append(static_cast<Dictionary>(si));
		}
		
		for (const auto &bsi : debug.break_call_stack) {
			Dictionary entry;
			entry["file"] = bsi.source ? String(bsi.source) : String();
			entry["func"] = bsi.name ? String(bsi.name) : String();
			entry["line"] = bsi.line;
			
			if (!bsi.members.is_empty()) {
				Dictionary members_dict;
				for (const auto &pair : bsi.members) {
					members_dict[pair.key] = pair.value;
				}
				entry["members"] = members_dict;
			}
			
			if (!bsi.locals.is_empty()) {
				Dictionary locals_dict;
				for (const auto &pair : bsi.locals) {
					locals_dict[pair.key] = pair.value;
				}
				entry["locals"] = locals_dict;
			}
			
			stack_info.append(entry);
		}
	}
#endif // TOOLS_ENABLED

	return stack_info;
}

#ifdef TOOLS_ENABLED
LuauLanguage::DebugInfo::StackInfo::operator Dictionary() const {
	Dictionary dict;
	dict["file"] = source ? String(source) : String();
	dict["func"] = name ? String(name) : String();
	dict["line"] = line;
	return dict;
}
#endif // TOOLS_ENABLED

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

LuauLanguage::LuauLanguage() {
	singleton = this;
	mutex.instantiate();
	
#ifdef TOOLS_ENABLED
	debug.call_lock.instantiate();
#endif // TOOLS_ENABLED
}

LuauLanguage::~LuauLanguage() {
	singleton = nullptr;
}
