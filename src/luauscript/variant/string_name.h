#ifndef LUAU_VARIANT_STRINGNAME_H
#define LUAU_VARIANT_STRINGNAME_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {
namespace luau {

class StringNameBridge: public VariantBridge<StringName> {
    friend class VariantBridge <StringName>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};
};

#endif // LUAU_VARIANT_STRINGNAME_H