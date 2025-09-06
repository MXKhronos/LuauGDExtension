#ifndef LUAU_VARIANT_VECTOR4_H
#define LUAU_VARIANT_VECTOR4_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class Vector4Bridge: public VariantBridge<Vector4> {
    friend class VariantBridge <Vector4>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};

#endif // LUAU_VARIANT_VECTOR4_H