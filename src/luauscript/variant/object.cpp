
#include "object.h"

#include "luauscript/luau_script.h"
#include "luauscript/lamda_wrapper.h"
#include "luauscript/luau_reentry_guard.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

HashMap<uint64_t, LuauObject*> LuauObject::list;

template<>
const char* VariantBridge<Object*>::variant_name("Object");

const luaL_Reg ObjectBridge::static_library[] = {
	{NULL, NULL}
};

void ObjectBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    // CONSTANTS
    LuauBridge::push_variant(L, 0);
    lua_setfield(L, -2, "NOTIFICATION_POSTINITIALIZE");
    
    LuauBridge::push_variant(L, 1);
    lua_setfield(L, -2, "NOTIFICATION_PREDELETE");
    
    LuauBridge::push_variant(L, 2);
    lua_setfield(L, -2, "NOTIFICATION_EXTENSION_RELOADED");

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Object*>::on_index(lua_State* L, Object* const &object, const char* key) {
    LuauObject* luau_object = LuauObject::get_luau_object(object);
    LuauScriptInstance* instance = (luau_object != nullptr) ? luau_object->instance : nullptr;

    int arg_count = lua_gettop(L); // 2: GDV, key

    bool script_function = false;

    if (instance != nullptr) {
        //non-luau objects
        Variant ret;
        LuauScriptInstance::PropertySetGetError err_code;
        bool is_function = false;
        bool success = instance->get_with_kind(StringName(key), ret, &err_code, is_function);

        script_function = success && is_function;

        if (success && !is_function) {
            LuauBridge::push_variant(L, ret);
            return 1;
        }
    }


    StringName prop_name = godot::resolve_prop_name(L, key);
    StringName method_name = script_function ? StringName(key) : prop_name;

    bool is_method = script_function ||
        nobind::ClassDB::get_singleton()->class_has_method(
            object->get_class(),
            prop_name,
            false
        );

    if (!is_method && !script_function) {
        is_method = object->has_method(prop_name);
    }

    if (is_method) {
        // method_call
        LuauBridge::push_uint64_t(L, object->get_instance_id());
        CharString key_cs = String(method_name).utf8();
        lua_pushstring(L, key_cs.get_data());
        lua_pushlightuserdata(L, instance);
        lua_pushcclosure(L, [](lua_State *L) -> int {
            uint64_t obj_id = LuauBridge::get_uint64_t(L, lua_upvalueindex(1));
            const char *method_name = lua_tostring(L, lua_upvalueindex(2));
            LuauScriptInstance* inst = (LuauScriptInstance*)lua_touserdata(L, lua_upvalueindex(3));

            if (method_name == nullptr) {
                WARN_PRINT(("method_call method_name=nil"));
                return 0;
            }

            int arg_count = lua_gettop(L);
            Array args;
            int start_idx = 1;
            
            if (arg_count > 0) {
                bool is_receiver = false;
                if (inst && lua_istable(L, 1)) {
                    lua_getref(L, inst->get_self_ref());
                    is_receiver = lua_rawequal(L, 1, -1) != 0;
                    lua_pop(L, 1);
                } else if (void *recv_ud = LuauBridge::luaL_testudata(L, 1, "Object")) {
                    is_receiver = (*(uint64_t *)recv_ud == obj_id);
                }
                if (is_receiver) {
                    start_idx = 2;
                }
            }
            for (int i = start_idx; i <= arg_count; i++) {
                args.append(LuauBridge::get_variant(L, i));
            }

            if (inst && !inst->is_ready) {
                // Create an OnReadyWrapper proxy that will be populated at _ready.
                void** proxy = (void**)lua_newuserdata(L, sizeof(void*));
                *proxy = nullptr;
                luaL_getmetatable(L, "OnReadyWrapper");
                lua_setmetatable(L, -2);

                // Anchor the proxy in the self table so it stays alive until the instance dies.
                lua_getref(L, inst->get_self_ref()); // self
                lua_getfield(L, -1, "__onready_proxies"); // self, proxies
                if (lua_isnil(L, -1)) {
                    lua_pop(L, 1);
                    lua_newtable(L); // self, proxies
                    lua_pushvalue(L, -1); // self, proxies, proxies
                    lua_setfield(L, -3, "__onready_proxies"); // self, proxies
                }
                lua_pushvalue(L, -3); // self, proxies, proxy
                lua_pushboolean(L, true); // self, proxies, proxy, true
                lua_settable(L, -3); // self, proxies[proxy] = true
                lua_pop(L, 2); // pop proxies, self

                String method_name_str = String(method_name);
                Ref<LambdaWrapper> wrapper = memnew(LambdaWrapper);
                wrapper->set_function([obj_id, method_name_str, args, proxy]() {
                    Object* obj = ObjectDB::get_instance(obj_id);
                    if (obj == nullptr) return;

                    Variant result = obj->callv(StringName(method_name_str), args);
                    Variant* heap_result = memnew(Variant(result));
                    *proxy = (void*)heap_result;
                });
                
                inst->on_ready_wrappers.append(wrapper);
                inst->on_ready_funcs.append(godot::Callable(wrapper.ptr(), "execute"));

                return 1; // the proxy userdata
            }

            Object* obj = ObjectDB::get_instance(obj_id);

            if (obj == nullptr) {
                WARN_PRINT(("method_call obj=nil"));
                return 0;
            }

            Variant result = obj->callv(StringName(method_name), args);


            String overflow_msg;
            if (LuauReentryGuard::peek_overflow_message(&overflow_msg)) {
                luaL_error(L, "%s", overflow_msg.utf8().get_data());
            }


            LuauBridge::push_variant(L, result);
            return 1;
        }, "method_call", 3);

        return 1;
    }

    // Get object member
    Variant value = object->get(prop_name);
    if (value.get_type() != Variant::NIL) {
        LuauBridge::push_variant(L, value);
        return 1;
    }

    return 0;
}

template<>
int VariantBridge<Object*>::on_newindex(lua_State* L, Object* const &object, const char* key) {
    // Stack: object_userdata, key, value
    Variant value = LuauBridge::get_variant(L, 3);

    LuauObject* luau_object = LuauObject::get_luau_object(object);
    if (luau_object != nullptr && luau_object->instance != nullptr) {
        if (luau_object->instance->set(StringName(key), value)) {
            return 0;
        }
    }

    StringName prop_name = godot::resolve_prop_name(L, key);
    if (nobind::ClassDB::get_singleton()->class_set_property(object, prop_name, value) == OK) {
        return 0;
    }

    WARN_PRINT(vformat("Object.%s could not be set (no script variable or native property)", key));
    return 0;
}

template<>
int VariantBridge<Object*>::on_call(lua_State* L, bool& is_valid) {
    //No constructors
    return 1;
}