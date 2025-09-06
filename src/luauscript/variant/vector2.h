#ifndef LUAU_VARIANT_VECTOR2_H
#define LUAU_VARIANT_VECTOR2_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class Vector2Bridge: public VariantBridge<Vector2> {
    friend class VariantBridge <Vector2>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
        static int from_angle(lua_State* L);
};

};

#endif // LUAU_VARIANT_VECTOR2_H