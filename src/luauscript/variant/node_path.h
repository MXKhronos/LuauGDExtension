#ifndef LUAU_VARIANT_NODE_PATH_H
#define LUAU_VARIANT_NODE_PATH_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class NodePathBridge: public VariantBridge<NodePath> {
    friend class VariantBridge <NodePath>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};

#endif // LUAU_VARIANT_NODE_PATH_H