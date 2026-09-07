#include "luau_script.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
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
#include "luau_bridge.h"
#include "luauscript_resource_format.h"
#include "variant/builtin_types.h"
#include "lamda_wrapper.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include "luauscript/luau_await.h"
#include "luauscript/luau_reentry_guard.h"

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
	
	// New fields in GDExtensionScriptInstanceInfo3
	p_info.get_class_category_func = nullptr;
	p_info.validate_property_func = nullptr;
	p_info.get_method_argument_count_func = nullptr;

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

//MARK: AST Helper Functions
Variant::Type LuauScript::parse_type_name(const String& type_name) {
	if (type_name == "boolean") return Variant::BOOL;
	if (type_name == "number") return Variant::FLOAT;
	if (type_name == "string") return Variant::STRING;
	if (type_name == "Vector2") return Variant::VECTOR2;
	if (type_name == "Vector3") return Variant::VECTOR3;
	if (type_name == "Color") return Variant::COLOR;
	if (type_name == "Rect2") return Variant::RECT2;
	if (type_name == "Transform2D") return Variant::TRANSFORM2D;
	if (type_name == "Plane") return Variant::PLANE;
	if (type_name == "Quaternion") return Variant::QUATERNION;
	if (type_name == "AABB") return Variant::AABB;
	if (type_name == "Basis") return Variant::BASIS;
	if (type_name == "Transform3D") return Variant::TRANSFORM3D;
	if (type_name == "Projection") return Variant::PROJECTION;
	if (type_name == "StringName") return Variant::STRING_NAME;
	if (type_name == "NodePath") return Variant::NODE_PATH;
	if (type_name == "RID") return Variant::RID;
	if (type_name == "Dictionary") return Variant::DICTIONARY;
	if (type_name == "Array") return Variant::ARRAY;
	if (type_name == "PackedByteArray") return Variant::PACKED_BYTE_ARRAY;
	if (type_name == "PackedInt32Array") return Variant::PACKED_INT32_ARRAY;
	if (type_name == "PackedInt64Array") return Variant::PACKED_INT64_ARRAY;
	if (type_name == "PackedFloat32Array") return Variant::PACKED_FLOAT32_ARRAY;
	if (type_name == "PackedFloat64Array") return Variant::PACKED_FLOAT64_ARRAY;
	if (type_name == "PackedStringArray") return Variant::PACKED_STRING_ARRAY;
	if (type_name == "PackedVector2Array") return Variant::PACKED_VECTOR2_ARRAY;
	if (type_name == "PackedVector3Array") return Variant::PACKED_VECTOR3_ARRAY;
	if (type_name == "PackedColorArray") return Variant::PACKED_COLOR_ARRAY;
	return Variant::OBJECT;
}

AstExprResult LuauScript::extract_ast_expr_value(
	Luau::AstExpr* expr,
	Luau::AstType* annotation,
	int recursion_depth
) {
	AstExprResult result;
	
	if (recursion_depth > 10) {
		return result;
	}
	
	if (!expr) {
		return result;
	}
	
	if (auto* num = expr->as<Luau::AstExprConstantNumber>()) {
		result.value = num->value;
		
		double int_part;
		if (modf(num->value, &int_part) == 0.0) {
			result.value = (int64_t)num->value;
			result.type = Variant::INT;
		} else {
			result.type = Variant::FLOAT;
		}
		result.success = true;
		return result;
	}
	
	if (auto* str = expr->as<Luau::AstExprConstantString>()) {
		result.value = String::utf8(str->value.data, str->value.size);
		result.type = Variant::STRING;
		result.success = true;
		return result;
	}
	
	if (auto* bool_val = expr->as<Luau::AstExprConstantBool>()) {
		result.value = bool_val->value;
		result.type = Variant::BOOL;
		result.success = true;
		return result;
	}
	
	if (auto* table = expr->as<Luau::AstExprTable>()) {
		if (annotation) {
			if (auto* table_type = annotation->as<Luau::AstTypeTable>()) {
				// Typed array with indexer
				if (table_type->indexer) {
					if (auto* var_type_ref = table_type->indexer->resultType->as<Luau::AstTypeReference>()) {
						
						result.native_type = StringName(var_type_ref->name.value);
						
						result.type = Variant::ARRAY;
						Array typed_arr;

						Variant::Type typed_var = parse_type_name(result.native_type);
						if (typed_var == Variant::OBJECT) {
							typed_arr.set_typed(Variant::OBJECT, result.native_type, Variant());
						} else {
							typed_arr.set_typed(typed_var, StringName(), Variant());
						}

						// Recursively extract table elements
						for (size_t i = 0; i < table->items.size; i++) {
							const Luau::AstExprTable::Item& item = table->items.data[i];
							if (item.value) {
								AstExprResult elem_result = extract_ast_expr_value(item.value, table_type->indexer->resultType, recursion_depth + 1);
								if (elem_result.success) {
									typed_arr.push_back(elem_result.value);
								}
							}
						}

						result.value = typed_arr;
						result.success = true;
						return result;
					}

				} else if (table_type->props.size > 0) {
					// Dictionary
					Dictionary dict;
					
					// Recursively extract dictionary values
					for (size_t i = 0; i < table->items.size; i++) {
						const Luau::AstExprTable::Item& item = table->items.data[i];
						if (item.key && item.value) {
							// Extract key
							String key_str;
							if (auto* key_str_expr = item.key->as<Luau::AstExprConstantString>()) {
								key_str = String::utf8(key_str_expr->value.data, key_str_expr->value.size);
							} else if (auto* key_global = item.key->as<Luau::AstExprGlobal>()) {
								key_str = String(key_global->name.value);
							}
							
							if (!key_str.is_empty()) {
								AstExprResult val_result = extract_ast_expr_value(item.value, nullptr, recursion_depth + 1);
								if (val_result.success) {
									dict[key_str] = val_result.value;
								}
							}
						}
					}
					
					result.value = dict;
					result.type = Variant::DICTIONARY;
					result.success = true;
					return result;

				}
			}
		}
		
		// Untyped table - extract as array or dictionary based on keys
		Array untyped_arr;
		Dictionary untyped_dict;
		bool has_string_keys = false;
		
		for (size_t i = 0; i < table->items.size; i++) {
			const Luau::AstExprTable::Item& item = table->items.data[i];
			if (item.value) {
				AstExprResult elem_result = extract_ast_expr_value(item.value, nullptr, recursion_depth + 1);
				
				if (item.key) {
					// Has key - treat as dictionary
					has_string_keys = true;
					String key_str;
					if (auto* key_str_expr = item.key->as<Luau::AstExprConstantString>()) {
						key_str = String::utf8(key_str_expr->value.data, key_str_expr->value.size);
					} else if (auto* key_global = item.key->as<Luau::AstExprGlobal>()) {
						key_str = String(key_global->name.value);
					}
					
					if (!key_str.is_empty() && elem_result.success) {
						untyped_dict[key_str] = elem_result.value;
					}
				} else {
					// No key - treat as array element
					if (elem_result.success) {
						untyped_arr.push_back(elem_result.value);
					}
				}
			}
		}
		
		if (has_string_keys && untyped_dict.size() > 0) {
			result.value = untyped_dict;
			result.type = Variant::DICTIONARY;
		} else {
			result.value = untyped_arr;
			result.type = Variant::ARRAY;
		}
		result.success = true;
		return result;
	}
	
	if (expr->as<Luau::AstExprConstantNil>()) {
		result.value = Variant();
		
		if (annotation) {
			if (auto* anno_type = annotation->as<Luau::AstTypeReference>()) {
				result.native_type = StringName(anno_type->name.value);
				result.type = parse_type_name(result.native_type);
				result.success = true;
				return result;
			}
		}
		
		result.success = false;
		return result;
	}
	
	return result;
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
	memdelete((StringName *)p_prop.name);
	memdelete((StringName *)p_prop.class_name);
	memdelete((String *)p_prop.hint_string);
}

void ScriptInstance::get_property_state(GDExtensionScriptInstancePropertyStateAdd p_add_func, void *p_userdata) {
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

    return false;
}

bool LuauScriptInstance::property_get_revert(const StringName &p_name, Variant *r_ret) {
    #define PROPERTY_GET_REVERT_NAME "_PropertyGetRevert"

    const LuauScript *s = script.ptr();

    return false;
}

