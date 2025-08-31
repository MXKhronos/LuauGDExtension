#ifndef LUAU_VARIANT_COLOR_H
#define LUAU_VARIANT_COLOR_H

#include <godot_cpp/variant/variant.hpp>
#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {
namespace luau {

class ColorBridge : public VariantBridge<Color> {
    friend class VariantBridge <Color>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
        static int hex(lua_State* L);
        static int lerp(lua_State* L);
};

};
};

#endif // LUAU_VARIANT_COLOR_H