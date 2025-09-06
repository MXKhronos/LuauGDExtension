
#include "transform2d.h"

#include <godot_cpp/variant/transform2d.hpp>

using namespace godot;


template<>
const char* VariantBridge<Transform2D>::variant_name("Transform2D");

const luaL_Reg Transform2DBridge::static_library[] = {
	{NULL, NULL}
};

void Transform2DBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Transform2D>::on_index(lua_State* L, const Transform2D& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Transform2D>::on_newindex(lua_State* L, Transform2D& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Transform2D>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 1) {
        Variant v1 = LuauBridge::get_variant(L, 2);

        switch(v1.get_type()) {
            case Variant::TRANSFORM2D: {
                push_from(L, v1.operator Transform2D());
                return 1;
            }
        };

    } else if (argc == 2) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);

        if (v1.get_type() == Variant::FLOAT && v2.get_type() == Variant::VECTOR2) {
            push_from(L, Transform2D(v1.operator float(), v2.operator Vector2()));
            return 1;
        }

    } else if (argc == 3) {

        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);
        Variant v3 = LuauBridge::get_variant(L, 4);

        if (v1.get_type() == Variant::VECTOR2
            && v2.get_type() == Variant::VECTOR2
            && v3.get_type() == Variant::VECTOR2
        ) {
            push_from(L, Transform2D(v1.operator Vector2(), v2.operator Vector2(), v3.operator Vector2()));
            return 1;
        }

    } else if (argc == 4) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);
        Variant v3 = LuauBridge::get_variant(L, 4);
        Variant v4 = LuauBridge::get_variant(L, 5);

        if (v1.get_type() == Variant::FLOAT 
            && v2.get_type() == Variant::VECTOR2 
            && v3.get_type() == Variant::FLOAT 
            && v4.get_type() == Variant::VECTOR2
        ) {
            push_from(L, Transform2D(v1.operator float(), v2.operator Vector2(), v3.operator float(), v4.operator Vector2()));
            return 1;
        }

    }

    is_valid = false;
    return 1;
}