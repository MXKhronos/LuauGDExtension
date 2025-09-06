#ifndef LUAU_VARIANT_VECTOR3_H
#define LUAU_VARIANT_VECTOR3_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class Vector3Bridge: public VariantBridge<Vector3> {
    friend class VariantBridge <Vector3>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
        static int octahedron_decode(lua_State* L);
};

};

#endif // LUAU_VARIANT_VECTOR3_H