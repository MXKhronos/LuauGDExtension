#ifndef LUAU_VARIANT_RID_H
#define LUAU_VARIANT_RID_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class RIDBridge: public VariantBridge<RID> {
    friend class VariantBridge <RID>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};

#endif // LUAU_VARIANT_RID_H