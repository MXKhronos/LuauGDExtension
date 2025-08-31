
#include "string_name.h"

#include <godot_cpp/variant/string_name.hpp>

using namespace godot;
using namespace luau;

template<>
const char* VariantBridge<StringName>::variant_name("StringName");

const luaL_Reg StringNameBridge::static_library[] = {
	{NULL, NULL}
};

void StringNameBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<StringName>::on_index(lua_State* L, const StringName& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<StringName>::on_newindex(lua_State* L, StringName& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<StringName>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::STRING_NAME: {
                push_from(L, v.operator StringName());
                return 1;
            }
            case Variant::STRING: {
                push_from(L, StringName(v));
                return 1;
            }
        };

    } 

    is_valid = false;
    return 1;
}