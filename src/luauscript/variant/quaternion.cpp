
#include "quaternion.h"

#include <godot_cpp/variant/quaternion.hpp>

using namespace godot;
using namespace luau;

template<>
const char* VariantBridge<Quaternion>::variant_name("Quaternion");

const luaL_Reg QuaternionBridge::static_library[] = {
    {"from_euler", from_euler},
	{NULL, NULL}
};

void QuaternionBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

int QuaternionBridge::from_euler(lua_State* L) {
    Variant v = LuauBridge::get_variant(L, 1);

    if (v.get_type() != Variant::VECTOR3) {
        luaL_error(L, vformat("Cannot pass a value of type %s as %s.", 
            Variant::get_type_name(v.get_type()), 
            Variant::get_type_name(Variant::VECTOR3)
        ).utf8().get_data());
        return 1;
    }

    Quaternion result = Quaternion::from_euler(
        v.operator Vector3()
    );
    push_from(L, result);

    return 1;
}

template<>
int VariantBridge<Quaternion>::on_index(lua_State* L, const Quaternion& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Quaternion>::on_newindex(lua_State* L, Quaternion& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Quaternion>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::QUATERNION: {
                push_from(L, v.operator Quaternion());
                return 1;
            }
            case Variant::BASIS: {
                push_from(L, v.operator Basis());
                return 1;
            }
        };

    } else if (argc == 2) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);

        if (v1.get_type() == Variant::VECTOR3 && v2.get_type() == Variant::VECTOR3) {
            push_from(L, Quaternion(v1.operator Vector3(), v2.operator Vector3()));
            return 1;
        }
        if (v1.get_type() == Variant::VECTOR3 && v2.get_type() == Variant::FLOAT) {
            push_from(L, Quaternion(v1.operator Vector3(), v2.operator float()));
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
            push_from(L, Quaternion(v1.operator float(), v2.operator float(), v3.operator float(), v4.operator float()));
            return 1;
        }
        
    }

    is_valid = false;
    return 1;
}