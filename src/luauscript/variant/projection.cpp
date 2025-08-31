
#include "projection.h"

#include <godot_cpp/variant/projection.hpp>

using namespace godot;
using namespace luau;


template<>
const char* VariantBridge<Projection>::variant_name("Projection");

const luaL_Reg ProjectionBridge::static_library[] = {
    {"create_depth_correction", create_depth_correction},
    {"create_fit_aabb", create_fit_aabb},
    {"create_for_hmd", create_for_hmd},
    {"create_frustum", create_frustum},
    {"create_frustum_aspect", create_frustum_aspect},
    {"create_light_atlas_rect", create_light_atlas_rect},
    {"create_orthogonal", create_orthogonal},
    {"create_orthogonal_aspect", create_orthogonal_aspect},
    {"create_perspective", create_perspective},
    {"create_perspective_hmd", create_perspective_hmd},
    {"get_fovy", get_fovy},
	{NULL, NULL}
};

void ProjectionBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

int ProjectionBridge::create_depth_correction(lua_State* L) {
    //MARK: TODO
    return 1;
}

int ProjectionBridge::create_fit_aabb(lua_State* L) {
    //MARK: TODO
    return 1;
}

int ProjectionBridge::create_for_hmd(lua_State* L) {
    //MARK: TODO
    return 1;
}

int ProjectionBridge::create_frustum(lua_State* L) {
    //MARK: TODO
    return 1;
}

int ProjectionBridge::create_frustum_aspect(lua_State* L) {
    //MARK: TODO
    return 1;
}

int ProjectionBridge::create_light_atlas_rect(lua_State* L) {
    //MARK: TODO
    return 1;
}

int ProjectionBridge::create_orthogonal(lua_State* L) {
    //MARK: TODO
    return 1;
}

int ProjectionBridge::create_orthogonal_aspect(lua_State* L) {
    //MARK: TODO
    return 1;
}

int ProjectionBridge::create_perspective(lua_State* L) {
    //MARK: TODO
    return 1;
}

int ProjectionBridge::create_perspective_hmd(lua_State* L) {
    //MARK: TODO
    return 1;
}

int ProjectionBridge::get_fovy(lua_State* L) {
    //MARK: TODO
    return 1;
}

template<>
int VariantBridge<Projection>::on_index(lua_State* L, const Projection& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Projection>::on_newindex(lua_State* L, Projection& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Projection>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PROJECTION: {
                push_from(L, v.operator Projection());
                return 1;
            }
            case Variant::TRANSFORM3D: {
                push_from(L, v.operator Transform3D());
                return 1;
            }
        };

    } else if (argc == 4) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);
        Variant v3 = LuauBridge::get_variant(L, 4);
        Variant v4 = LuauBridge::get_variant(L, 5);

        // vec4 vec4 vec4 vec4
        if (v1.get_type() == Variant::VECTOR4 
            && v2.get_type() == Variant::VECTOR4 
            && v3.get_type() == Variant::VECTOR4 
            && v4.get_type() == Variant::VECTOR4
        ) {
            push_from(L, Projection(
                v1.operator Vector4(), 
                v2.operator Vector4(), 
                v3.operator Vector4(), 
                v4.operator Vector4()
            ));
            return 1;
        }

    }

    is_valid = false;
    return 1;
}