//MARK: engine script invokation
void LuauScriptInstance::call(
    const StringName &p_method,
    const Variant *const *p_args,
	const GDExtensionInt p_argument_count,
    Variant *r_return,
	GDExtensionCallError *r_error
) {
    if (!L || !T || self_ref == LUA_NOREF) {
        r_error->error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
        return;
    }

	if (!script->definition.is_tool && Engine::get_singleton()->is_editor_hint()) {
		r_error->error = GDEXTENSION_CALL_OK;
		return;
	}

	//reentry gaurd
    if (LuauReentryGuard::would_exceed_limit()) {
        String overflow_msg = vformat(
            "Luau: script call recursion limit exceeded (%d) while calling '%s' on %s",
            LUAU_MAX_REENTRY_DEPTH, String(p_method),
            owner ? String(owner->get_class()) : String("<null>"));
        LuauReentryGuard::raise_overflow(overflow_msg);
        r_error->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        return;
    }
    LuauReentryGuard script_call_guard;
    

    const LuauScript *s = script.ptr();
    
	if (p_method == StringName("_ready") && !is_ready) {
		is_ready = true;
		for (int a=0; a < on_ready_funcs.size(); a++) {
			godot::Callable c = on_ready_funcs[a];
			if (!c.is_valid()) {
				continue;
			} 
			c.call();
		}
		on_ready_funcs.clear();
		on_ready_wrappers.clear();
	}

    // Look for the method in the script hierarchy
    while (s) {
        if (s->definition.methods.has(p_method)) {
            const GDMethod &method = s->definition.methods[p_method];
            
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
            
            if (!lua_checkstack(T, 1)) {
                UtilityFunctions::push_error(vformat(
                    "Luau: out of thread stack while calling '%s'", String(p_method)));
                r_error->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
                return;
            }

            lua_State *ET = lua_newthread(T);
            
            for (int i = 0; i < p_argument_count; i++) {
                const Variant &arg = *p_args[i];
                LuauBridge::push_variant(ET, arg);
            }
            
            // Add default arguments
            for (int i = p_argument_count; i < args_allowed; i++) {
                int default_idx = i - (args_allowed - args_default);
                if (default_idx >= 0 && default_idx < args_default) {
                    LuauBridge::push_variant(ET, method.default_arguments[default_idx]);
                }
            }
            
            r_error->error = GDEXTENSION_CALL_OK;
            int status = call_internal(p_method, ET, args_allowed, 1);
            
            if (status == LUA_OK) {
                lua_settop(ET, 1); // call_internal requests exactly one result
                *r_return = LuauBridge::get_variant(ET, -1);

            } else if (status == LUA_YIELD) {
                *r_return = Variant();
                r_error->error = GDEXTENSION_CALL_OK;

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

	if (!script->definition.is_tool && Engine::get_singleton()->is_editor_hint()) {
		return;
	}


	if (LuauReentryGuard::would_exceed_limit()) {
		String overflow_msg = vformat(
			"Luau: script notification recursion limit exceeded (%d) in %s",
			LUAU_MAX_REENTRY_DEPTH,
			owner ? String(owner->get_class()) : String("<null>"));
		LuauReentryGuard::raise_overflow(overflow_msg);

		return;
	}
	LuauReentryGuard notification_guard;


	if (p_what > 10000) return;
	if (!lua_checkstack(T, 1)) {
		UtilityFunctions::push_error("Luau: out of thread stack while delivering notification");
		return;
	}


	lua_State *ET = lua_newthread(T);
	
	lua_getref(L, self_ref);
	lua_xmove(L, ET, 1);
	
	lua_rawgetfield(ET, -1, "_notification");
	
	if (!lua_isfunction(ET, -1)) {
		lua_pop(ET, 2);
		lua_pop(T, 1);
		return;
	}
	
	lua_insert(ET, -2);
	lua_pushinteger(ET, p_what); //notificaiton code
	
	// Call: function, self, notification_code
	int reentry_depth_before = LuauReentryGuard::depth();
	int call_result = lua_pcall(ET, 2, 0, 0); // 1 for self + 1 for notification code
	LuauReentryGuard::restore(reentry_depth_before);
	
	if (call_result != LUA_OK) {
		const char* error_msg = lua_tostring(ET, -1);
		if (error_msg) {
			UtilityFunctions::printerr(vformat("Luau script error in _notification: %s", error_msg));
		}
		lua_pop(ET, 1); // Remove error message
	}
	
	lua_pop(T, 1); // Remove thread
}


void LuauScriptInstance::to_string(GDExtensionBool *r_is_valid, String *r_out) {
#define TO_STRING_NAME "_ToString"
}


bool LuauScriptInstance::set(const StringName &p_name, const Variant &p_value, PropertySetGetError *r_err) {
	if (!L || self_ref == LUA_NOREF) {
		if (r_err) *r_err = PROP_NOT_FOUND;
		return false;
	}


	RefCounted *new_rc = Object::cast_to<RefCounted>(p_value.get_validated_object());
	auto prev = held_member_refs.find(p_name);
	if (prev != held_member_refs.end()) {
		if (prev->value != nullptr) {
			prev->value->unreference();
		}
		held_member_refs.erase(p_name);
	}
	if (new_rc != nullptr) {
		new_rc->reference();
		held_member_refs[p_name] = new_rc;
	}

	
	lua_getref(L, self_ref);
	
	String prop_str = String(p_name);
	LuauBridge::push_variant(L, p_value);
	lua_setfield(L, -2, prop_str.utf8().get_data());
	
	lua_pop(L, 1); // Remove self table
	
	if (r_err) *r_err = PROP_OK;
	return true;
}


bool LuauScriptInstance::get(const StringName &p_name, Variant &r_ret, PropertySetGetError *r_err) {
	bool is_function = false;
	return get_with_kind(p_name, r_ret, r_err, is_function);
}

bool LuauScriptInstance::get_with_kind(const StringName &p_name, Variant &r_ret, PropertySetGetError *r_err, bool &r_is_function) {
	Object* object = get_owner();

	r_is_function = false;

	if (!L || self_ref == LUA_NOREF || object == nullptr) {
        if (r_err) {
			*r_err = PROP_NOT_FOUND;
		}
        return false;
	}

	CharString key_cs = String(p_name).utf8();
	const char* key = key_cs.get_data();

    lua_getref(L, self_ref); // self
    lua_pushstring(L, key); // self, key

    lua_rawget(L, -2);
    if (!lua_isnil(L, -1)) {
		if (r_err) {
			*r_err = PROP_OK;
		}

		r_is_function = lua_isfunction(L, -1) != 0;
		r_ret = LuauBridge::get_variant(L, -1);
		lua_pop(L, 2);

		return true;
    }
    lua_pop(L, 2);
    
    if (strcmp(key, "self") == 0) {
		if (r_err) {
			*r_err = PROP_OK;
		}

        r_ret = Variant(object);
        return true;
    }

	if (r_err) {
		*r_err = PROP_NOT_FOUND;
	}
	return false;
}


GDExtensionPropertyInfo *LuauScriptInstance::get_property_list(uint32_t *r_count) {
    // get properties from script definition (static properties)
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
        lua_getref(L, self_ref); //get from self
        
        if (lua_istable(L, -1)) {
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

Variant::Type LuauScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
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
    // check self for runtime methods
    if (L && self_ref != LUA_NOREF) {
        lua_getref(L, self_ref);
        String method_str = String(p_name);
        lua_rawgetfield(L, -1, method_str.utf8().get_data());
        bool is_func = lua_isfunction(L, -1);
        lua_pop(L, 2); // Remove function/nil and self table
        if (is_func) {
            return true;
        }
    }
    
    // fall back to checking the script metadata
    const LuauScript *s = script.ptr();
    while (s) {
        if (s->definition.methods.has(p_name)) {
            return true;
        }
        s = s->base.ptr();
    }
    return false;
}

Object* LuauScriptInstance::get_owner() const {
    return owner;
}

Ref<LuauScript> LuauScriptInstance::get_script() const {
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
    lua_rawgetfield(ET, -1, method_str.utf8().get_data());
    
    if (!lua_isfunction(ET, -1)) {
        lua_pop(ET, 2); // Remove non-function and self table
        return LUA_ERRRUN;
    }
    
    lua_remove(ET, -2);	//remove self
	lua_insert(ET, -(argc + 1)); //move function to the top
    

    int reentry_depth_before = LuauReentryGuard::depth();
    int call_result = lua_resume(ET, T, argc);
    LuauReentryGuard::restore(reentry_depth_before);


    if (call_result == LUA_YIELD) {
        luau_track_suspended_thread(ET, T, lua_gettop(T));
        return LUA_YIELD;
    }

    if (call_result != LUA_OK) {
        const char* error_msg = lua_tostring(ET, -1);
        if (error_msg) {
            UtilityFunctions::printerr(vformat("Luau script error in %s: %s", p_method, error_msg));
        }
        lua_pop(ET, 1);
    }
    
    return call_result;
}

LuauScriptInstance::LuauScriptInstance(const Ref<LuauScript> &p_script, Object *p_owner, LuauEngine::VMType p_vmtype) 
	: script(p_script), owner(p_owner), vm_type(p_vmtype) {
}

LuauScriptInstance::~LuauScriptInstance() {
	if (owner != nullptr) {
		LuauObject::unregister_object(owner);
	}

	for (auto &pair : held_member_refs) {
		if (pair.value != nullptr) {
			pair.value->unreference();
		}
	}
	held_member_refs.clear();

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
	if (!script->_is_placeholder_fallback_enabled()) {
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
				values.erase(p_name); //erase if it's default
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

			if (op_valid && op_result.operator bool()){
				values[p_name] = p_value;
			}

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
		//WARN_PRINT(vformat("LuauScript PlaceHolderScriptInstance::get %s=%s", p_name, r_ret));
		return true;
	}

	if (constants.has(p_name)) {
		r_ret = constants[p_name];
		return true;
	}

	if (script->_is_placeholder_fallback_enabled() && script->_has_property_default_value(p_name)) {
		r_ret = script->_get_property_default_value(p_name);
		return true;
	}

	if (r_err)
		*r_err = PROP_NOT_FOUND;

	return false;
}

void PlaceHolderScriptInstance::update(const Vector<GDClassProperty> &p_properties) {
	HashSet<StringName> new_values;
	HashMap<StringName, GDClassProperty> prop_map;
	
	properties.clear();
	for (GDClassProperty prop : p_properties) {
		GDProperty property = prop.property;

		if (property.usage & (PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP | PROPERTY_USAGE_CATEGORY)) {
			continue;
		}

		StringName p_name = property.name;
		new_values.insert(p_name);
	
		properties.push_back(prop.property);
		prop_map[p_name] = prop;
	}

	List<StringName> to_remove;

	for (KeyValue<StringName, Variant> &E : values) {
		if (!new_values.has(E.key)) {
			to_remove.push_back(E.key);
		}

		Variant defval = prop_map.has(E.key) ? prop_map[E.key].default_value : Variant();
		if (script->get_property_default_value(E.key)) {
			//remove because it's the same as the default value
			if (defval == E.value) {
				to_remove.push_back(E.key);
			}
		}
	}

	while (to_remove.size()) {
		values.erase(to_remove.front()->get());
		to_remove.pop_front();
	}

	constants.clear();
	Dictionary consts_dict = script->_get_constants();
	Array keys = consts_dict.keys();
	for (int i = 0; i < keys.size(); ++i) {
		Variant key = keys[i];
		constants[StringName(key)] = consts_dict[key];
	}

	if (owner && owner->get_script() == this) {
		owner->notify_property_list_changed();
	}
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
		int i = 0;
		for(GDProperty prop : properties) {
			GDExtensionPropertyInfo dst;
			copy_prop(prop, dst);

			props[i] = dst;
			i++;
		}
		
	} else {
		int i = 0;
		for(GDProperty prop : properties) {
			GDExtensionPropertyInfo &pinfo = props[i];
			copy_prop(prop, pinfo);

			if (!values.has(prop.name))
				pinfo.usage |= PROPERTY_USAGE_SCRIPT_DEFAULT_VALUE;

			i++;
		}
	}

	*r_count = size;

	if (size == 0) {
		return nullptr;
	}

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

	script->placeholders.insert(p_owner->get_instance_id(), this);
	script->update_exports_internal(this);
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
	Error reload_err = OK;
	
#ifdef TOOLS_ENABLED
	// In the editor, handle existing instances appropriately
	if (!p_keep_state && instances.size() > 0) {
		MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
		
		// In editor mode, we allow reload with existing instances
		// The instances will be updated after the script is reloaded
		placeholder_fallback_enabled = true;

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
	WARN_PRINT(vformat("Re-load script: %s", get_path()));
	reload_err = load(LOAD_FULL, true);
	
#ifdef TOOLS_ENABLED
	// After successful reload, update instances if needed
	if (reload_err == OK) {
		if (placeholder_fallback_enabled) {
			MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
			
			for (const KeyValue<uint64_t, PlaceHolderScriptInstance *> &E : placeholders) {
				PlaceHolderScriptInstance *placeholder = E.value;
				if (placeholder) {
					placeholder->update(definition.properties);
				}
			}
		}
		
		// Update regular instances if keeping state
		if (p_keep_state) {
			MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
			
			//MARK: TODO Hot Reload
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
	// Array scripts = LuauLanguage::get_singleton()->get_scripts();

	// for (Ref<LuauScript> &scr : scripts) {
	// 	// Check dependent to avoid endless loop.
	// 	if (scr->has_dependency(this) && !this->has_dependency(scr))
	// 		scr->_update_exports();
	// }
#endif // TOOLS_ENABLED
}

StringName LuauScript::_get_doc_class_name() const {
	if (!definition.name.is_empty()) {
		return definition.name;
	}
	if (!definition.extends.is_empty()) {
		return definition.extends;
	}
	return StringName();
}

bool LuauScript::_has_script_signal(const StringName &p_signal) const {
	// Check if this script or any of its base scripts has the given signal
	const LuauScript *s = this;
	
	while (s) {
		if (s->definition.signals.has(p_signal)) {
			return true;
		}
		s = s->base.ptr();
	}
	
	return false;
}

TypedArray<Dictionary> LuauScript::_get_script_method_list() const {
	TypedArray<Dictionary> methods;
	HashSet<StringName> seen;

	const LuauScript *s = this;

	// Return methods from this script and all base scripts
	while (s) {
		for (const KeyValue<StringName, GDMethod> &pair : s->definition.methods) {
			// Skip if we've already seen this method (overridden in derived class)
			if (seen.has(pair.key)) {
				continue;
			}
			seen.insert(pair.key);
			
			// Convert GDMethod to Dictionary format expected by Godot
			methods.push_back(pair.value.operator Dictionary());
		}

		s = s->base.ptr();
	}

	return methods;
}

TypedArray<Dictionary> LuauScript::_get_script_property_list() const {
	TypedArray<Dictionary> properties;

	const LuauScript *s = this;

	// Return global variables (properties)
	while (s) {
		// Reverse to add properties from base scripts first.
		for (int i = s->definition.properties.size() - 1; i >= 0; i--) {
			const GDClassProperty &prop = s->definition.properties[i];
			properties.push_front(prop.property.operator Dictionary());
		}

		s = s->base.ptr();
	}

	return properties;
}

Dictionary LuauScript::_get_constants() const {
	Dictionary constants_dict;

	for (const KeyValue<StringName, Variant> &pair : constants)
		constants_dict[pair.key] = pair.value;

	return constants_dict;
}

TypedArray<StringName> LuauScript::_get_members() const {
	TypedArray<StringName> members;

	// Return local variables (members)
	for (const GDClassProperty &member : definition.members)
		members.push_back(member.property.name);

	return members;
}

bool LuauScript::_is_placeholder_fallback_enabled() const {
#ifdef TOOLS_ENABLED
	return placeholder_fallback_enabled;
#else
	return false;
#endif // TOOLS_ENABLED
}

bool LuauScript::is_placeholder_fallback_enabled() const {
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

//MARK: Annotation helpers
static String parse_annotation(const String &p_trimmed_line) {
    String trimmed = p_trimmed_line.strip_edges();
    int pre = trimmed.begins_with("---") ? 3 : (trimmed.begins_with("--") ? 2 : -1);
    if (pre < 0) {
        return String();
    }
    String raw_body = trimmed.substr(pre);
    if (raw_body.begins_with("@")) {
        static bool warned = false;
        if (!warned) {
            WARN_PRINT("Luau: spaceless annotation form (e.g. '---@extends') is deprecated; use '--- @extends' (space after ---).");
            warned = true;
        }
    }
    return raw_body.strip_edges();
}

// extract anno args
static String extract_paren_args(const String &p_anno) {
    int open = p_anno.find("(");
    if (open < 0) {
        return String();
    }
    int close = p_anno.find(")", open);
    if (close < 0) {
        return String();
    }
    return p_anno.substr(open + 1, close - open - 1).strip_edges();
}

static String normalize_csv(const String &p_csv) {
    PackedStringArray parts = p_csv.split(",");
    String result;
    for (int i = 0; i < parts.size(); i++) {
        String p = parts[i].strip_edges().replace("\"", "").replace("'", "");
        if (i > 0) {
            result += ",";
        }
        result += p;
    }
    return result;
}

static Vector<String> collect_leading_annotations(const PackedStringArray &p_lines, int p_ast_line) {
    Vector<String> out;
    int i = p_ast_line - 1;
    while (i >= 0) {
        String line = p_lines[i];
        String trimmed = line.strip_edges();
        if (trimmed.is_empty()) {
            break;
        }
        if (!trimmed.begins_with("---") && !trimmed.begins_with("--")) {
            break;
        }
        String anno = parse_annotation(trimmed);
        if (!anno.is_empty()) {
            out.push_back(anno);
        }
        i--;
    }
    return out;
}

static void parse_export_annotation(const String &p_anno, GDClassProperty &p_prop, Variant::Type p_type) {
    if (p_anno == "@export") {
        p_prop.property.usage = p_prop.property.usage | PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
        return;
    }
    if (p_anno.begins_with("@export_range")) {
        p_prop.property.hint = PROPERTY_HINT_RANGE;
        p_prop.property.hint_string = normalize_csv(extract_paren_args(p_anno));
        p_prop.property.usage = p_prop.property.usage | PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
        return;
    }
    if (p_anno.begins_with("@export_enum")) {
        p_prop.property.hint = PROPERTY_HINT_ENUM;
        p_prop.property.hint_string = normalize_csv(extract_paren_args(p_anno));
        p_prop.property.usage = p_prop.property.usage | PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
        return;
    }
    if (p_anno.begins_with("@export_flags")) {
        p_prop.property.hint = PROPERTY_HINT_FLAGS;
        p_prop.property.hint_string = normalize_csv(extract_paren_args(p_anno));
        p_prop.property.usage = p_prop.property.usage | PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
        return;
    }
    if (p_anno.begins_with("@export_file")) {
        p_prop.property.hint = PROPERTY_HINT_FILE;
        p_prop.property.hint_string = extract_paren_args(p_anno).replace("\"", "").replace("'", "").strip_edges();
        p_prop.property.usage = p_prop.property.usage | PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
        return;
    }
    if (p_anno == "@export_node_path") {
        // ?
        p_prop.property.hint = PROPERTY_HINT_NONE;
        p_prop.property.usage = p_prop.property.usage | PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
        return;
    }
    if (p_anno == "@export_multiline") {
        p_prop.property.hint = PROPERTY_HINT_MULTILINE_TEXT;
        p_prop.property.usage = p_prop.property.usage | PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
        return;
    }
}

Error LuauScript::load(LoadStage p_load_stage, bool p_force) {
    if (!p_force && load_stage >= p_load_stage) {
        return OK;
    }
    
    if (source.is_empty()) {
        ERR_FAIL_V_MSG(ERR_INVALID_DATA, "Script source is empty");
    }
    
    Error err = OK;
    
    // Compile
    if (p_load_stage >= LOAD_COMPILE) {
        CharString utf8 = source.utf8();
        std::string source_str(utf8.get_data(), utf8.length());
		
        Luau::CompileOptions compile_opts;
        compile_opts.optimizationLevel = 2; // Full optimization
        compile_opts.debugLevel = 2; // Full debug info
        compile_opts.typeInfoLevel = 1; // Generate type info for all modules
        compile_opts.coverageLevel = 0; // No coverage by default
        
        try {
            std::string compiled = Luau::compile(source_str, compile_opts);
            
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
    
    // Parse and analyse
    if (p_load_stage >= LOAD_ANALYSIS) {
        CharString utf8 = source.utf8();
        std::string source_str(utf8.get_data(), utf8.length());
        
        Luau::Allocator allocator;
        Luau::AstNameTable names(allocator);
        
        Luau::ParseOptions parse_opts;
        Luau::ParseResult parse_result = Luau::Parser::parse(
            source_str.c_str(), source_str.size(), names, allocator, parse_opts);
        
        if (!parse_result.errors.empty()) {
            const auto &error = parse_result.errors[0];
            ERR_FAIL_V_MSG(ERR_PARSE_ERROR,
                vformat("Parse error at line %d: %s",
                    error.getLocation().begin.line + 1, error.getMessage().c_str()));
        }
        
        // Extract from AST
        if (parse_result.root) {
            definition.methods.clear();
            definition.properties.clear();
            definition.property_indices.clear();
            definition.members.clear();
            definition.member_indices.clear();
            definition.signals.clear();
            definition.constants.clear();
            constants.clear();
            
            if (definition.name.is_empty()) {
                String path = get_path();
                if (!path.is_empty()) {
                    definition.name = path.get_file().get_basename();
                }
            }
            
            if (definition.extends.is_empty()) {
                definition.extends = "RefCounted";
            }

            PackedStringArray lines_packed = source.split("\n");

            {
                // MARK: Config annotations
                for (int i = 0; i < lines_packed.size(); i++) {
                    String line = lines_packed[i];
                    String trimmed = line.strip_edges();
                    
                    if (trimmed.begins_with("---") || trimmed.begins_with("--")) {
                        String comment = parse_annotation(trimmed);
                        
                        // @extends annotation
                        if (comment.begins_with("@extends ")) {
                            String base_class = comment.substr(9).strip_edges();
                            if (!base_class.is_empty()) {
                                definition.extends = base_class;
                            }
                        }
                        // @class annotation
                        else if (comment.begins_with("@class ")) {
                            String class_name = comment.substr(7).strip_edges();
                            if (!class_name.is_empty()) {
                                definition.name = class_name; //MARK: TODO custom class
                            }
                        }
                        // @tool annotation
                        else if (comment == "@tool") {
                            definition.is_tool = true;
                        }

                    } else if (!trimmed.is_empty() && !trimmed.begins_with("--")) {
                        break;
                    }
                }
            }
			String class_name = definition.name;

            // Ast for metadata
            for (Luau::AstStat* stat : parse_result.root->body) {
				// Global vars (e.g., ACONST = 123)
                if (auto* assign = stat->as<Luau::AstStatAssign>()) {
                    for (size_t i = 0; i < assign->vars.size; i++) {
						if (i >= assign->values.size || !assign->values.data[i]) continue;
						
						auto* global = assign->vars.data[i]->as<Luau::AstExprGlobal>();
						if (!global) continue;

						String var_name = String(global->name.value);
						Luau::AstExpr* value = assign->values.data[i];
						
						//constant convention
						bool is_constant = true;
						for (int j = 0; j < var_name.length(); j++) {
							char32_t c = var_name[j];
							if ((c >= 'a' && c <= 'z')) {
								is_constant = false;
								break;
							}
						}
						
						//extract value from ast expression
						Variant::Type var_type;
						Variant var_value;
						StringName native_type;
						
						auto* type_assert = value->as<Luau::AstExprTypeAssertion>();
						if (!type_assert) continue; //untyped assignment;

						AstExprResult expr_result = extract_ast_expr_value(type_assert->expr, type_assert->annotation);
						
						if (expr_result.success) {
							var_value = expr_result.value;
							var_type = expr_result.type;
							native_type = expr_result.native_type;

						} else {
							// failed to extract
						}

						if (is_constant) {
							constants[StringName(var_name)] = var_value;
							definition.constants[StringName(var_name)] = var_value;
							continue;
						}

						String var_type_name = Variant::get_type_name(var_type);
						// Declare variable definition
						GDClassProperty var_def;

						var_def.property.name = StringName(var_name);
						var_def.property.type = GDEXTENSION_VARIANT_TYPE_NIL;
						var_def.property.class_name = class_name;
						var_def.property.hint = PROPERTY_HINT_NONE;
						var_def.property.hint_string = "";
						var_def.property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE;

						var_def.default_value = var_value;
						
						//WARN_PRINT(vformat(">> Type(%s) %s=%s (%s) native_type=%s",Variant::get_type_name(var_type), var_name, var_value, var_value.get_type_name(var_value.get_type()), native_type));
						
						if (var_type == Variant::OBJECT) {
							var_def.property.type = GDEXTENSION_VARIANT_TYPE_OBJECT;
							var_def.property.hint_string = native_type;

							if (ClassDB::is_parent_class(native_type, StringName("Resource"))) {
								var_def.property.hint = PROPERTY_HINT_RESOURCE_TYPE;
							} else if (ClassDB::is_parent_class(native_type, StringName("Node"))) {
								var_def.property.hint = PROPERTY_HINT_NODE_TYPE;
							}

						} else if (var_type == Variant::ARRAY && !native_type.is_empty()) {
							var_def.property.type = GDEXTENSION_VARIANT_TYPE_ARRAY;
							var_def.property.hint = PROPERTY_HINT_TYPE_STRING;
		
							if (ClassDB::is_parent_class(native_type, StringName("Resource"))) {
								Array hint_values;
								hint_values.resize(3);
								hint_values[0] = Variant::OBJECT;
								hint_values[1] = PROPERTY_HINT_RESOURCE_TYPE;
								hint_values[2] = native_type;
								var_def.property.hint_string = String("{0}/{1}:{2}").format(hint_values);
	
							} else {
								Variant::Type typed_var = parse_type_name(native_type);

								Array hint_values;
								hint_values.resize(3);
								hint_values[0] = typed_var;

								if (typed_var == Variant::OBJECT) {
									hint_values[1] = PROPERTY_HINT_NODE_TYPE;
									hint_values[2] = native_type;

								} else if (typed_var == Variant::STRING) {
									hint_values[1] = PROPERTY_HINT_TYPE_STRING;
									hint_values[2] = native_type;

								} else {
									hint_values[1] = PROPERTY_HINT_NONE;
									hint_values[2] = "";

								}

								var_def.property.hint_string = String("{0}/{1}:{2}").format(hint_values);
	
							}
	
						} else {
							switch(var_type) {
								case Variant::BOOL:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_BOOL;
									break;
								case Variant::INT:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_INT;
									break;
								case Variant::FLOAT:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_FLOAT;
									break;
								case Variant::STRING:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_STRING;
									break;
								case Variant::VECTOR2:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_VECTOR2;
									break;
								case Variant::VECTOR3:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_VECTOR3;
									break;
								case Variant::COLOR:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_COLOR;
									break;
								case Variant::RECT2:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_RECT2;
									break;
								case Variant::TRANSFORM2D:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_TRANSFORM2D;
									break;
								case Variant::PLANE:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_PLANE;
									break;
								case Variant::QUATERNION:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_QUATERNION;
									break;
								case Variant::AABB:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_AABB;
									break;
								case Variant::BASIS:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_BASIS;
									break;
								case Variant::TRANSFORM3D:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_TRANSFORM3D;
									break;
								case Variant::PROJECTION:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_PROJECTION;
									break;
								case Variant::STRING_NAME:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_STRING_NAME;
									break;
								case Variant::NODE_PATH:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_NODE_PATH;
									break;
								case Variant::RID:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_RID;
									break;
								case Variant::DICTIONARY:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_DICTIONARY;
									break;
								case Variant::ARRAY:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_ARRAY;
									break;
								case Variant::PACKED_BYTE_ARRAY:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY;
									break;
								case Variant::PACKED_INT32_ARRAY:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_PACKED_INT32_ARRAY;
									break;
								case Variant::PACKED_INT64_ARRAY:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_PACKED_INT64_ARRAY;
									break;
								case Variant::PACKED_FLOAT32_ARRAY:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT32_ARRAY;
									break;
								case Variant::PACKED_FLOAT64_ARRAY:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT64_ARRAY;
									break;
								case Variant::PACKED_STRING_ARRAY:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_PACKED_STRING_ARRAY;
									break;
								case Variant::PACKED_VECTOR2_ARRAY:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR2_ARRAY;
									break;
								case Variant::PACKED_VECTOR3_ARRAY:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR3_ARRAY;
									break;
								case Variant::PACKED_COLOR_ARRAY:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_PACKED_COLOR_ARRAY;
									break;
								default:
									var_def.property.type = GDEXTENSION_VARIANT_TYPE_NIL;
									break;
							}

							var_def.property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT;
							var_def.property.hint_string = "";
						}

						{
							Vector<String> annos = collect_leading_annotations(lines_packed, stat->location.begin.line);
							String pending_group;
							String pending_subgroup;
							for (const String &anno : annos) {
								if (anno.begins_with("@export_group")) {
									pending_group = extract_paren_args(anno).replace("\"", "").replace("'", "").strip_edges();
								} else if (anno.begins_with("@export_subgroup")) {
									pending_subgroup = extract_paren_args(anno).replace("\"", "").replace("'", "").strip_edges();
								} else if (anno.begins_with("@export")) {
									parse_export_annotation(anno, var_def, var_type);
								}
							}

							if (!pending_group.is_empty()) {
								GDClassProperty grp;
								grp.property.name = StringName(pending_group);
								grp.property.type = GDEXTENSION_VARIANT_TYPE_NIL;
								grp.property.usage = PROPERTY_USAGE_GROUP;
								definition.properties.push_back(grp);
								definition.property_indices[StringName(pending_group)] = definition.properties.size() - 1;
							}
							if (!pending_subgroup.is_empty()) {
								GDClassProperty grp;
								grp.property.name = StringName(pending_subgroup);
								grp.property.type = GDEXTENSION_VARIANT_TYPE_NIL;
								grp.property.usage = PROPERTY_USAGE_SUBGROUP;
								definition.properties.push_back(grp);
								definition.property_indices[StringName(pending_subgroup)] = definition.properties.size() - 1;
							}
						}

						definition.members.push_back(var_def);
						definition.member_indices[StringName(var_name)] = definition.members.size() - 1;
						
						definition.properties.push_back(var_def);
						definition.property_indices[StringName(var_name)] = definition.properties.size() - 1;
                    }
                }
                // Local vars (e.g. local hello = "world")
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
                            GDExtensionVariantType var_type = GDEXTENSION_VARIANT_TYPE_NIL;
                            bool has_type_annotation = false;
                            
                            // Function to map identifier to variant type
                            auto map_identifier_to_type = [](const String& identifier) -> GDExtensionVariantType {
                                // Atomic types
                                if (identifier == "bool" || identifier == "boolean") return GDEXTENSION_VARIANT_TYPE_BOOL;
                                if (identifier == "int" || identifier == "integer") return GDEXTENSION_VARIANT_TYPE_INT;
                                if (identifier == "float" || identifier == "number") return GDEXTENSION_VARIANT_TYPE_FLOAT;
                                if (identifier == "string") return GDEXTENSION_VARIANT_TYPE_STRING;
                                
                                // Math types
                                if (identifier == "Vector2") return GDEXTENSION_VARIANT_TYPE_VECTOR2;
                                if (identifier == "Vector2i" || identifier == "Vector2I") return GDEXTENSION_VARIANT_TYPE_VECTOR2I;
                                if (identifier == "Rect2") return GDEXTENSION_VARIANT_TYPE_RECT2;
                                if (identifier == "Rect2i" || identifier == "Rect2I") return GDEXTENSION_VARIANT_TYPE_RECT2I;
                                if (identifier == "Vector3") return GDEXTENSION_VARIANT_TYPE_VECTOR3;
                                if (identifier == "Vector3i" || identifier == "Vector3I") return GDEXTENSION_VARIANT_TYPE_VECTOR3I;
                                if (identifier == "Transform2D") return GDEXTENSION_VARIANT_TYPE_TRANSFORM2D;
                                if (identifier == "Vector4") return GDEXTENSION_VARIANT_TYPE_VECTOR4;
                                if (identifier == "Vector4i" || identifier == "Vector4I") return GDEXTENSION_VARIANT_TYPE_VECTOR4I;
                                if (identifier == "Plane") return GDEXTENSION_VARIANT_TYPE_PLANE;
                                if (identifier == "Quaternion" || identifier == "Quat") return GDEXTENSION_VARIANT_TYPE_QUATERNION;
                                if (identifier == "AABB") return GDEXTENSION_VARIANT_TYPE_AABB;
                                if (identifier == "Basis") return GDEXTENSION_VARIANT_TYPE_BASIS;
                                if (identifier == "Transform3D") return GDEXTENSION_VARIANT_TYPE_TRANSFORM3D;
                                if (identifier == "Projection") return GDEXTENSION_VARIANT_TYPE_PROJECTION;
                                
                                // Misc types
                                if (identifier == "Color") return GDEXTENSION_VARIANT_TYPE_COLOR;
                                if (identifier == "StringName") return GDEXTENSION_VARIANT_TYPE_STRING_NAME;
                                if (identifier == "NodePath") return GDEXTENSION_VARIANT_TYPE_NODE_PATH;
                                if (identifier == "RID") return GDEXTENSION_VARIANT_TYPE_RID;
                                if (identifier == "Object") return GDEXTENSION_VARIANT_TYPE_OBJECT;
                                if (identifier == "Callable") return GDEXTENSION_VARIANT_TYPE_CALLABLE;
                                if (identifier == "Signal") return GDEXTENSION_VARIANT_TYPE_SIGNAL;
                                if (identifier == "Dictionary" || identifier == "dict") return GDEXTENSION_VARIANT_TYPE_DICTIONARY;
                                if (identifier == "Array" || identifier == "array") return GDEXTENSION_VARIANT_TYPE_ARRAY;
                                
                                // Packed arrays
                                if (identifier == "PackedByteArray") return GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY;
                                if (identifier == "PackedInt32Array") return GDEXTENSION_VARIANT_TYPE_PACKED_INT32_ARRAY;
                                if (identifier == "PackedInt64Array") return GDEXTENSION_VARIANT_TYPE_PACKED_INT64_ARRAY;
                                if (identifier == "PackedFloat32Array") return GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT32_ARRAY;
                                if (identifier == "PackedFloat64Array") return GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT64_ARRAY;
                                if (identifier == "PackedStringArray") return GDEXTENSION_VARIANT_TYPE_PACKED_STRING_ARRAY;
                                if (identifier == "PackedVector2Array") return GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR2_ARRAY;
                                if (identifier == "PackedVector3Array") return GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR3_ARRAY;
                                if (identifier == "PackedColorArray") return GDEXTENSION_VARIANT_TYPE_PACKED_COLOR_ARRAY;
                                if (identifier == "PackedVector4Array") return GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR4_ARRAY;
                                
                                return GDEXTENSION_VARIANT_TYPE_NIL;
                            };

                            // Check if the variable has a type annotation
                            if (var->annotation) {
                                // Extract type from annotation
                                if (auto* ref = var->annotation->as<Luau::AstTypeReference>()) {
                                    String type_name = String(ref->name.value);
                                    
                                    // Map Godot types using the lambda
                                    var_type = map_identifier_to_type(type_name);
                                    if (var_type != GDEXTENSION_VARIANT_TYPE_NIL) {
                                        has_type_annotation = true;
                                    }
                                }
                            } else if (auto* index_name = value->as<Luau::AstExprIndexName>()) {
                                // No type annotation, but we have a field access like Color.RED
                                if (auto* global = index_name->expr->as<Luau::AstExprGlobal>()) {
                                    String base_type = String(global->name.value);
                                    var_type = map_identifier_to_type(base_type);
                                    if (var_type != GDEXTENSION_VARIANT_TYPE_NIL) {
                                        has_type_annotation = true;
                                    }
                                }
                            }
                            
                            
                            if (auto* num = value->as<Luau::AstExprConstantNumber>()) {
                                var_value = num->value;
                                has_value = true;
                                if (!has_type_annotation) {
                                    var_type = GDEXTENSION_VARIANT_TYPE_FLOAT;
                                }
                            } else if (auto* str = value->as<Luau::AstExprConstantString>()) {
                                var_value = String::utf8(str->value.data, str->value.size);
                                has_value = true;
                                if (!has_type_annotation) {
                                    var_type = GDEXTENSION_VARIANT_TYPE_STRING;
                                }
                            } else if (auto* bool_val = value->as<Luau::AstExprConstantBool>()) {
                                var_value = bool_val->value;
                                has_value = true;
                                if (!has_type_annotation) {
                                    var_type = GDEXTENSION_VARIANT_TYPE_BOOL;
                                }
							} else if (value->as<Luau::AstExprTable>()) {
                                var_value = Dictionary();
                                has_value = true;
                                if (!has_type_annotation) {
                                    var_type = GDEXTENSION_VARIANT_TYPE_DICTIONARY;
                                }
                            } else if (value->as<Luau::AstExprConstantNil>()) {
                                var_value = Variant();
                                has_value = true;
                            } else if (has_type_annotation) {
                                // For typed variables with complex expressions (like Color.RED),
                                // we can't extract the value at compile time, but we know the type
                                has_value = true;
                                // Set a default value based on type
                                switch (var_type) {
                                    // Atomic types
                                    case GDEXTENSION_VARIANT_TYPE_BOOL:
                                        var_value = false;
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_INT:
                                        var_value = 0;
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_FLOAT:
                                        var_value = 0.0f;
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_STRING:
                                        var_value = String();
                                        break;
                                    
                                    // Math types
                                    case GDEXTENSION_VARIANT_TYPE_VECTOR2:
                                        var_value = Vector2();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_VECTOR2I:
                                        var_value = Vector2i();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_RECT2:
                                        var_value = Rect2();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_RECT2I:
                                        var_value = Rect2i();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_VECTOR3:
                                        var_value = Vector3();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_VECTOR3I:
                                        var_value = Vector3i();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_TRANSFORM2D:
                                        var_value = Transform2D();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_VECTOR4:
                                        var_value = Vector4();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_VECTOR4I:
                                        var_value = Vector4i();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_PLANE:
                                        var_value = Plane();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_QUATERNION:
                                        var_value = Quaternion();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_AABB:
                                        var_value = AABB();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_BASIS:
                                        var_value = Basis();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_TRANSFORM3D:
                                        var_value = Transform3D();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_PROJECTION:
                                        var_value = Projection();
                                        break;
                                    
                                    // Misc types
                                    case GDEXTENSION_VARIANT_TYPE_COLOR:
                                        var_value = Color();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_STRING_NAME:
                                        var_value = StringName();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_NODE_PATH:
                                        var_value = NodePath();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_RID:
                                        var_value = RID();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_CALLABLE:
                                        var_value = Callable();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_SIGNAL:
                                        var_value = Signal();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_DICTIONARY:
                                        var_value = Dictionary();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_ARRAY:
                                        var_value = Array();
                                        break;
                                    
                                    // Packed arrays
                                    case GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY:
                                        var_value = PackedByteArray();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_PACKED_INT32_ARRAY:
                                        var_value = PackedInt32Array();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_PACKED_INT64_ARRAY:
                                        var_value = PackedInt64Array();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT32_ARRAY:
                                        var_value = PackedFloat32Array();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT64_ARRAY:
                                        var_value = PackedFloat64Array();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_PACKED_STRING_ARRAY:
                                        var_value = PackedStringArray();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR2_ARRAY:
                                        var_value = PackedVector2Array();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR3_ARRAY:
                                        var_value = PackedVector3Array();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_PACKED_COLOR_ARRAY:
                                        var_value = PackedColorArray();
                                        break;
                                    case GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR4_ARRAY:
                                        var_value = PackedVector4Array();
                                        break;
                                    
                                    default:
                                        var_value = Variant();
                                        break;
                                }
                            }
                            
                            if (has_value) {
                                if (is_constant) {
									// ALL_CAPS variables are constants
									constants[StringName(var_name)] = var_value;
									definition.constants[StringName(var_name)] = var_value;
								} else {
									// Create the variable definition
									GDClassProperty var_def;
									var_def.property.name = StringName(var_name);
									var_def.property.class_name = "";
									var_def.property.hint = PROPERTY_HINT_NONE;
									var_def.property.hint_string = "";
									var_def.property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE;
									var_def.default_value = var_value;

									// Use type from annotation if available, otherwise determine from value
									if (has_type_annotation) {
										var_def.property.type = var_type;
										if (var_type == GDEXTENSION_VARIANT_TYPE_NIL) {
											var_def.property.usage = var_def.property.usage | PROPERTY_USAGE_NIL_IS_VARIANT;
										}
									} else {
										// Determine type from value
										switch (var_value.get_type()) {
											case Variant::BOOL:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_BOOL;
												break;
											case Variant::INT:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_INT;
												break;
											case Variant::FLOAT:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_FLOAT;
												break;
											case Variant::STRING:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_STRING;
												break;
											case Variant::COLOR:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_COLOR;
												break;
											case Variant::VECTOR2:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_VECTOR2;
												break;
											case Variant::VECTOR3:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_VECTOR3;
												break;
											case Variant::TRANSFORM2D:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_TRANSFORM2D;
												break;
											case Variant::TRANSFORM3D:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_TRANSFORM3D;
												break;
											case Variant::RECT2:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_RECT2;
												break;
											case Variant::DICTIONARY:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_DICTIONARY;
												break;
											default:
												var_def.property.type = GDEXTENSION_VARIANT_TYPE_NIL;
												var_def.property.usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE | PROPERTY_USAGE_NIL_IS_VARIANT;
												break;
										}
									}
									
									// Local variables go to members only (not accessible from outside)
									definition.members.push_back(var_def);
									definition.member_indices[StringName(var_name)] = definition.members.size() - 1;
								}
                            }
                        }
                    }
                }
                // functions 
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
                        
                        if (!method_name.is_empty()) {
                            {
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
            
        }
        
        load_stage = LOAD_ANALYSIS;
    }
    
    // Link and validate
    if (p_load_stage >= LOAD_FULL) {
        // Resolve base script references if extends another Luau script
        if (!definition.extends.is_empty()) {
            //MARK: extends Custom Class
            if (definition.extends.begins_with("res://")) {
                Ref<LuauScript> base_script = ResourceLoader::get_singleton()->load(definition.extends);
                if (base_script.is_valid()) {
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
        
        load_stage = LOAD_FULL;
        
#ifdef TOOLS_ENABLED
        //MARK: debug print loaded information
        if (Engine::get_singleton()->is_editor_hint()) {
            int method_count = definition.methods.size();
            int property_count = definition.properties.size();
            int signal_count = definition.signals.size();
            int constant_count = definition.constants.size();
			int member_count = definition.members.size();
            
			print_verbose(
				vformat(
					"LuauScript loaded: %s (extends %s) - %d methods, %d properties, %d signals, %d constants, %d members",
					definition.name.is_empty() ? get_path() : definition.name,
					definition.extends.is_empty() ? "RefCounted" : definition.extends,
					method_count, property_count, signal_count, constant_count, member_count
				)
			);
			// //WARN_PRINT members
			// for (const GDClassProperty &member : definition.members) {
			// 	WARN_PRINT(vformat("Member: %s", member.property.name));
			// }
			// //WARN_PRINT properties
			// for (const GDClassProperty &prop : definition.properties) {
			// 	WARN_PRINT(vformat("Property: %s", prop.property.name));
			// }
			// //WARN_PRINT constants
			// for (const KeyValue<StringName, Variant> &pair : constants) {
			// 	WARN_PRINT(vformat("Constant: %s = %s", pair.key, pair.value));
			// }
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


//MARK: initialize_lua_state
void LuauScriptInstance::initialize_lua_state(lua_State *p_L, lua_State *p_thread, int p_thread_ref, int p_self_ref) {
	L = p_L;
	T = p_thread;
	thread_ref = p_thread_ref;
	self_ref = p_self_ref;
}

bool is_variant_type(const String &type_name) {
    for (int i = 0; i < Variant::VARIANT_MAX; i++) {
        if (Variant::get_type_name(Variant::Type(i)) == type_name) {
            return true;
        }
    }

    return false;
}

void LuauScriptInstance::register_signal(const StringName &p_name) {
    Ref<LuauScript> s = get_script();
    if (s.is_valid()) {
        s->definition.signals[p_name] = GDMethod();
    }
}

void *LuauScript::_instance_create(Object *obj_ptr) const {
	uint64_t obj_id = obj_ptr->get_instance_id();

	String script_name = get_path();
	if (script_name.is_empty()) {
		script_name = definition.name;
	}

#ifdef TOOLS_ENABLED
	//WARN_PRINT(vformat("Creating LuauScript instance for object: %s", obj_ptr->get_class()));
	bool should_create_placeholder = false;
	
	if (!can_instantiate()) {
		should_create_placeholder = true;
	}

	if (!should_create_placeholder && load_stage != LOAD_FULL) {
		const_cast<LuauScript*>(this)->load(LOAD_FULL, false);
		if (load_stage != LOAD_FULL) {
			// Script failed to load fully, use placeholder
			should_create_placeholder = true;
		}
	}
	
	if (!should_create_placeholder) {
		StringName base_type = _get_instance_base_type();
		if (base_type != StringName()) {
			if (!nobind::ClassDB::get_singleton()->is_parent_class(obj_ptr->get_class(), base_type)) {
				// Type mismatch, use placeholder
				should_create_placeholder = true;
			} else if (nobind::ClassDB::get_singleton()->is_parent_class(base_type, StringName("MainLoop"))) {
				should_create_placeholder = true;
			}
		}
	}
	
	if (should_create_placeholder) {
		// WARN_PRINT(vformat(
		// 	"Create Placeholder %s can_instan=%s load_stage=%s", 
		// 	script_name, 
		// 	!can_instantiate() ? "false" : "true",
		// 	load_stage
		// ));
		const_cast<LuauScript*>(this)->placeholder_fallback_enabled = true;
		return _placeholder_instance_create(obj_ptr);
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
		if (!nobind::ClassDB::get_singleton()->is_parent_class(obj_ptr->get_class(), base_type)) {
			ERR_FAIL_V_MSG(nullptr, 
				vformat("Script inherits from '%s', so it can't be assigned to an object of type '%s'", 
					base_type, obj_ptr->get_class()));
		}
	}
#endif // TOOLS_ENABLED
	

	LuauEngine::VMType vm_type = LuauEngine::VM_USER;
	LuauScriptInstance *script_instance = memnew(LuauScriptInstance(Ref<LuauScript>(this), obj_ptr, vm_type));
	
	// Register the instance with the script
	{
		MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
		const_cast<LuauScript*>(this)->instances.insert(obj_ptr->get_instance_id()); //const_case bypass const func 
	}

	if (LuauLanguage::singleton->luau) {
		lua_State* L = LuauLanguage::singleton->luau->get_vm(vm_type);
		if (L) {
			lua_State* T = lua_newthread(L);
			
			int thread_ref = lua_ref(L, -1);
			lua_pop(L, 1);
			
			lua_newtable(T); //self

			LuauBridge::push_string(T, script_name);
			lua_setfield(T, -2, "ScriptName");

			lua_xmove(T, L, 1);

			int self_ref = lua_ref(L, -1);
			lua_pop(L, 1);
			
			script_instance->initialize_lua_state(L, T, thread_ref, self_ref);
			LuauObject::register_object(obj_ptr, script_instance);
			
			if (bytecode.size() > 0) {
				int load_result = luau_load(
					T, 
					script_name.utf8().get_data(), 
					(const char*) bytecode.ptr(), 
					bytecode.size(), 
					0
				);
				
				if (load_result == 0) {
//MARK: setup script self env
					int func_idx = lua_gettop(T);

					lua_getref(L, self_ref); // self table
					lua_newtable(L); // meta

					//MARK: self.__index
					lua_getref(L, self_ref);
					LuauBridge::push_uint64_t(L, obj_id);
					lua_pushcclosure(L, [](lua_State *L) -> int {
						const char* key = lua_tostring(L, 2); // t, k
						if (!key) {
							WARN_PRINT("key == NULL");
							lua_pushnil(L);
							return 1;
						}

						lua_pushvalue(L, lua_upvalueindex(1)); // t, k, self
						if (!lua_rawequal(L, -1, 1)) {
							WARN_PRINT("t != self");
							lua_pop(L, 1);                     // pop self
							lua_pushnil(L);
							return 1;
						}
						lua_replace(L, 1);                     // self, k

						// self, k, obj
						uint64_t obj_id = LuauBridge::get_uint64_t(L, lua_upvalueindex(2));
						LuauObject* luau_object = LuauObject::get_luau_object(obj_id); 

						if (luau_object == nullptr) {
							WARN_PRINT("obj == nullptr");
							lua_pushnil(L);
							return 1;
						}

						if (strcmp(key, "signal") == 0) {
							// Bind signal() to this instance so it can register signals on the owner.
							lua_pushlightuserdata(L, luau_object);
							lua_pushcclosure(L, [](lua_State *L) -> int {
								LuauObject* luau_object = (LuauObject*)lua_touserdata(L, lua_upvalueindex(1));
								LuauScriptInstance* inst = luau_object ? luau_object->instance : nullptr;
								if (!inst) {
									luaL_error(L, "signal() can only be called from within a script instance");
									return 0;
								}

								if (lua_gettop(L) < 1) {
									luaL_error(L, "signal() requires a signal name argument");
									return 0;
								}

								Variant arg1 = LuauBridge::get_variant(L, 1);
								if (arg1.get_type() != Variant::STRING && arg1.get_type() != Variant::STRING_NAME) {
									luaL_error(L, vformat("signal() requires a String or StringName argument, got %s.", arg1.get_type_name(arg1.get_type())).utf8().get_data());
									return 0;
								}

								Object *owner = inst->get_owner();
								if (!owner) {
									luaL_error(L, "signal() called on an instance without an owner");
									return 0;
								}

								StringName sig_name = StringName(String(arg1));

								// Register to script definition
								inst->register_signal(sig_name);

								Signal sig(owner, sig_name);
								LuauBridge::push_variant(L, sig);

								return 1;
							}, "signal", 1);
							return 1;
						}

						if (nobind::ClassDB::get_singleton()->class_exists(key)) {
							//key e.g. = Node
							LuauEngine::singleton->register_and_push_godot_class(L, key);
							return 1;

						} else if (is_variant_type(key)) {
							//key e.g. = Vector3
							lua_getglobal(L, key);
							if (!lua_isnil(L, -1)) {
								lua_newtable(L);
								lua_pushvalue(L, -2); // Push global metatable
								lua_setmetatable(L, -2);
								lua_pop(L, 1); // Pop metatable

								lua_setreadonly(L, -1, true);
								return 1;
							}
						}

						lua_getglobal(L, key);
						if (!lua_isnil(L, -1)) {
							return 1;
						}


						//MARK: Godot & autoload singletons e.g. Input, Global
						if (Engine::get_singleton()->has_singleton(key)) {
							Object* singleton_obj = Engine::get_singleton()->get_singleton(key);
							if (singleton_obj != nullptr) {
								LuauBridge::push_variant(L, singleton_obj);
								return 1;
							}
						}

						String autoload_key = String("autoload/") + String(key).replace("/", "_");
						if (ProjectSettings::get_singleton()->has_setting(autoload_key)) {
							SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
							if (tree != nullptr) {
								Node* autoload_node = tree->get_root()->get_node_or_null(NodePath(String(key)));
								if (autoload_node != nullptr) {
									LuauBridge::push_variant(L, autoload_node);
									return 1;
								}
							}
						}


						return VariantBridge<Object*>::on_index(L, static_cast<Object*>(*luau_object), key);
					}, "__index", 2);
					lua_setfield(L, -2, "__index");

					
					//MARK: self.__newindex
					lua_getref(L, self_ref);
					LuauBridge::push_uint64_t(L, obj_id);
					lua_pushcclosure(L, [](lua_State *L) -> int {
						// t, k, v
						const char* key = lua_tostring(L, 2);
						if (!key) {
							lua_rawset(L, 1);
							return 0;
						}

						lua_pushvalue(L, lua_upvalueindex(1)); // self
						if (!lua_rawequal(L, -1, 1)) {
							lua_pop(L, 1);
							lua_rawset(L, 1);
							return 0;
						}
						lua_pop(L, 1);

						uint64_t obj_id = LuauBridge::get_uint64_t(L, lua_upvalueindex(2));
						Object* obj = ObjectDB::get_instance(obj_id);
						if (obj != nullptr) {
							Variant value = LuauBridge::get_variant(L, 3);
							StringName prop_name = resolve_prop_name(L, key);
							if (nobind::ClassDB::get_singleton()->class_set_property(obj, prop_name, value) == OK) {
								return 0;
							}
						}

						lua_rawset(L, 1);

						return 0;
					}, "__newindex", 2);
					lua_setfield(L, -2, "__newindex");

					lua_setmetatable(L, -2);

					lua_getref(L, self_ref);
					lua_xmove(L, T, 1);

					lua_getref(L, self_ref);
					lua_getmetatable(L, -1);
					LuauBridge::protect_metatable(L, -1);
					lua_pop(L, 2);
					lua_pop(L, 1);

					lua_setfenv(T, func_idx);
					
					
					int reentry_depth_before = LuauReentryGuard::depth();
					int call_result = lua_pcall(T, 0, 0, 0); //load pcall
					LuauReentryGuard::restore(reentry_depth_before);


					if (call_result != 0) {
						WARN_PRINT(vformat("Script execution failed for: %s", script_name));
						const char* error_msg = lua_tostring(T, -1);
						if (error_msg) {
							String error_str = String(error_msg);
							
							if (error_str.contains("attempt to call a nil value")) {
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
								ERR_PRINT(error_msg);
							}

						} else {
							ERR_PRINT(vformat("Failed to execute Luau script %s: unknown error", script_name));
							
						}
						lua_pop(T, 1); //pop err msg


#ifdef TOOLS_ENABLED
						//MARK: Editor using placeholder instance 
						{
							MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
							const_cast<LuauScript*>(this)->instances.erase(obj_ptr->get_instance_id());
						}
						memdelete(script_instance);
						
						const_cast<LuauScript*>(this)->placeholder_fallback_enabled = true;
						return _placeholder_instance_create(obj_ptr);
#endif // TOOLS_ENABLED


					} else {
						lua_getref(L, script_instance->get_self_ref());
						lua_xmove(L, T, 1);
						

						lua_pushnil(T);
						int func_count = 0;
						while (lua_next(T, -2) != 0) {
							int type = lua_type(T, -1);
							if (lua_type(T, -2) == LUA_TSTRING) {
								const char* key = lua_tostring(T, -2);
								if (key) {
									const char* type_name = lua_typename(T, type);
									if (type == LUA_TFUNCTION) {
										func_count++;
									}
								}
							}
							lua_pop(T, 1); // Remove value, keep key for next iteration
						}
						
                        lua_pushvalue(T, -1);
                        lua_getfield(T, -2, "_init");
                        int init_type = lua_type(T, -1);
                        
                        if (lua_isfunction(T, -1)) {
                            lua_insert(T, -2);
							
                            int reentry_depth_before_init = LuauReentryGuard::depth();
                            int init_result = lua_pcall(T, 1, 0, 0); // 1 argument (self)
                            LuauReentryGuard::restore(reentry_depth_before_init);
							
							if (init_result != 0) {
								const char* error_msg = lua_tostring(T, -1);
								ERR_PRINT(vformat("Failed to call _init for %s: %s", 
									script_name, error_msg ? error_msg : "unknown error")
								);
								lua_pop(T, 1); // Remove error message
							}

						} else {
							// _init is not a function or doesn't exist
							lua_pop(T, 1);
							lua_pop(T, 1);
						}
						
						lua_pop(T, 1); // Remove self table
					}
				}
			}
		}
	}
	
	// Create and return the GDExtension script instance
	return internal::gdextension_interface_script_instance_create3(
		&LuauScriptInstance::INSTANCE_INFO,
		script_instance
	);
}

void *LuauScript::_placeholder_instance_create(Object *obj_ptr) const {
    #ifdef TOOLS_ENABLED
	    PlaceHolderScriptInstance *internal = memnew(PlaceHolderScriptInstance(Ref<LuauScript>(this), obj_ptr));
		return internal::gdextension_interface_script_instance_create3(
			&PlaceHolderScriptInstance::INSTANCE_INFO, 
			internal
		);

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

ScriptInstance* LuauScript::get_instance(uint16_t p_obj_id) const {
	#ifdef TOOLS_ENABLED
		for (const KeyValue<uint64_t, PlaceHolderScriptInstance *> &E : placeholders) {
			if (E.key != p_obj_id) continue;

			PlaceHolderScriptInstance *placeholder = E.value;
			if (placeholder == nullptr) continue;

			return placeholder;
		}
	#else
		for (const KeyValue<uint64_t, LuauScriptInstance *> &E : instances) {
			if (E.key != p_obj_id) continue;

			LuauScriptInstance *instance = E.value;
			if (instance == nullptr) continue;

			return instance;
		}
	#endif

	return nullptr;
}

bool LuauScript::_editor_can_reload_from_file() {
	return true;
}

void LuauScript::_placeholder_erased(void *p_placeholder) {
#ifdef TOOLS_ENABLED
	placeholders.erase(((PlaceHolderScriptInstance *)p_placeholder)->get_owner()->get_instance_id());
#endif // TOOLS_ENABLED
}

bool LuauScript::can_instantiate() const {
#ifdef TOOLS_ENABLED
    return _is_valid() && (_is_tool() || !Engine::get_singleton()->is_editor_hint());
#else
	return _is_valid();
#endif
}

void LuauScript::update_exports_internal(PlaceHolderScriptInstance *placeholder) const {
	if (!_is_valid()) return;
	placeholder->update(definition.properties);
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
Array LuauLanguage::get_scripts() const {
    Array scripts;
    
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

	if (p_object == StringName("Node")) {
		Dictionary t;
		t["inherit"] = "Node";
		t["name"] = "Default";
		t["description"] = "Default template for Nodes with _ready and _process callbacks";
		t["content"] = R"TEMPLATE(--- @extends _BASE_CLASS_
function _ready()
end

function _process(delta: number)
end
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

struct LuauScriptDepSort {
	bool operator()(const Ref<LuauScript> &a, const Ref<LuauScript> &b) const {
		if (a == b) return false;

		const LuauScript *i = b->get_base().ptr();
		while (i) {
			if (i == a.ptr()) return true;
			
			i = i->get_base().ptr();
		}

		return false;
	}
};

void LuauLanguage::_reload_all_scripts() {
#ifdef TOOLS_ENABLED
	_reload_scripts(get_scripts(), true);
#endif // TOOLS_ENABLED
}

void LuauLanguage::_reload_scripts(const Array &p_scripts, bool p_soft_reload) {
#ifdef TOOLS_ENABLED
	List<Ref<LuauScript>> scripts;

	{
		MutexLock lock(*this->mutex.ptr());

		const SelfList<LuauScript> *item = script_list.first();

		while (item) {
			Ref<LuauScript> src = Ref<LuauScript>(item->self());
			WARN_PRINT(vformat("Reload check: %s", src->get_path()));
			if (src->is_root_script() && !src->get_path().is_empty()) {
				scripts.push_back(src);
			}
			item = item->next();
		}
	}

	scripts.sort_custom<LuauScriptDepSort>();

	for (int i = 0; i < p_scripts.size(); i++) {
		Ref<LuauScript> script = p_scripts[i];
		scripts.push_back(script);
	}
	
	HashMap<Ref<LuauScript>, HashMap<ObjectID, List<Pair<StringName, Variant>>>> to_reload;

	for (Ref<LuauScript> &script : scripts) {
		bool skip = p_scripts.has(script) || to_reload.has(script->get_base());
		if (!skip) continue;

		to_reload.insert(script, HashMap<ObjectID, List<Pair<StringName, Variant>>>());

		if (!p_soft_reload) {
			HashMap<ObjectID, List<Pair<StringName, Variant>>> &map = to_reload[script];
		}
	}
	
	for (KeyValue<Ref<LuauScript>, HashMap<ObjectID, List<Pair<StringName, Variant>>>> &E : to_reload) {
		Ref<LuauScript> scr = E.key;
		
		WARN_PRINT(vformat("Reload: %s", scr->get_path()));

		// Clear cached bytecode to force recompilation
		scr->load_stage = LuauScript::LOAD_NONE;
		scr->bytecode.clear();

		// Reload source from file
		String path = scr->get_path();
		if (!path.is_empty()) {
			scr->load_source_code(path);
		}

		scr->reload();

		//restore state if saved
		for (KeyValue<ObjectID, List<Pair<StringName, Variant>>> &F : E.value) {
			List<Pair<StringName, Variant>> &saved_state = F.value;

			Object *obj = ObjectDB::get_instance(F.key);
			if (!obj) {
				continue;
			}

			if (!p_soft_reload) {
				//clear it just in case (may be a pending reload state)
				obj->set_script(Variant());
			}
			obj->set_script(scr);

			ScriptInstance *script_inst = scr->get_instance(obj->get_instance_id());

			if (!script_inst) {
				if (!scr->pending_reload_state.has(obj->get_instance_id())) {
					scr->pending_reload_state[obj->get_instance_id()] = saved_state;
				}
				continue;
			}

			if (scr->is_placeholder_fallback_enabled()) {
				PlaceHolderScriptInstance *placeholder = static_cast<PlaceHolderScriptInstance *>(script_inst);
				for (List<Pair<StringName, Variant>>::Element *G = saved_state.front(); G; G = G->next()) {
					placeholder->property_set_fallback(G->get().first, G->get().second);
				}

			} else {
				for (List<Pair<StringName, Variant>>::Element *G = saved_state.front(); G; G = G->next()) {
					script_inst->set(G->get().first, G->get().second);
				}

			}

			scr->pending_reload_state.erase(obj->get_instance_id()); //as it reloaded, remove pending state
		}
	}

#endif // TOOLS_ENABLED
}

void LuauLanguage::_reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) {
#ifdef TOOLS_ENABLED
	Ref<LuauScript> script = p_script;
	if (script.is_null()) {
		return;
	}
	
	script->load_stage = LuauScript::LOAD_NONE;
	script->bytecode.clear();
	
	String path = script->get_path();
	if (!path.is_empty()) {
		script->load_source_code(path);
	}
	
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

//MARK: _validate
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

	CharString utf8 = p_script.utf8();
	std::string source_str(utf8.get_data(), utf8.length());
	
	// Create allocator and name table for AST
	Luau::Allocator allocator;
	Luau::AstNameTable names(allocator);
	
	// Parse the source
	Luau::ParseOptions parse_opts;
	Luau::ParseResult parse_result = Luau::Parser::parse(
		source_str.c_str(), source_str.size(), names, allocator, parse_opts);
	
	if (p_validate_errors && !parse_result.errors.empty()) {
		Array errorArray;

		Luau::ParseError error = parse_result.errors[0];
		Luau::Position errorPos = error.getLocation().begin;

		Dictionary errorEntry;
		errorEntry["path"] = p_path;
		errorEntry["line"] = errorPos.line + 1;
		errorEntry["column"] = errorPos.column + 1;
		errorEntry["message"] = String(error.getMessage().c_str());
		errorArray.push_back(errorEntry);

		ret["errors"] = errorArray;
		ret["valid"] = false;
	}

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

//MARK: Code completion helpers

// Completion option kinds, matching Godot's ScriptLanguage::CodeCompletionKind
// (see core/object/script_language.h). The numeric values MUST stay in sync with
// that enum, otherwise the editor renders options with the wrong icon or drops them.
enum LuauCompletionKind {
	LUAU_KIND_CLASS = 0, // CODE_COMPLETION_KIND_CLASS
	LUAU_KIND_FUNCTION = 1, // CODE_COMPLETION_KIND_FUNCTION
	LUAU_KIND_SIGNAL = 2, // CODE_COMPLETION_KIND_SIGNAL
	LUAU_KIND_VARIABLE = 3, // CODE_COMPLETION_KIND_VARIABLE (properties)
	LUAU_KIND_MEMBER = 4, // CODE_COMPLETION_KIND_MEMBER
	LUAU_KIND_ENUM = 5, // CODE_COMPLETION_KIND_ENUM
	LUAU_KIND_CONSTANT = 6, // CODE_COMPLETION_KIND_CONSTANT
	LUAU_KIND_PLAIN_TEXT = 9, // CODE_COMPLETION_KIND_PLAIN_TEXT (keywords/builtins)
};

static void luau_complete_add(Array &p_result, const String &p_name, int p_kind, const String &p_insert = String(), const String &p_description = String()) {
	Dictionary opt;
	opt["display"] = p_name;
	opt["insert_text"] = p_insert.is_empty() ? p_name : p_insert;
	opt["kind"] = p_kind;
	opt["location"] = 0;

	// engine required
	opt["font_color"] = Color(1, 1, 1);
	opt["icon"] = Variant();
	opt["default_value"] = Variant();

	// Optional detail shown by the editor (e.g. the snake_case remap of a
	// PascalCase method). Not read by the wrapper, so safe to include.
	if (!p_description.is_empty()) {
		opt["description"] = p_description;
	}
	p_result.push_back(opt);
}

static String luau_to_pascal(const String &p_snake) {
	String out;
	bool upper = true;
	for (int i = 0; i < p_snake.length(); i++) {
		char32_t c = p_snake[i];
		if (c == '_') {
			upper = true;
			continue;
		}
		if (upper && c >= 'a' && c <= 'z') {
			out += String::chr(c - 'a' + 'A');
			upper = false;
		} else {
			out += String::chr(c);
			upper = false;
		}
	}
	return out;
}

static void luau_complete_class(nobind::ClassDB *p_class_db, const String &p_class, bool p_pascal, Array &r_out, bool p_methods_only = false) {
	// methods
	TypedArray<Dictionary> methods = p_class_db->class_get_method_list(p_class, false);
	for (int i = 0; i < methods.size(); i++) {
		Dictionary m = methods[i];
		if (!m.has("name")) {
			continue;
		}
		String name = m["name"];
		if (name.begins_with("_")) {
			continue;
		}
		if (p_pascal) {
			luau_complete_add(r_out, luau_to_pascal(name), 1 /*KIND_FUNCTION*/, String(), name);
		} else {
			luau_complete_add(r_out, name, 1 /*KIND_FUNCTION*/);
		}
	}
	// properties (skipped for `:` member access, where only methods apply)
	if (!p_methods_only) {
	TypedArray<Dictionary> props = p_class_db->class_get_property_list(p_class, false);
	for (int i = 0; i < props.size(); i++) {
		Dictionary p = props[i];
		if (!p.has("name")) {
			continue;
		}
		String name = p["name"];
		if (name.begins_with("_")) {
			continue;
		}
		if (p_pascal) {
			luau_complete_add(r_out, luau_to_pascal(name), LUAU_KIND_VARIABLE /*KIND_PROPERTY*/, String(), name);
		} else {
			luau_complete_add(r_out, name, LUAU_KIND_VARIABLE /*KIND_PROPERTY*/);
		}
	}
	}
	// constants
	PackedStringArray consts = p_class_db->class_get_integer_constant_list(p_class, false);
	for (int i = 0; i < consts.size(); i++) {
		luau_complete_add(r_out, consts[i], LUAU_KIND_CONSTANT /*KIND_CONSTANT*/);
	}
	// enums
	PackedStringArray enums = p_class_db->class_get_enum_list(p_class, false);
	for (int i = 0; i < enums.size(); i++) {
		luau_complete_add(r_out, enums[i], LUAU_KIND_ENUM /*KIND_ENUM*/);
	}
	// signals
	TypedArray<Dictionary> sigs = p_class_db->class_get_signal_list(p_class, false);
	for (int i = 0; i < sigs.size(); i++) {
		Dictionary s = sigs[i];
		if (!s.has("name")) {
			continue;
		}
		String name = s["name"];
		if (p_pascal) {
			luau_complete_add(r_out, "On" + luau_to_pascal(name), 2 /*KIND_SIGNAL*/, String(), "on_" + name);
		} else {
			luau_complete_add(r_out, name, 2 /*KIND_SIGNAL*/);
		}
	}
}

// Collect a LuauScript's own definitions (and its base scripts).
static void luau_complete_script(Ref<LuauScript> p_script, Array &r_out) {
	LuauScript *s = p_script.ptr();
	while (s != nullptr) {
		Dictionary consts = s->_get_constants();
		Array keys = consts.keys();
		for (int i = 0; i < keys.size(); i++) {
			luau_complete_add(r_out, keys[i], LUAU_KIND_CONSTANT /*KIND_CONSTANT*/);
		}
		for (const GDClassProperty &p : s->get_definition().properties) {
			luau_complete_add(r_out, p.property.name, LUAU_KIND_VARIABLE /*KIND_PROPERTY*/);
		}
		for (const KeyValue<StringName, GDMethod> &E : s->get_definition().methods) {
			luau_complete_add(r_out, E.key, 1 /*KIND_FUNCTION*/);
		}
		s = s->get_base().ptr();
	}
}


namespace {
class LuauJsonParser {
	String src;
	int pos = 0;

	void skip_ws() {
		while (pos < src.length()) {
			char32_t c = src[pos];
			if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
				pos++;
			} else {
				break;
			}
		}
	}

	char32_t peek() {
		return pos < src.length() ? src[pos] : 0;
	}

	Variant parse_value() {
		skip_ws();
		char32_t c = peek();
		if (c == '{') {
			return parse_object();
		}
		if (c == '[') {
			return parse_array();
		}
		if (c == '"') {
			return Variant(parse_string());
		}
		if (c == 't' || c == 'f') {
			return parse_bool();
		}
		if (c == 'n') {
			parse_null();
			return Variant();
		}
		return Variant(parse_number());
	}

	String parse_string() {
		// Assumes current char is '"'.
		pos++; // consume opening quote
		String out;
		while (pos < src.length() && src[pos] != '"') {
			char32_t c = src[pos++];
			if (c == '\\') {
				char32_t e = src[pos++];
				switch (e) {
					case '"': out += '"'; break;
					case '\\': out += '\\'; break;
					case '/': out += '/'; break;
					case 'n': out += '\n'; break;
					case 't': out += '\t'; break;
					case 'r': out += '\r'; break;
					case 'b': out += '\b'; break;
					case 'f': out += '\f'; break;
					case 'u': {
						// Parse 4 hex digits into a codepoint.
						int cp = 0;
						for (int i = 0; i < 4 && pos < src.length(); i++) {
							char32_t h = src[pos++];
							cp <<= 4;
							if (h >= '0' && h <= '9') {
								cp |= (h - '0');
							} else if (h >= 'a' && h <= 'f') {
								cp |= (h - 'a' + 10);
							} else if (h >= 'A' && h <= 'F') {
								cp |= (h - 'A' + 10);
							}
						}
						out += String::chr(cp);
					} break;
					default: out += e; break;
				}
			} else {
				out += c;
			}
		}
		if (pos < src.length()) {
			pos++; // consume closing quote
		}
		return out;
	}

	Dictionary parse_object() {
		pos++; // consume '{'
		Dictionary d;
		skip_ws();
		if (peek() == '}') {
			pos++;
			return d;
		}
		while (true) {
			skip_ws();
			String key = parse_string();
			skip_ws();
			if (peek() == ':') {
				pos++;
			}
			Variant val = parse_value();
			d[key] = val;
			skip_ws();
			char32_t c = peek();
			if (c == ',') {
				pos++;
			} else if (c == '}') {
				pos++;
				break;
			} else {
				break;
			}
		}
		return d;
	}

	Array parse_array() {
		pos++; // consume '['
		Array a;
		skip_ws();
		if (peek() == ']') {
			pos++;
			return a;
		}
		while (true) {
			a.push_back(parse_value());
			skip_ws();
			char32_t c = peek();
			if (c == ',') {
				pos++;
			} else if (c == ']') {
				pos++;
				break;
			} else {
				break;
			}
		}
		return a;
	}

	double parse_number() {
		int start = pos;
		while (pos < src.length()) {
			char32_t c = src[pos];
			if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') {
				pos++;
			} else {
				break;
			}
		}
		String num = src.substr(start, pos - start);
		return num.to_float();
	}

	bool parse_bool() {
		if (src.substr(pos, 4) == "true") {
			pos += 4;
			return true;
		}
		pos += 5; // "false"
		return false;
	}

	void parse_null() {
		pos += 4; // "null"
	}

public:
	static Variant parse(const String &p_src) {
		LuauJsonParser p;
		p.src = p_src;
		p.skip_ws();
		return p.parse_value();
	}
};

// Cached map: built-in type name -> its parsed variant record (Dictionary).
static HashMap<String, Dictionary> *g_builtin_variants = nullptr;

const HashMap<String, Dictionary> &luau_get_builtin_variants() {
	if (g_builtin_variants == nullptr) {
		g_builtin_variants = new HashMap<String, Dictionary>();
		Ref<FileAccess> file = FileAccess::open("res://bin/LuauGDExt/variants.json", FileAccess::ModeFlags::READ);
		if (file.is_valid()) {
			String text = file->get_as_text();
			Variant root = LuauJsonParser::parse(text);
			if (root.get_type() == Variant::DICTIONARY) {
				Dictionary top = root;
				if (top.has("variants")) {
					Array variants = top["variants"];
					for (int i = 0; i < variants.size(); i++) {
						Dictionary v = variants[i];
						if (v.has("name")) {
							(*g_builtin_variants)[String(v["name"])] = v;
						}
					}
				}
			}
		}
	}
	return *g_builtin_variants;
}

} // namespace

static void luau_complete_builtin_type(const String &p_type, bool p_pascal, Array &r_out, bool p_methods_only = false, bool p_show_constants = true) {
	const HashMap<String, Dictionary> &variants = luau_get_builtin_variants();
	const Dictionary *v = variants.getptr(p_type);
	if (v == nullptr) {
		return;
	}

	// methods
	if ((*v).has("methods")) {
		Array methods = (*v)["methods"];
		for (int i = 0; i < methods.size(); i++) {
			Dictionary m = methods[i];
			if (!m.has("name")) {
				continue;
			}
			String name = m["name"];
			if (name.begins_with("_")) {
				continue;
			}
			if (p_pascal) {
				luau_complete_add(r_out, luau_to_pascal(name), 1 /*KIND_FUNCTION*/, String(), name);
			} else {
				luau_complete_add(r_out, name, 1 /*KIND_FUNCTION*/);
			}
		}
	}
	// properties (skipped for `:` member access, where only methods apply)
	if (!p_methods_only && (*v).has("properties")) {
		Array props = (*v)["properties"];
		for (int i = 0; i < props.size(); i++) {
			Dictionary p = props[i];
			if (!p.has("name")) {
				continue;
			}
			String name = p["name"];
			if (name.begins_with("_")) {
				continue;
			}
			if (p_pascal) {
				luau_complete_add(r_out, luau_to_pascal(name), LUAU_KIND_VARIABLE /*KIND_PROPERTY*/, String(), name);
			} else {
				luau_complete_add(r_out, name, LUAU_KIND_VARIABLE /*KIND_PROPERTY*/);
			}
		}
	}
	// constants (only when accessing the type directly, e.g. Vector3.UP;
	// not on an instance such as moveDir: or moveDir.X)
	if (p_show_constants && (*v).has("constants")) {
		Array consts = (*v)["constants"];
		for (int i = 0; i < consts.size(); i++) {
			Dictionary c = consts[i];
			if (!c.has("name")) {
				continue;
			}
			luau_complete_add(r_out, c["name"], LUAU_KIND_CONSTANT /*KIND_CONSTANT*/);
		}
	}
}

static HashMap<String, String> luau_collect_local_types(const String &p_code, int p_caret) {
	HashMap<String, String> locals;

	String scope = p_code.substr(0, p_caret);
	// Split into lines, keeping it simple: scan each line for `name: Type`.
	int line_start = 0;
	for (int i = 0; i <= scope.length(); i++) {
		if (i == scope.length() || scope[i] == '\n') {
			String line = scope.substr(line_start, i - line_start);
			line_start = i + 1;

			// Strip a leading `local `/`local` keyword if present.
			String l = line.strip_edges();
			if (l.begins_with("local ")) {
				l = l.substr(6).strip_edges();
			}

			int colon = l.find(":");
			if (colon <= 0) {
				continue;
			}
			String name = l.substr(0, colon).strip_edges();
			// Name must be a valid identifier (no spaces, no operators).
			bool name_ok = !name.is_empty();
			for (int k = 0; name_ok && k < name.length(); k++) {
				char32_t c = name[k];
				if (!(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
					name_ok = false;
				}
			}
			if (!name_ok) {
				continue;
			}

			String rest = l.substr(colon + 1).strip_edges();
			// Type is the first identifier in `rest` (stop at '=', whitespace,
			// '(', etc.).
			int type_end = 0;
			while (type_end < rest.length()) {
				char32_t c = rest[type_end];
				if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
					type_end++;
				} else {
					break;
				}
			}
			if (type_end == 0) {
				continue;
			}
			String type_name = rest.substr(0, type_end);
			locals[name] = type_name;
		}
	}

	return locals;
}

Dictionary LuauLanguage::_complete_code(const String &p_code, const String &p_path, Object *p_owner) const {
	Dictionary ret;
	Array result;

	nobind::ClassDB *class_db = nobind::ClassDB::get_singleton();

	// Resolve the script for context (owner or path).
	Ref<LuauScript> luau_script;
	if (p_owner != nullptr) {
		Ref<Script> script = p_owner->get_script();
		if (script.is_valid()) {
			luau_script = script;
		}
	}
	if (luau_script.is_null() && !p_path.is_empty()) {
		Ref<Script> script = ResourceLoader::get_singleton()->load(p_path);
		if (script.is_valid()) {
			luau_script = script;
		}
	}

	String extends = "RefCounted";
	if (luau_script.is_valid()) {
		extends = luau_script->get_definition().extends;
	}

	// cursor = 0xFFFF caret marker
	String code = p_code;
	if (code.is_empty() && luau_script.is_valid()) {
		code = luau_script->_get_source_code();
	}

	String current_line = code;
	int caret = code.find(String::chr(0xFFFF));
	if (caret != -1) {
		// Text from the start of the caret's line up to the caret.
		int line_start = code.rfind("\n", caret);
		line_start = (line_start == -1) ? 0 : line_start + 1;
		current_line = code.substr(line_start, caret - line_start);
	} else {
		int last_nl = code.rfind("\n");
		if (last_nl != -1) {
			current_line = code.substr(last_nl + 1);
		}
	}

	String trimmed = current_line.strip_edges();

	// @extends / @class context: suggest class names.
	if (trimmed.begins_with("@extends") || trimmed.begins_with("@class")) {
		PackedStringArray classes = class_db->get_class_list();
		for (int i = 0; i < classes.size(); i++) {
			luau_complete_add(result, classes[i], 0 /*KIND_CLASS*/);
		}
		ret["options"] = result;
		ret["force"] = false;
		ret["call_hint"] = String();
		ret["result"] = OK;
		return ret;
	}


	String prefix;
	String receiver;
	bool dot = false;
	bool colon = false;

	
	int prefix_end = current_line.length();
	while (prefix_end > 0) {
		char32_t c = current_line[prefix_end - 1];
		if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
			prefix_end--;
		} else {
			break;
		}
	}
	prefix = current_line.substr(prefix_end);

	
	int recv_start = -1;
	if (prefix_end > 0) {
		char32_t sep = current_line[prefix_end - 1];
		if (sep == '.' || sep == ':') {
			colon = (sep == ':');
			int recv_end = prefix_end - 1;
			recv_start = recv_end;
			while (recv_start > 0) {
				char32_t c = current_line[recv_start - 1];
				if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
					recv_start--;
				} else {
					break;
				}
			}
			if (recv_start < recv_end) {
				dot = true;
				receiver = current_line.substr(recv_start, recv_end - recv_start);
			}
		}
	}


	bool want_pascal = false;
	for (int i = 0; i < prefix.length(); i++) {
		if (prefix[i] >= 'A' && prefix[i] <= 'Z') {
			want_pascal = true;
			break;
		}
	}

	bool receiver_is_class = class_db->class_exists(receiver) || class_db->class_get_method_list(receiver, false).size() > 0;
	bool known_receiver = (receiver == "self" || receiver == "this" || receiver_is_class);

	HashMap<String, String> local_types = luau_collect_local_types(code, caret);
	String receiver_type;
	if (!known_receiver && local_types.has(receiver)) {
		receiver_type = local_types[receiver];
		bool type_is_class = !receiver_type.is_empty() &&
				(luau_get_builtin_variants().has(receiver_type) ||
						class_db->class_exists(receiver_type) ||
						class_db->class_get_method_list(receiver_type, false).size() > 0);
		if (type_is_class) {
			known_receiver = true;
			receiver_is_class = true;
		}
	}

	if (dot && known_receiver && receiver_is_class) {
		want_pascal = true;
	}

	if (dot && known_receiver) {
		if (receiver == "self" || receiver == "this") {
			if (luau_script.is_valid()) {
				luau_complete_script(luau_script, result);
			}
			if (!extends.begins_with("res://") && class_db->class_exists(extends)) {
				luau_complete_class(class_db, extends, want_pascal, result, colon);
			}
		} else if (!receiver_type.is_empty()) {
			if (class_db->class_exists(receiver_type)) {
				luau_complete_class(class_db, receiver_type, want_pascal, result, colon);
			} else {
				// Constants are type-level (Vector3.UP), not instance-level
				// (moveDir: / moveDir.X), so only show them when the receiver
				// is the type name itself.
				bool show_constants = luau_get_builtin_variants().has(receiver);
				luau_complete_builtin_type(receiver_type, want_pascal, result, colon, show_constants);
			}
		} else {
			luau_complete_class(class_db, receiver, want_pascal, result, colon);
		}
	}

	//MARK: Enum auto complete
	if (dot) {
		const HashMap<String, HashMap<String, int64_t>> &global_enums = LuauEngine::get_global_enums();

		if (receiver == "Enum") {
			for (const KeyValue<String, HashMap<String, int64_t>> &E : global_enums) {
				luau_complete_add(result, E.key, LUAU_KIND_ENUM);
			}
		} else if (current_line.substr(0, recv_start).ends_with("Enum.")) {
			const HashMap<String, int64_t> *values = global_enums.getptr(receiver);
			if (values) {
				for (const KeyValue<String, int64_t> &V : *values) {
					luau_complete_add(result, V.key, LUAU_KIND_CONSTANT);
				}
			}
		}
	}

	if (!dot) {
		PackedStringArray keywords = _get_reserved_words();
		for (int i = 0; i < keywords.size(); i++) {
			luau_complete_add(result, keywords[i], LUAU_KIND_PLAIN_TEXT /*KIND_PLAIN_TEXT*/);
		}

		if (luau_script.is_valid()) {
			luau_complete_script(luau_script, result);
		}

		for (const KeyValue<StringName, Variant> &E : global_constants) {
			luau_complete_add(result, E.key, LUAU_KIND_CONSTANT /*KIND_CONSTANT*/);
		}

		static const char *builtins[] = {
			"type", "tostring", "tonumber", "pairs", "ipairs", "next", "select",
			"unpack", "rawequal", "rawget", "rawset", "rawlen", "setmetatable",
			"getmetatable", "pcall", "xpcall", "error", "assert", "warn", "print",
			"string", "table", "math", "task", "coroutine", "vector", "buffer",
			"os", "utf8", "bit32", "debug", "typeof", "wait", "Enum", nullptr
		};
		for (int i = 0; builtins[i] != nullptr; i++) {
			luau_complete_add(result, String(builtins[i]), LUAU_KIND_PLAIN_TEXT /*KIND_GLOBAL*/);
		}

		if (!extends.begins_with("res://") && class_db->class_exists(extends)) {
			// Luau uses PascalCase for methods/properties, so complete the
			// base class members in PascalCase (e.g. MoveAndSlide, not
			// move_and_slide) even when the typed prefix is lowercase.
			luau_complete_class(class_db, extends, true, result);
		}
	}

	// Deduplicate by display name.
	{
		Array deduped;
		HashSet<String> seen;
		for (int i = 0; i < result.size(); i++) {
			Dictionary opt = result[i];
			String name = opt["display"];
			if (seen.has(name)) {
				continue;
			}
			seen.insert(name);
			deduped.push_back(opt);
		}
		result = deduped;
	}

	// Filter by the typed prefix (case-insensitive).
	if (!prefix.is_empty()) {
		Array filtered;
		String lp = prefix.to_lower();
		for (int i = 0; i < result.size(); i++) {
			Dictionary opt = result[i];
			String name = opt["display"];
			if (name.to_lower().begins_with(lp)) {
				filtered.push_back(opt);
			}
		}
		result = filtered;
		}
	
		ret["options"] = result;
		ret["force"] = false;
		ret["call_hint"] = String();
		ret["result"] = OK;
		return ret;
	}
	
//MARK: Code lookup helpers
static StringName luau_remap_symbol(const String &p_symbol) {
	String remapped = p_symbol;
	if (remapped.begins_with("On") && remapped.length() > 2) {
		char32_t c = remapped[2];
		if (c >= 'A' && c <= 'Z') {
			remapped = remapped.substr(2);
		}
	}

	String godot_key;
	for (int i = 0; i < remapped.length(); i++) {
		char32_t c = remapped[i];
		if (c >= 'A' && c <= 'Z') {
			if (i > 0) {
				godot_key += "_";
			}
			char lower[2] = { (char)(c - 'A' + 'a'), '\0' };
			godot_key += lower;
		} else {
			char ch[2] = { (char)c, '\0' };
			godot_key += ch;
		}
	}
	return StringName(godot_key);
}

static String luau_function_name(Luau::AstStat *p_stat) {
	if (auto *func = p_stat->as<Luau::AstStatFunction>()) {
		if (!func->name) {
			return String();
		}
		if (auto *index = func->name->as<Luau::AstExprIndexName>()) {
			return String(index->index.value);
		} else if (auto *local = func->name->as<Luau::AstExprLocal>()) {
			return String(local->local->name.value);
		} else if (auto *global = func->name->as<Luau::AstExprGlobal>()) {
			return String(global->name.value);
		}
	} else if (auto *local_func = p_stat->as<Luau::AstStatLocalFunction>()) {
		if (local_func->name) {
			return String(local_func->name->name.value);
		}
	}
	return String();
}


struct LuauSymbolDef {
	int line = -1; // 1-based line, or -1 if not found
	int type = 1;  // CodeCompletionType: 1 TYPE_FUNCTION, 2 TYPE_MEMBER, 3 TYPE_CONSTANT
};

static LuauSymbolDef luau_find_symbol(const String &p_source, const String &p_name) {
	LuauSymbolDef result;
	if (p_source.is_empty() || p_name.is_empty()) {
		return result;
	}

	CharString utf8 = p_source.utf8();
	std::string source_str(utf8.get_data(), utf8.length());

	Luau::Allocator allocator;
	Luau::AstNameTable names(allocator);
	Luau::ParseOptions parse_opts;
	Luau::ParseResult parse_result = Luau::Parser::parse(
			source_str.c_str(), source_str.size(), names, allocator, parse_opts);

	if (!parse_result.root) {
		return result;
	}

	for (Luau::AstStat *stat : parse_result.root->body) {
		if (auto *func = stat->as<Luau::AstStatFunction>()) {
			String name = luau_function_name(stat);
			if (!name.is_empty() && name == p_name) {
				result.line = stat->location.begin.line + 1;
				result.type = 1; // TYPE_FUNCTION
				return result;
			}
		} else if (auto *local_func = stat->as<Luau::AstStatLocalFunction>()) {
			String name = luau_function_name(stat);
			if (!name.is_empty() && name == p_name) {
				result.line = stat->location.begin.line + 1;
				result.type = 1; // TYPE_FUNCTION
				return result;
			}
		} else if (auto *assign = stat->as<Luau::AstStatAssign>()) {
			for (size_t i = 0; i < assign->vars.size; i++) {
				auto *global = assign->vars.data[i]->as<Luau::AstExprGlobal>();
				if (!global) {
					continue;
				}
				String var_name = String(global->name.value);
				if (var_name == p_name) {
					result.line = stat->location.begin.line + 1;
					bool is_constant = true;
					for (int j = 0; j < var_name.length(); j++) {
						char32_t c = var_name[j];
						if (c >= 'a' && c <= 'z') {
							is_constant = false;
							break;
						}
					}
					result.type = is_constant ? 3 : 2; // TYPE_CONSTANT : TYPE_MEMBER
					return result;
				}
			}
		}
	}

	return result;
}

Dictionary LuauLanguage::_lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner) const {
	Dictionary ret;

	// ScriptLanguage::LookupResultType
	//   0 SCRIPT_LOCATION, 1 CLASS, 2 CLASS_CONSTANT, 3 CLASS_ENUM,
	//   4 CLASS_METHOD, 5 CLASS_SIGNAL, 6 CLASS_PROPERTY, 7 CLASS_ANNOTATION
	// ScriptLanguage::CodeCompletionType
	//   0 TYPE_CLASS, 1 TYPE_FUNCTION, 2 TYPE_MEMBER, 3 TYPE_CONSTANT,
	//   4 TYPE_ENUM, 5 TYPE_SIGNAL, 6 TYPE_ANNOTATION
	ret["result"] = 0; // default: no result
	ret["type"] = 0;
	ret["is_deprecated"] = false;

	String symbol = p_symbol.strip_edges();
	if (symbol.is_empty()) {
		ret["result"] = FAILED;
		return ret;
	}

	nobind::ClassDB *class_db = nobind::ClassDB::get_singleton();

	if (symbol.contains(".")) {
		PackedStringArray parts = symbol.split(".");
		if (parts.size() == 2) {
			String class_name = parts[0];
			String member_name = parts[1];
			StringName remapped_member = luau_remap_symbol(member_name);

			if (class_db->class_exists(class_name)) {
				if (class_db->class_has_method(class_name, member_name, false) ||
						class_db->class_has_method(class_name, remapped_member, false)) {
					ret["result"] = OK;
					ret["type"] = 4;   // LOOKUP_RESULT_CLASS_METHOD
					ret["class_name"] = class_name;
					ret["class_member"] = member_name;
					ret["description"] = String("Method of class ") + class_name;
					return ret;
				}

				TypedArray<Dictionary> property_list = class_db->class_get_property_list(class_name, false);
				for (int i = 0; i < property_list.size(); i++) {
					Dictionary prop = property_list[i];
					if (prop.has("name")) {
						String pname = prop["name"];
						if (pname == member_name || pname == remapped_member) {
							ret["result"] = OK;
							ret["type"] = 3;   // LOOKUP_RESULT_CLASS_PROPERTY
							ret["class_name"] = class_name;
							ret["class_member"] = member_name;
							ret["description"] = String("Property of class ") + class_name;
							return ret;
						}
					}
				}

				if (class_db->class_has_integer_constant(class_name, member_name) ||
						class_db->class_has_integer_constant(class_name, remapped_member)) {
					ret["result"] = OK;
					ret["type"] = 2;   // LOOKUP_RESULT_CLASS_CONSTANT
					ret["class_name"] = class_name;
					ret["class_member"] = member_name;
					ret["description"] = String("Constant of class ") + class_name;
					return ret;
				}

				if (class_db->class_has_enum(class_name, member_name, false) ||
						class_db->class_has_enum(class_name, remapped_member, false)) {
					ret["result"] = OK;
					ret["type"] = 6;   // LOOKUP_RESULT_CLASS_ENUM
					ret["class_name"] = class_name;
					ret["class_member"] = member_name;
					ret["description"] = String("Enum of class ") + class_name;
					return ret;
				}
			}
		}
		// Not a resolvable qualified name; fall through to bare-symbol handling.
	}

	Ref<LuauScript> luau_script;
	if (p_owner != nullptr) {
		Ref<Script> script = p_owner->get_script();
		if (script.is_valid()) {
			luau_script = script;
		}
	}
	if (luau_script.is_null() && !p_path.is_empty()) {
		Ref<Script> script = ResourceLoader::get_singleton()->load(p_path);
		if (script.is_valid()) {
			luau_script = script;
		}
	}

	if (luau_script.is_valid()) {
		LuauSymbolDef def = luau_find_symbol(p_code, symbol);
		if (def.line > 0) {
			ret["result"] = OK;
			ret["type"] = 0;   // LOOKUP_RESULT_SCRIPT_LOCATION
			ret["class_name"] = luau_script->get_definition().name;
			ret["class_member"] = symbol;
			ret["class_path"] = luau_script->get_path();
			ret["location"] = def.line;
			ret["description"] = String("Definition in script: ") + symbol;
			return ret;
		}

		LuauScript *base = luau_script->get_base().ptr();
		while (base != nullptr) {
			LuauSymbolDef bdef = luau_find_symbol(base->_get_source_code(), symbol);
			if (bdef.line > 0) {
				ret["result"] = OK;
				ret["type"] = 0;   // LOOKUP_RESULT_SCRIPT_LOCATION
				ret["class_name"] = base->get_definition().name;
				ret["class_member"] = symbol;
				ret["class_path"] = base->get_path();
				ret["location"] = bdef.line;
				ret["description"] = String("Inherited definition from: ") + base->get_definition().name;
				return ret;
			}
			base = base->get_base().ptr();
		}

		String extends = luau_script->get_definition().extends;
		if (!extends.begins_with("res://") && class_db->class_exists(extends)) {
			StringName remapped = luau_remap_symbol(symbol);

			if (class_db->class_has_method(extends, symbol, false) ||
					class_db->class_has_method(extends, remapped, false)) {
				ret["result"] = OK;
				ret["type"] = 4;   // LOOKUP_RESULT_CLASS_METHOD
				ret["class_name"] = extends;
				ret["class_member"] = symbol;
				ret["description"] = String("Method of class ") + extends;
				return ret;
			}

			TypedArray<Dictionary> property_list = class_db->class_get_property_list(extends, false);
			for (int i = 0; i < property_list.size(); i++) {
				Dictionary prop = property_list[i];
				if (prop.has("name")) {
					String pname = prop["name"];
					if (pname == symbol || pname == remapped) {
						ret["result"] = OK;
						ret["type"] = 3;   // LOOKUP_RESULT_CLASS_PROPERTY
						ret["class_name"] = extends;
						ret["class_member"] = symbol;
						ret["description"] = String("Property of class ") + extends;
						return ret;
					}
				}
			}

			if (class_db->class_has_integer_constant(extends, symbol) ||
					class_db->class_has_integer_constant(extends, remapped)) {
				ret["result"] = OK;
				ret["type"] = 2;   // LOOKUP_RESULT_CLASS_CONSTANT
				ret["class_name"] = extends;
				ret["class_member"] = symbol;
				ret["description"] = String("Constant of class ") + extends;
				return ret;
			}

			if (class_db->class_has_enum(extends, symbol, false) ||
					class_db->class_has_enum(extends, remapped, false)) {
				ret["result"] = OK;
				ret["type"] = 6;   // LOOKUP_RESULT_CLASS_ENUM
				ret["class_name"] = extends;
				ret["class_member"] = symbol;
				ret["description"] = String("Enum of class ") + extends;
				return ret;
			}
		}
	}

	if (class_db->class_exists(symbol)) {
		ret["result"] = OK;
		ret["type"] = 1;   // LOOKUP_RESULT_CLASS
		ret["class_name"] = symbol;
		ret["class_path"] = String(); // built-in class, no path
		ret["description"] = String("Godot built-in class: ") + symbol;
		return ret;
	}

	if (global_constants.has(symbol)) {
		Variant value = global_constants[symbol];
		ret["result"] = OK;
		ret["type"] = 2;   // LOOKUP_RESULT_CLASS_CONSTANT
		ret["class_name"] = "@GlobalScope";
		ret["class_member"] = symbol;
		ret["description"] = String("Global constant: ") + symbol + " = " + String(value);
		return ret;
	}

	PackedStringArray keywords = _get_reserved_words();
	if (keywords.has(symbol)) {
		ret["result"] = OK;
		ret["type"] = 8;   // LOOKUP_RESULT_CLASS_ANNOTATION
		ret["class_name"] = "Luau";
		ret["class_member"] = symbol;
		ret["description"] = String("Luau keyword: ") + symbol;
		return ret;
	}

	ret["result"] = FAILED;
	ret["type"] = 0;   // LOOKUP_RESULT_SCRIPT_LOCATION (unused on failure)
	ret["description"] = String("Unknown symbol: ") + symbol;
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





