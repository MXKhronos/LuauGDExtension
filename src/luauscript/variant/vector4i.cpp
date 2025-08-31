
#include "vector4i.h"

#include <godot_cpp/variant/vector4i.hpp>

using namespace godot;
using namespace luau;


template<>
const char* VariantBridge<Vector4i>::variant_name("Vector4i");

const luaL_Reg Vector4iBridge::static_library[] = {
	{NULL, NULL}
};

void Vector4iBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Vector4i>::on_index(lua_State* L, const Vector4i& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Vector4i>::on_newindex(lua_State* L, Vector4i& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Vector4i>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::VECTOR4: {
                push_from(L, v.operator Vector4());
                return 1;
            }
            case Variant::VECTOR4I: {
                push_from(L, v.operator Vector4i());
                return 1;
            }
        };

    } else if (argc == 4) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);
        Variant v3 = LuauBridge::get_variant(L, 4);
        Variant v4 = LuauBridge::get_variant(L, 5);

        if (v1.get_type() == Variant::FLOAT 
            && v2.get_type() == Variant::FLOAT 
            && v3.get_type() == Variant::FLOAT 
            && v4.get_type() == Variant::FLOAT
        ) {
            push_from(L, Vector4i(v1.operator int32_t(), v2.operator int32_t(), v3.operator int32_t(), v4.operator int32_t()));
            return 1;
        }

    }

    is_valid = false;
    return 1;
}