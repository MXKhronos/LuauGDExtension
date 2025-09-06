
#include "plane.h"

#include <godot_cpp/variant/plane.hpp>

using namespace godot;

template<>
const char* VariantBridge<Plane>::variant_name("Plane");

const luaL_Reg PlaneBridge::static_library[] = {
	{NULL, NULL}
};

void PlaneBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Plane>::on_index(lua_State* L, const Plane& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Plane>::on_newindex(lua_State* L, Plane& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Plane>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PLANE: {
                push_from(L, v.operator Plane());
                return 1;
            }
            case Variant::VECTOR3: {
                push_from(L, v.operator Vector3());
                return 1;
            }
        };

    } else if (argc == 2) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);

        if (v1.get_type() == Variant::VECTOR3 && v2.get_type() == Variant::VECTOR3) {
            push_from(L, Plane(v1.operator Vector3(), v2.operator Vector3()));
            return 1;
        }

    } else if (argc == 3) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);
        Variant v3 = LuauBridge::get_variant(L, 4);
        
        if (v1.get_type() == Variant::VECTOR3 
            && v2.get_type() == Variant::VECTOR3 
            && v3.get_type() == Variant::VECTOR3
        ) {
            push_from(L, Plane(v1.operator Vector3(), v2.operator Vector3(), v3.operator Vector3()));
            return 1;
        }

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
            push_from(L, Plane(v1.operator float(), v2.operator float(), v3.operator float(), v4.operator float()));
            return 1;
        }

    }

    is_valid = false;
    return 1;
}