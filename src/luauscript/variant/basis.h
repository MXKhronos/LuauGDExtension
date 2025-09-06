#ifndef LUAU_VARIANT_BASIS_H
#define LUAU_VARIANT_BASIS_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class BasisBridge: public VariantBridge<Basis> {
    friend class VariantBridge <Basis>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
        static int from_euler(lua_State* L);
        static int from_scale(lua_State* L);
        static int looking_at(lua_State* L);
};

};

#endif // LUAU_VARIANT_BASIS_H