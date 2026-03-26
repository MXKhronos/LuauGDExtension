
#include "vector3.h"

#include <godot_cpp/variant/vector3.hpp>

using namespace godot;

template<>
const char* VariantBridge<Vector3>::variant_name("Vector3");

const luaL_Reg Vector3Bridge::static_library[] = {
    {"octahedron_decode", octahedron_decode},
	{NULL, NULL}
};

void Vector3Bridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    // CONSTANTS
    Vector3Bridge::push_from(L, Vector3(0, 0, 0));
    lua_setfield(L, -2, "ZERO");

    Vector3Bridge::push_from(L, Vector3(1, 1, 1));
    lua_setfield(L, -2, "ONE");

    Vector3Bridge::push_from(L, Vector3(Math_INF, Math_INF, Math_INF));
    lua_setfield(L, -2, "INF");

    Vector3Bridge::push_from(L, Vector3(-1, 0, 0));
    lua_setfield(L, -2, "LEFT");
    
    Vector3Bridge::push_from(L, Vector3(1, 0, 0));
    lua_setfield(L, -2, "RIGHT");
    
    Vector3Bridge::push_from(L, Vector3(0, -1, 0));
    lua_setfield(L, -2, "DOWN");
    
    Vector3Bridge::push_from(L, Vector3(0, 1, 0));
    lua_setfield(L, -2, "UP");
    
    Vector3Bridge::push_from(L, Vector3(0, 0, -1));
    lua_setfield(L, -2, "FORWARD");
    
    Vector3Bridge::push_from(L, Vector3(0, 0, 1));
    lua_setfield(L, -2, "BACK");

    Vector3Bridge::push_from(L, Vector3(1, 0, 0));
    lua_setfield(L, -2, "MODEL_LEFT");
    
    Vector3Bridge::push_from(L, Vector3(-1, 0, 0));
    lua_setfield(L, -2, "MODEL_RIGHT");
    
    Vector3Bridge::push_from(L, Vector3(0, 1, 0));
    lua_setfield(L, -2, "MODEL_TOP");
    
    Vector3Bridge::push_from(L, Vector3(0, -1, 0));
    lua_setfield(L, -2, "MODEL_BOTTOM");
    
    Vector3Bridge::push_from(L, Vector3(0, 0, 1));
    lua_setfield(L, -2, "MODEL_FRONT");
    
    Vector3Bridge::push_from(L, Vector3(0, 0, -1));
    lua_setfield(L, -2, "MODEL_REAR");


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
int VariantBridge<Vector3>::on_newindex(lua_State* L, const Vector3& object, const char* key) {
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