#ifndef LUAU_VARIANT_VECTOR4I_H
#define LUAU_VARIANT_VECTOR4I_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {
namespace luau {

class Vector4iBridge: public VariantBridge<Vector4i> {
    friend class VariantBridge <Vector4i>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};
};

#endif // LUAU_VARIANT_VECTOR4I_H