
#include "vector3i.h"

#include <godot_cpp/variant/vector3.hpp>

using namespace godot;

template<>
const char* VariantBridge<Vector3i>::variant_name("Vector3i");

const luaL_Reg Vector3iBridge::static_library[] = {
	{NULL, NULL}
};

void Vector3iBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    // CONSTANTS
    Vector3iBridge::push_from(L, Vector3i(0, 0, 0));
    lua_setfield(L, -2, "ZERO");
    
    Vector3iBridge::push_from(L, Vector3i(1, 1, 1));
    lua_setfield(L, -2, "ONE");
    
    Vector3iBridge::push_from(L, Vector3i(INT32_MIN, INT32_MIN, INT32_MIN));
    lua_setfield(L, -2, "MIN");
    
    Vector3iBridge::push_from(L, Vector3i(INT32_MAX, INT32_MAX, INT32_MAX));
    lua_setfield(L, -2, "MAX");
    
    Vector3iBridge::push_from(L, Vector3i(-1, 0, 0));
    lua_setfield(L, -2, "LEFT");
    
    Vector3iBridge::push_from(L, Vector3i(1, 0, 0));
    lua_setfield(L, -2, "RIGHT");
    
    Vector3iBridge::push_from(L, Vector3i(0, 1, 0));
    lua_setfield(L, -2, "UP");
    
    Vector3iBridge::push_from(L, Vector3i(0, -1, 0));
    lua_setfield(L, -2, "DOWN");
    
    Vector3iBridge::push_from(L, Vector3i(0, 0, -1));
    lua_setfield(L, -2, "FORWARD");
    
    Vector3iBridge::push_from(L, Vector3i(0, 0, 1));
    lua_setfield(L, -2, "BACK");


    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Vector3i>::on_index(lua_State* L, const Vector3i& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Vector3i>::on_newindex(lua_State* L, const Vector3i& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Vector3i>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::VECTOR3: {
                push_from(L, v.operator Vector3i());
                return 1;
            }
            case Variant::VECTOR3I: {
                push_from(L, v.operator Vector3i());
                return 1;
            }
        };

    } else if (argc == 3) {
        Variant x = LuauBridge::get_variant(L, 2);
        Variant y = LuauBridge::get_variant(L, 3);        
        Variant z = LuauBridge::get_variant(L, 4);        

        if (x.get_type() == Variant::INT 
            && y.get_type() == Variant::INT 
            && z.get_type() == Variant::INT
        ) {
            push_from(L, Vector3i(x.operator int(), y.operator int(), z.operator int()));
            return 1;
        }

    }

    is_valid = false;
    return 1;
}