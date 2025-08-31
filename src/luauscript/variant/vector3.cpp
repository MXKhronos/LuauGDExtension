
#include "vector3.h"

#include <godot_cpp/variant/vector3.hpp>

using namespace godot;
using namespace luau;

template<>
const char* VariantBridge<Vector3>::variant_name("Vector3");

const luaL_Reg Vector3Bridge::static_library[] = {
    {"octahedron_decode", octahedron_decode},
	{NULL, NULL}
};

void Vector3Bridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

int Vector3Bridge::octahedron_decode(lua_State* L) {
    Variant v = LuauBridge::get_variant(L, 1);

    if (v.get_type() != Variant::VECTOR2) {
        luaL_error(L, "octahedron_decode requires a Vector2");
        return 1;
    }
    
    Vector3 result = Vector3::octahedron_decode(v.operator Vector2());
    push_from(L, result);

    return 1;
}

template<>
int VariantBridge<Vector3>::on_index(lua_State* L, const Vector3& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Vector3>::on_newindex(lua_State* L, Vector3& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Vector3>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::VECTOR3: {
                push_from(L, v.operator Vector3());
                return 1;
            }
            case Variant::VECTOR3I: {
                push_from(L, v.operator Vector3());
                return 1;
            }
        };

    } else if (argc == 3) {
        Variant x = LuauBridge::get_variant(L, 2);
        Variant y = LuauBridge::get_variant(L, 3);        
        Variant z = LuauBridge::get_variant(L, 4);        

        if (x.get_type() == Variant::FLOAT 
            && y.get_type() == Variant::FLOAT 
            && z.get_type() == Variant::FLOAT
        ) {
            push_from(L, Vector3(x.operator float(), y.operator float(), z.operator float()));
            return 1;
        }

    }

    is_valid = false;
    return 1;
}