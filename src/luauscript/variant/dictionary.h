#ifndef LUAU_VARIANT_DICTIONARY_H
#define LUAU_VARIANT_DICTIONARY_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class DictionaryBridge: public VariantBridge<Dictionary> {
    friend class VariantBridge <Dictionary>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};

};

#endif // LUAU_VARIANT_DICTIONARY_H