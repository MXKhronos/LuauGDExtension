
#include "color.h"

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/color_names.inc.hpp>

using namespace godot;
using namespace luau;

template<>
const char* VariantBridge<Color>::variant_name("Color");

const luaL_Reg ColorBridge::static_library[] = {
    {"hex", hex},
    {"lerp", lerp},
	{NULL, NULL}
};

int ColorBridge::hex(lua_State* L) {
    Variant v = LuauBridge::get_variant(L, 1);

    if (v.get_type() != Variant::FLOAT) {
        luaL_error(L, vformat("Cannot pass a value of type %s as %s.", 
            Variant::get_type_name(v.get_type()), 
            "int"
        ).utf8().get_data());
        return 1;
    }
    
    Color result = Color::hex(v.operator int32_t());
    push_from(L, result);

    return 1;
}

int ColorBridge::lerp(lua_State* L) {
    const int argc = lua_gettop(L);
    if (argc < 3) {
        luaL_error(L, "Color.lerp requires at least 3 arguments");
        return 1;
    }

    Color from = get_object(L, 1);

    Variant to = LuauBridge::get_variant(L, 2);
    if (to.get_type() != Variant::COLOR) {
        WARN_PRINT("Color.lerp: to argument is not a color: " + String(Variant::get_type_name(to.get_type())));
        return 1;
    }
    float weight = luaL_checknumber(L, 3);

    Color result = from.lerp(to, weight);
    push_from(L, result);

    return 1;
}

void ColorBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);
        
    // Add named color constants
    for (int i = 0; named_colors[i].name != nullptr; i++) {
        const Color &c = named_colors[i].color;
        ColorBridge::push_from(L, c);
        lua_setfield(L, -2, named_colors[i].name);
    }

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);

    lua_pop(L, 1);
}

template<>
int VariantBridge<Color>::on_index(lua_State* L, const Color& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Color>::on_newindex(lua_State* L, Color& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Color>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;

    } else if (argc == 1) {
        Variant v1 = LuauBridge::get_variant(L, 2);

        if (v1.get_type() == Variant::COLOR) {
            push_from(L, Color(v1));
            return 1;

        } else if (v1.get_type() == Variant::STRING) {
            push_from(L, Color(String(v1)));
            return 1;

        } 

    } else if (argc == 2) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);
        if (v1.get_type() == Variant::COLOR 
            && v2.get_type() == Variant::FLOAT
        ) {
            push_from(L, Color(v1.operator Color(), v2.operator float()));
            return 1;
            
        } else if (v1.get_type() == Variant::STRING 
            && v2.get_type() == Variant::FLOAT
        ) {
            push_from(L, Color(String(v1), v2.operator float()));
            return 1;
            
        } 

    } else if (argc == 3) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);
        Variant v3 = LuauBridge::get_variant(L, 4);

        if (v1.get_type() == Variant::FLOAT 
            && v2.get_type() == Variant::FLOAT 
            && v3.get_type() == Variant::FLOAT
        ) {
            push_from(L, Color(v1.operator float(), v2.operator float(), v3.operator float()));
            return 1;
            
        } 

    } else if (argc == 4) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);
        Variant v3 = LuauBridge::get_variant(L, 4);
        Variant v4 = LuauBridge::get_variant(L, 5);

        if (v1.get_type() == Variant::FLOAT 
            && v2.get_type() == Variant::FLOAT 
            && v3.get_type() == Variant::FLOAT 
            && v4.get_type() == Variant::FLOAT
        ) {
            push_from(L, Color(v1.operator float(), v2.operator float(), v3.operator float(), v4.operator float()));
            return 1;
        }        
    }

    is_valid = false;
    return 1;
}