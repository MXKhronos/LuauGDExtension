#ifndef LUAU_VARIANT_VECTOR3I_H
#define LUAU_VARIANT_VECTOR3I_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {
namespace luau {

class Vector3iBridge: public VariantBridge<Vector3i> {
    friend class VariantBridge <Vector3i>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};
};

#endif // LUAU_VARIANT_VECTOR3_H