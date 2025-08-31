
#include "vector2i.h"

#include <godot_cpp/variant/vector2i.hpp>

using namespace godot;
using namespace luau;

template<>
const char* VariantBridge<Vector2i>::variant_name("Vector2i");

const luaL_Reg Vector2iBridge::static_library[] = {
	{NULL, NULL}
};

void Vector2iBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Vector2i>::on_index(lua_State* L, const Vector2i& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Vector2i>::on_newindex(lua_State* L, Vector2i& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Vector2i>::on_call(lua_State* L) {
    const int argc = lua_gettop(L)-1;

    if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::VECTOR2I: {
                push_from(L, v.operator Vector2i());
                return 1;
            }
            case Variant::VECTOR2: {
                push_from(L, v.operator Vector2());
                return 1;
            }
        };

    } else if (argc == 2) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);

        if (v1.get_type() == Variant::INT && v2.get_type() == Variant::INT) {
            push_from(L, Vector2i(v1.operator int(), v2.operator int()));
            return 1;
        }
        if (v1.get_type() == Variant::FLOAT && v2.get_type() == Variant::FLOAT) {
            push_from(L, Vector2i(v1.operator float(), v2.operator float()));
            return 1;
        }

    }

    push_new(L);
    return 1;
}