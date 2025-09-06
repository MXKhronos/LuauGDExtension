
#include "rid.h"

using namespace godot;

template<>
const char* VariantBridge<RID>::variant_name("RID");

const luaL_Reg RIDBridge::static_library[] = {
	{NULL, NULL}
};

void RIDBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<RID>::on_index(lua_State* L, const RID& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<RID>::on_newindex(lua_State* L, RID& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<RID>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::RID: {
                push_from(L, v.operator ::RID());
                return 1;
            }
        };

    }

    is_valid = false;
    return 1;
}