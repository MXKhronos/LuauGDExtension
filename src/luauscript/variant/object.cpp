
#include "object.h"

#include "luauscript/luau_script.h"

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
    LuauScriptInstance* instance = luau_object->instance;

    int arg_count = lua_gettop(L); // 2: GDV, key

    // if (strcmp(key, "_notification") != 0) {
        WARN_PRINT(vformat("on_index #(%s) %s.%s", arg_count, object, key));
    // }

    Variant ret;
    LuauScriptInstance::PropertySetGetError err_code;

    bool success = instance->get(StringName(key), ret, &err_code);
    if (success) {
        LuauBridge::push_variant(L, ret);
        return 1;
    }


    StringName prop_name = godot::resolve_prop_name(L, key);

    bool is_method = nobind::ClassDB::get_singleton()->class_has_method(
        object->get_class(), 
        prop_name, 
        false
    );
    if (is_method) {
        // method_call
        LuauBridge::push_uint64_t(L, object->get_instance_id());
        lua_pushstring(L, String(prop_name).utf8().get_data());
        lua_pushcclosure(L, [](lua_State *L) -> int {
            LuauObject* luau_object = LuauObject::get_luau_object(lua_upvalueindex(1));

            if (luau_object == nullptr) {
                return 0;
            }

            Object* obj = static_cast<Object*>(*luau_object);

            if (obj == nullptr) {
                WARN_PRINT(("method_call obj=nil"));
                return 0;
            }

            const char *method_name = lua_tostring(L, lua_upvalueindex(2));
            if (method_name == nullptr) {
                WARN_PRINT(("method_call method_name=nil"));
                return 0;
            }


            int arg_count = lua_gettop(L);
            Array args;
            
            int start_idx = 2;
            for (int i = start_idx; i <= arg_count; i++) {
                args.append(LuauBridge::get_variant(L, i));
            }

            WARN_PRINT(vformat("method_call %s.%s argsv=%s", obj, method_name, args));

            Variant result = obj->callv(StringName(method_name), args);

            LuauBridge::push_variant(L, result);
            return 1;
        }, "method_call", 2);

        return 1;
    }

    // Get object member
    Variant value = object->get(prop_name);
    if (value.get_type() != Variant::NIL) {
        LuauBridge::push_variant(L, value);
        return 1;
    }

    WARN_PRINT(vformat("Object.%s = nil;", key));

    // lua_pushnil(L);
    return 0;
}

template<>
int VariantBridge<Object*>::on_newindex(lua_State* L, Object* const &object, const char* key) {
    WARN_PRINT(vformat("Object.%s = nil; on_newindex setting.", key));
    return 1;
}

template<>
int VariantBridge<Object*>::on_call(lua_State* L, bool& is_valid) {
    //No constructors
    return 1;
}