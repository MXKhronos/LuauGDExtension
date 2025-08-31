
#include "transform3d.h"

#include <godot_cpp/variant/transform3d.hpp>

using namespace godot;
using namespace luau;

template<>
const char* VariantBridge<Transform3D>::variant_name("Transform3D");

const luaL_Reg Transform3DBridge::static_library[] = {
	{NULL, NULL}
};

void Transform3DBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Transform3D>::on_index(lua_State* L, const Transform3D& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Transform3D>::on_newindex(lua_State* L, Transform3D& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Transform3D>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::TRANSFORM3D: {
                push_from(L, v.operator Transform3D());
                return 1;
            }
            case Variant::PROJECTION: {
                push_from(L, v.operator Projection());
                return 1;
            }
        };

    } else if (argc == 2) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);

        if (v1.get_type() == Variant::BASIS && v2.get_type() == Variant::VECTOR3) {
            push_from(L, Transform3D(v1.operator Basis(), v2.operator Vector3()));
            return 1;
        }

    } else if (argc == 4) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);
        Variant v3 = LuauBridge::get_variant(L, 4);
        Variant v4 = LuauBridge::get_variant(L, 5);

        if (v1.get_type() == Variant::VECTOR3 
            && v2.get_type() == Variant::VECTOR3 
            && v3.get_type() == Variant::VECTOR3 
            && v4.get_type() == Variant::VECTOR3
        ) {
            push_from(L, Transform3D(v1.operator Vector3(), v2.operator Vector3(), v3.operator Vector3(), v4.operator Vector3()));
            return 1;
        }

    }

    is_valid = false;
    return 1;
}