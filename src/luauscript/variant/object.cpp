
#include "object.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

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
    UtilityFunctions::print("Object on_index: %s\n", key);
    return 1;
}

template<>
int VariantBridge<Object*>::on_newindex(lua_State* L, Object* const &object, const char* key) {
    UtilityFunctions::print("Object on_newindex: %s\n", key);
    return 1;
}

template<>
int VariantBridge<Object*>::on_call(lua_State* L, bool& is_valid) {
    //No constructors
    return 1;
}