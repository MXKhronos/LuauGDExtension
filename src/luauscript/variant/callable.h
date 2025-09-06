#ifndef LUAU_VARIANT_CALLABLE_H
#define LUAU_VARIANT_CALLABLE_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class CallableBridge: public VariantBridge<Callable> {
    friend class VariantBridge <Callable>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
        static int create(lua_State* L);
};

class SignalBridge: public VariantBridge<Signal> {
    friend class VariantBridge <Signal>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};

#endif // LUAU_VARIANT_TEMPLATE_H