#ifndef LUAU_COLOR_HPP
#define LUAU_COLOR_HPP

#include <lua.h>
#include <lualib.h>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/color_names.inc.hpp>
#include <string>
#include "../luau_marshal.h"

namespace luau {

// Register the Color class proxy in the Lua state
void register_color_class(lua_State *L);
void create_color_variant(lua_State *L, const ::godot::Color &c);

// Helper struct to push Color to Lua stack
struct Color {
    Color(lua_State *L, godot::Color color) {
        create_color_variant(L, color);
    }
};

} // namespace luau

#endif // LUAU_COLOR_HPP
