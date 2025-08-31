
#include "vector2.h"

#include <godot_cpp/variant/vector2.hpp>

using namespace godot;
using namespace luau;

template<>
const char* VariantBridge<Vector2>::variant_name("Vector2");

const luaL_Reg Vector2Bridge::static_library[] = {
    {"from_angle", from_angle},
	{NULL, NULL}
};

void Vector2Bridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

int Vector2Bridge::from_angle(lua_State* L) {
    float angle = luaL_checknumber(L, 1);
    Vector2 result = Vector2::from_angle(angle);
    push_from(L, result);
    return 1;
}

template<>
int VariantBridge<Vector2>::on_index(lua_State* L, const Vector2& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Vector2>::on_newindex(lua_State* L, Vector2& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Vector2>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;

    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::VECTOR2: {
                push_from(L, v.operator Vector2());
                return 1;
            }
            case Variant::VECTOR2I: {
                push_from(L, v.operator Vector2());
                return 1;
            }
        };

    } else if (argc == 2) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);

        if (v1.get_type() == Variant::FLOAT && v2.get_type() == Variant::FLOAT) {
            push_from(L, Vector2(v1.operator float(), v2.operator float()));
            return 1;
        } 
        if (v1.get_type() == Variant::INT && v2.get_type() == Variant::INT) {
            push_from(L, Vector2(v1.operator int(), v2.operator int()));
            return 1;
        }

    }

    is_valid = false;
    return 1;
}