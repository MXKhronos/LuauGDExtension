#ifndef LUAU_VARIANT_OBJECT_H
#define LUAU_VARIANT_OBJECT_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class ObjectBridge: public VariantBridge<Object> {
    friend class VariantBridge <Object>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};

#endif // LUAU_VARIANT_OBJECT_H