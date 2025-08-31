#ifndef LUAU_VARIANT_RECT2_H
#define LUAU_VARIANT_RECT2_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {
namespace luau {

class Rect2Bridge : public VariantBridge<Rect2> {
    friend class VariantBridge <Rect2>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};
};

#endif // LUAU_VARIANT_RECT2_H