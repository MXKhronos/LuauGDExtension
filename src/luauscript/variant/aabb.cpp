
#include "aabb.h"

#include <godot_cpp/variant/aabb.hpp>

using namespace godot;
using namespace luau;

template<>
const char* VariantBridge<AABB>::variant_name("AABB");

const luaL_Reg AABBBridge::static_library[] = {
	{NULL, NULL}
};

void AABBBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<AABB>::on_index(lua_State* L, const AABB& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<AABB>::on_newindex(lua_State* L, AABB& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<AABB>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::AABB: {
                push_from(L, v.operator godot::AABB());
                return 1;
            }
        };

    } else if (argc == 2) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);

        if (v1.get_type() == Variant::VECTOR3 && v2.get_type() == Variant::VECTOR3) {
            push_from(L, AABB(v1.operator Vector3(), v2.operator Vector3()));
            return 1;
        }

    }

    is_valid = false;
    return 1;
}