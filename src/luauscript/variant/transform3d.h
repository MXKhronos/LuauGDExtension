#ifndef LUAU_VARIANT_TRANSFORM3D_H
#define LUAU_VARIANT_TRANSFORM3D_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {
namespace luau {

class Transform3DBridge: public VariantBridge<Transform3D> {
    friend class VariantBridge <Transform3D>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};
};

#endif // LUAU_VARIANT_TRANSFORM3D_H