
#include "basis.h"

using namespace godot;
using namespace luau;

template<>
const char* VariantBridge<Basis>::variant_name("Basis");

const luaL_Reg BasisBridge::static_library[] = {
    {"from_euler", from_euler},
    {"from_scale", from_scale},
    {"looking_at", looking_at},
	{NULL, NULL}
};

void BasisBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

int BasisBridge::from_euler(lua_State* L) {
    Variant v1 = LuauBridge::get_variant(L, 1);
    Variant v2 = LuauBridge::get_variant(L, 2);

    if (v1.get_type() != Variant::VECTOR3) {
        luaL_error(L, vformat("Cannot pass a value of type %s as %s.", 
            Variant::get_type_name(v1.get_type()), 
            Variant::get_type_name(Variant::VECTOR3)
        ).utf8().get_data());
        return 1;
    }
    if (v2.get_type() != Variant::FLOAT) {
        luaL_error(L, vformat("Cannot pass a value of type %s as %s.", 
            Variant::get_type_name(v2.get_type()), 
            "EulerOrder"
        ).utf8().get_data());
        return 1;
    }

    Basis result = Basis::from_euler(
        v1.operator Vector3(),
        static_cast<godot::EulerOrder>(v2.operator int32_t())
    );
    push_from(L, result);

    return 1;
}

int BasisBridge::from_scale(lua_State* L) {
    Variant v = LuauBridge::get_variant(L, 1);

    if (v.get_type() != Variant::VECTOR3) {
        luaL_error(L, vformat("Cannot pass a value of type %s as %s.", 
            Variant::get_type_name(v.get_type()), 
            Variant::get_type_name(Variant::VECTOR3)
        ).utf8().get_data());
        return 1;
    }
    
    Basis result = Basis::from_scale(
        v.operator Vector3()
    );
    push_from(L, result);

    return 1;
}

int BasisBridge::looking_at(lua_State* L) {
    Variant v1 = LuauBridge::get_variant(L, 1); //vec3;
    Variant v2 = LuauBridge::get_variant(L, 2); //vec3
    Variant v3 = LuauBridge::get_variant(L, 3); //bool


    if (v1.get_type() != Variant::VECTOR3) {
        luaL_error(L, vformat("Cannot pass a value of type %s as %s.", 
            Variant::get_type_name(v1.get_type()), 
            Variant::get_type_name(Variant::VECTOR3)
        ).utf8().get_data());
        return 1;
    }

    if (v2.get_type() != Variant::VECTOR3) {
        luaL_error(L, vformat("Cannot pass a value of type %s as %s.", 
            Variant::get_type_name(v2.get_type()), 
            Variant::get_type_name(Variant::VECTOR3)
        ).utf8().get_data());
        return 1;
    }

    if (v3.get_type() != Variant::BOOL) {
        luaL_error(L, vformat("Cannot pass a value of type %s as %s.", 
            Variant::get_type_name(v3.get_type()), 
            "bool"
        ).utf8().get_data());
        return 1;
    }
    
    Basis result = Basis::looking_at(
        v1.operator Vector3(),
        v2.operator Vector3(),
        v3.operator bool()
    );
    push_from(L, result);

    return 1;
}

template<>
int VariantBridge<Basis>::on_index(lua_State* L, const Basis& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Basis>::on_newindex(lua_State* L, Basis& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Basis>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::BASIS: {
                push_from(L, v.operator Basis());
                return 1;
            }
            case Variant::QUATERNION: {
                push_from(L, v.operator Quaternion());
                return 1;
            }
        };

    } else if (argc == 2) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);

        if (v1.get_type() == Variant::VECTOR3 && v2.get_type() == Variant::FLOAT) {
            push_from(L, Vector2i(v1.operator int32_t(), v2.operator int32_t()));
            return 1;
        }

    }

    is_valid = false;
    return 1;
}