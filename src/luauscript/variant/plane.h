#ifndef LUAU_VARIANT_PLANE_H
#define LUAU_VARIANT_PLANE_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class PlaneBridge: public VariantBridge<Plane> {
    friend class VariantBridge <Plane>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};

#endif // LUAU_VARIANT_PLANE_H