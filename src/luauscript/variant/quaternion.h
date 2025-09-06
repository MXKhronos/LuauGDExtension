#ifndef LUAU_VARIANT_QUATERNION_H
#define LUAU_VARIANT_QUATERNION_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class QuaternionBridge: public VariantBridge<Quaternion> {
    friend class VariantBridge <Quaternion>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
        static int from_euler(lua_State* L);
};

};

#endif // LUAU_VARIANT_QUATERNION_H