#ifndef LUAU_VARIANT_PROJECTION_H
#define LUAU_VARIANT_PROJECTION_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {

class ProjectionBridge: public VariantBridge<Projection> {
    friend class VariantBridge <Projection>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
        static int create_depth_correction(lua_State* L);
        static int create_fit_aabb(lua_State* L);
        static int create_for_hmd(lua_State* L);
        static int create_frustum(lua_State* L);
        static int create_frustum_aspect(lua_State* L);
        static int create_light_atlas_rect(lua_State* L);
        static int create_orthogonal(lua_State* L);
        static int create_orthogonal_aspect(lua_State* L);
        static int create_perspective(lua_State* L);
        static int create_perspective_hmd(lua_State* L);
        static int get_fovy(lua_State* L);
};

};

#endif // LUAU_VARIANT_PROJECTION_H