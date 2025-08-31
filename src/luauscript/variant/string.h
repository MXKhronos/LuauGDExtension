#ifndef LUAU_VARIANT_STRING_H
#define LUAU_VARIANT_STRING_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {
namespace luau {

class StringBridge : public VariantBridge<godot::String> {
    friend class VariantBridge <godot::String>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
        static int chr(lua_State* L);
        static int humanize_size(lua_State* L);
        static int num(lua_State* L);
        static int num_int64(lua_State* L);
        static int num_scientific(lua_State* L);
        static int num_uint64(lua_State* L);
};

};
};

#endif // LUAU_VARIANT_TEMPLATE_H