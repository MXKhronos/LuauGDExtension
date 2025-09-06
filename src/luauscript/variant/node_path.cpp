
#include "node_path.h"

#include <godot_cpp/variant/node_path.hpp>

using namespace godot;

template<>
const char* VariantBridge<NodePath>::variant_name("NodePath");

const luaL_Reg NodePathBridge::static_library[] = {
	{NULL, NULL}
};

void NodePathBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<NodePath>::on_index(lua_State* L, const NodePath& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<NodePath>::on_newindex(lua_State* L, NodePath& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<NodePath>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::NODE_PATH: {
                push_from(L, v.operator NodePath());
                return 1;
            }
            case Variant::STRING: {
                push_from(L, v.operator NodePath());
                return 1;
            }
        };

    } 
    
    is_valid = false;
    return 1;
}