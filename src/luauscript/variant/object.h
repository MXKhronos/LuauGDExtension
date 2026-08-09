#ifndef LUAU_VARIANT_OBJECT_H
#define LUAU_VARIANT_OBJECT_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

#include <godot_cpp/core/object.hpp>

namespace godot {

class ObjectBridge: public VariantBridge<Object*> {
    friend class VariantBridge <Object*>;

    public:
        static void register_variant_class(lua_State* L);

        static Object* push_from(lua_State* L, const Variant& v) {
            Object* obj = (Object*)v;
            if (obj == nullptr) {
                lua_pushnil(L);
                return nullptr;
            }

            uint64_t* ud = (uint64_t*)lua_newuserdata(L, sizeof(uint64_t));
            new (ud) uint64_t(obj->get_instance_id());

            luaL_getmetatable(L, variant_name);
            lua_setmetatable(L, -2);

            return obj;
        }

        static Object* get_object(lua_State* L, unsigned int index) {
            void *ud = LuauBridge::luaL_checkudata(L, index, variant_name);

            if (!ud) {
                luaL_error(L, "Invalid userdata");
            }

            uint64_t obj_id = *(uint64_t*)ud;
            return ObjectDB::get_instance(obj_id);
        }

    private:
        static const luaL_Reg static_library[];
};

};

#endif // LUAU_VARIANT_OBJECT_H