
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

    int arg_count = lua_gettop(L); // 2: GDV, key

    // String gdv = String(lua_tostring(L, 1));
    // String strkey = String(lua_tostring(L, 2));
    // WARN_PRINT(vformat("Object __on_index k(%s) arg_count=%s gdv=%s key=%s", key, arg_count, gdv, strkey));
    // WARN_PRINT(vformat("Object __on_index luau_object =%s", luau_object));

    luau_object->push_self(L); // GDV, key, self
    lua_pushvalue(L, 2); // GDV, key, self, key

    lua_rawget(L, -2);
    if (!lua_isnil(L, -1)) {
        return 1;
    }

    StringName prop_name = godot::resolve_prop_name(L, key);

    // Get object member
    Variant value = object->get(prop_name);
    if (value.get_type() != Variant::NIL) {
        LuauBridge::push_variant(L, value);
        return 1;
    }

     WARN_PRINT(vformat("Object.%s = nil;", key));

    lua_pushnil(L);
    return 1;
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