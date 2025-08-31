
#include "string.h"

#include <godot_cpp/variant/string.hpp>

using namespace godot;
using namespace luau;

template<>
const char* VariantBridge<String>::variant_name("String");

const luaL_Reg StringBridge::static_library[] = {
    {"chr", chr},
    {"humanize_size", humanize_size},
    {"num", num},
    {"num_int64", num_int64},
    {"num_scientific", num_scientific},
    {"num_uint64", num_uint64},
	{NULL, NULL}
};

int StringBridge::chr(lua_State* L) {
    //MARK: TODO
    return 1;
}

int StringBridge::humanize_size(lua_State* L) {
    //MARK: TODO
    return 1;
}

int StringBridge::num(lua_State* L) {
    //MARK: TODO
    return 1;
}

int StringBridge::num_int64(lua_State* L) {
    //MARK: TODO
    return 1;
}

int StringBridge::num_scientific(lua_State* L) {
    //MARK: TODO
    return 1;
}

int StringBridge::num_uint64(lua_State* L) {
    //MARK: TODO
    return 1;
}

void StringBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<String>::on_index(lua_State* L, const String& object, const char* key) {
    lua_pushnil(L);
    return 1;
}

template<>
int VariantBridge<String>::on_newindex(lua_State* L, String& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<String>::on_call(lua_State* L) {
    const int argc = lua_gettop(L)-1;

    if (argc == 1) {
        push_from(L, LuauBridge::get_string(L, 2));
    }

    push_new(L);
    return 1;
}