#ifndef LUAU_VARIANT_OBJECT_H
#define LUAU_VARIANT_OBJECT_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/ref_counted.hpp>

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

            LuauObjectUD* ud = (LuauObjectUD*)lua_newuserdatatagged(L, sizeof(LuauObjectUD), OBJECT_UD_TAG);
            new (&ud->id) uint64_t(obj->get_instance_id());
            if (Object::cast_to<RefCounted>(obj) != nullptr) {
                new (&ud->ref) Variant(v);
            } else {
                new (&ud->ref) Variant();
            }

            luaL_getmetatable(L, variant_name);
            lua_setmetatable(L, -2);

            return obj;
        }

        static Object* get_object(lua_State* L, unsigned int index) {
            void *ud = LuauBridge::luaL_checkudata(L, index, variant_name);

            if (!ud) {
                luaL_error(L, "Invalid userdata");
            }

            return ObjectDB::get_instance(luau_object_ud_id(ud));
        }

    private:
        static const luaL_Reg static_library[];
};

};

#endif // LUAU_VARIANT_OBJECT_H