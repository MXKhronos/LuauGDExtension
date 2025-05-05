#include "luau_script.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/self_list.hpp>
#include <godot_cpp/templates/pair.hpp>

#include "nobind.h"
#include "luau_engine.h"
#include "luau_cache.h"
#include "luau_constants.h"
#include "luauscript_resource_format.h"

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
    // const LuauScript *s = script.ptr();

    // while (s) {
    //     StringName actual_name = p_method;

    //     // check name given and name converted to pascal
    //     // (e.g. if Node::_ready is called -> _Ready)
    //     if (s->has_method(p_method, &actual_name)) {
    //         const GDMethod &method = s->get_definition().methods[actual_name];

    //         // Check argument count
    //         int args_allowed = method.arguments.size();
    //         int args_default = method.default_arguments.size();
    //         int args_required = args_allowed - args_default;

    //         if (p_argument_count < args_required) {
    //             r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
    //             r_error->argument = args_required;

    //             return;
    //         }

    //         if (p_argument_count > args_allowed) {
    //             r_error->error = GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS;
    //             r_error->argument = args_allowed;

    //             return;
    //         }

    //         // Prepare for call
    //         lua_State *ET = lua_newthread(T); // execution thread

    //         for (int i = 0; i < p_argument_count; i++) {
    //             const Variant &arg = *p_args[i];

    //             if (!(method.arguments[i].usage & PROPERTY_USAGE_NIL_IS_VARIANT) &&
    //                     !Utils::variant_types_compatible(arg.get_type(), Variant::Type(method.arguments[i].type))) {
    //                 r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
    //                 r_error->argument = i;
    //                 r_error->expected = method.arguments[i].type;

    //                 lua_pop(T, 1); // thread
    //                 return;
    //             }

    //             LuaStackOp<Variant>::push(ET, arg);
    //         }

    //         for (int i = p_argument_count - args_required; i < args_default; i++)
    //             LuaStackOp<Variant>::push(ET, method.default_arguments[i]);

    //         // Call
    //         r_error->error = GDEXTENSION_CALL_OK;

    //         int status = call_internal(actual_name, ET, args_allowed, 1);

    //         if (status == LUA_OK) {
    //             *r_return = LuaStackOp<Variant>::get(ET, -1);
    //         } else if (status == LUA_YIELD) {
    //             if (method.return_val.type != GDEXTENSION_VARIANT_TYPE_NIL) {
    //                 lua_pop(T, 1); // thread
    //                 ERR_FAIL_MSG("Non-void method yielded unexpectedly");
    //             }

    //             *r_return = Variant();
    //         }

    //         lua_pop(T, 1); // thread
    //         return;
    //     }

    //     s = s->base.ptr();
    // }

    // r_error->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
}

void LuauScriptInstance::notification(int32_t p_what) {
#define NOTIF_NAME "_Notification"

    // These notifications will fire at program exit; see ~LuauScriptInstance
    // 3: NOTIFICATION_PREDELETE_CLEANUP (not bound)
    // if ((p_what == Object::NOTIFICATION_PREDELETE || p_what == 3) && !LuauEngine::get_singleton()) {
    //     return;
    // }

    // const LuauScript *s = script.ptr();

    // while (s) {
    //     if (s->methods.has(NOTIF_NAME)) {
    //         lua_State *ET = lua_newthread(T);

    //         LuaStackOp<int32_t>::push(ET, p_what);
    //         call_internal(NOTIF_NAME, ET, 1, 0);

    //         lua_pop(T, 1); // thread
    //     }

    //     s = s->base.ptr();
    // }
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
	
	return false;
}

bool LuauScriptInstance::get(const StringName &p_name, Variant &r_ret, PropertySetGetError *r_err) {
    return false;
}

GDExtensionPropertyInfo *LuauScriptInstance::get_property_list(uint32_t *r_count) {
    
	GDExtensionPropertyInfo *list = (GDExtensionPropertyInfo *)memalloc(sizeof(GDExtensionPropertyInfo) * 0);
	memcpy(list, 0, sizeof(GDExtensionPropertyInfo) * 0);
	
	return list;
}

Variant::Type godot::LuauScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {

    return Variant::NIL;
}

bool godot::LuauScriptInstance::has_method(const StringName &p_name) const {
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
}

Error LuauScript::_reload(bool p_keep_state) {
	// if (_is_module)
	// 	return OK;

	{
		MutexLock lock(*LuauLanguage::singleton->mutex.ptr());
		ERR_FAIL_COND_V(!p_keep_state && instances.size() > 0, ERR_ALREADY_IN_USE);
	}

	return load(LOAD_FULL, true);
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
	return true; //MARK: TODO
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
    Error err = OK;
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
	LuauEngine::VMType type = LuauEngine::VM_USER;
	
	LuauScriptInstance *internal = memnew( LuauScriptInstance(Ref<Script>(this), p_for_object, type) );

    return nullptr;
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
	
	for (Ref<LuauScript> &script : scripts) {
		//script->unload_module();
	}

	for (Ref<LuauScript> &script : scripts) {
		script->load_source_code(script->get_path());
		script->_reload(true);
	}
#endif // TOOLS_ENABLED
}

void LuauLanguage::_reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) {
#ifdef TOOLS_ENABLED

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
	return Dictionary();
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
}

LuauLanguage::~LuauLanguage() {
	singleton = nullptr;
}
