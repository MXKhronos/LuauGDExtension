#include "color.h"
#include "../luau_marshal.h"
#include <godot_cpp/variant/variant.hpp>
#include <cstring>
#include <cstdio>

using namespace godot;

namespace luau {

// Metatable name for Color instances
static const char *COLOR_INSTANCE_METATABLE = "Godot.ColorInstance";
static const char *COLOR_USERDATA_METATABLE = "Godot.ColorUserdata";

void register_color_class(lua_State *L) {
    // Create Color class table
    lua_newtable(L);
    
    // Store the Color table for later use
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "ColorClass");
    
    // Add static methods and constants to Color table
    
    // Add named color constants
    for (int i = 0; named_colors[i].name != nullptr; i++) {
        const ::godot::Color &c = named_colors[i].color;
        create_color_variant(L, c);
        lua_setfield(L, -2, named_colors[i].name);
    }
    
    // Add static methods
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char* hex = luaL_checkstring(L, 1);
        ::godot::Color c = ::godot::Color::html(hex);
        create_color_variant(L, c);
        return 1;
    }, "Color.from_html");
    lua_setfield(L, -2, "from_html");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        float h = luaL_checknumber(L, 1);
        float s = luaL_checknumber(L, 2);
        float v = luaL_checknumber(L, 3);
        float a = luaL_optnumber(L, 4, 1.0);
        ::godot::Color c = ::godot::Color::from_hsv(h, s, v, a);
        create_color_variant(L, c);
        return 1;
    }, "Color.from_hsv");
    lua_setfield(L, -2, "from_hsv");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char* hex = luaL_checkstring(L, 1);
        ::godot::Color c = ::godot::Color::html(hex);
        create_color_variant(L, c);
        return 1;
    }, "Color.html");
    lua_setfield(L, -2, "html");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        uint32_t hex_val = (uint32_t)luaL_checknumber(L, 1);
        ::godot::Color c = ::godot::Color::hex(hex_val);
        create_color_variant(L, c);
        return 1;
    }, "Color.hex");
    lua_setfield(L, -2, "hex");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        uint64_t hex_val = (uint64_t)luaL_checknumber(L, 1);
        ::godot::Color c = ::godot::Color::hex64(hex_val);
        create_color_variant(L, c);
        return 1;
    }, "Color.hex64");
    lua_setfield(L, -2, "hex64");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char* str = luaL_checkstring(L, 1);
        ::godot::Color default_color(0, 0, 0, 1);
        if (lua_gettop(L) >= 2 && lua_istable(L, 2)) {
            // Get default color from table
            lua_getfield(L, 2, "__godot_variant");
            if (lua_isuserdata(L, -1)) {
                void *udata = lua_touserdata(L, -1);
                ::godot::Color *color_ptr = udata ? (::godot::Color*)udata : nullptr;
                if (color_ptr) {
                    default_color = *color_ptr;
                }
            }
            lua_pop(L, 1);
        }
        ::godot::Color c = ::godot::Color::from_string(String(str), default_color);
        create_color_variant(L, c);
        return 1;
    }, "Color.from_string");
    lua_setfield(L, -2, "from_string");
    
    // Add static lerp method for Color.lerp(from, to, weight)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        // Get from color
        if (!lua_istable(L, 1)) {
            lua_pushstring(L, "Expected Color as first argument");
            lua_error(L);
            return 0;
        }
        lua_getfield(L, 1, "__godot_variant");
        void *udata = lua_touserdata(L, -1);
        ::godot::Color *color_from = udata ? (::godot::Color*)udata : nullptr;
        lua_pop(L, 1);
        
        if (!color_from) {
            lua_pushstring(L, "Invalid 'from' Color");
            lua_error(L);
            return 0;
        }
        
        // Get to color
        if (!lua_istable(L, 2)) {
            lua_pushstring(L, "Expected Color as second argument");
            lua_error(L);
            return 0;
        }
        lua_getfield(L, 2, "__godot_variant");
        void *udata2 = lua_touserdata(L, -1);
        ::godot::Color *color_to = udata2 ? (::godot::Color*)udata2 : nullptr;
        lua_pop(L, 1);
        
        if (!color_to) {
            lua_pushstring(L, "Invalid 'to' Color");
            lua_error(L);
            return 0;
        }
        
        // Get weight
        float weight = luaL_checknumber(L, 3);
        
        // Perform lerp
        ::godot::Color result = color_from->lerp(*color_to, weight);
        
        // Create and return new Color instance
        create_color_variant(L, result);
        return 1;
    }, "Color.lerp");
    lua_setfield(L, -2, "lerp");
    
    // Create metatable for Color class
    lua_newtable(L);
    
    // __index metamethod - handles accessing properties/methods from Color class
    lua_pushcfunction(L, [](lua_State *L) -> int {
        // self is the Color class table
        // key is what we're looking for
        const char *key = luaL_checkstring(L, 2);
        
        // Look up in the Color class table itself
        lua_pushstring(L, key);
        lua_rawget(L, 1);
        
        if (!lua_isnil(L, -1)) {
            return 1;
        }
        lua_pop(L, 1);
        
        // Not found
        lua_pushnil(L);
        return 1;
    }, "Color.__index");
    lua_setfield(L, -2, "__index");
    
    // __call metamethod to make Color() work as constructor
    lua_pushcfunction(L, [](lua_State *L) -> int {
        // Skip the table itself (first argument)
        int nargs = lua_gettop(L) - 1;
        ::godot::Color c;
        
        if (nargs == 0) {
            c = ::godot::Color(0, 0, 0, 1);
        } else if (nargs == 1 && lua_isstring(L, 2)) {
            const char* hex = lua_tostring(L, 2);
            c = ::godot::Color::html(hex);
        } else if (nargs >= 3) {
            float r = luaL_checknumber(L, 2);
            float g = luaL_checknumber(L, 3);
            float b = luaL_checknumber(L, 4);
            float a = luaL_optnumber(L, 5, 1.0);
            c = ::godot::Color(r, g, b, a);
        }
        
        create_color_variant(L, c);
        return 1;
    }, "Color.__call");
    lua_setfield(L, -2, "__call");
    
    // Set metatable to Color class
    lua_setmetatable(L, -2);
    
    // Create metatable for Color userdata
    luaL_newmetatable(L, COLOR_USERDATA_METATABLE);
    
    // __gc metamethod for cleanup
    lua_pushcfunction(L, [](lua_State *L) -> int {
        ::godot::Color *color = (::godot::Color*)lua_touserdata(L, 1);
        if (color) {
            color->~Color(); // Call destructor explicitly
        }
        return 0;
    }, "ColorUserdata.__gc");
    lua_setfield(L, -2, "__gc");
    lua_pop(L, 1); // Pop COLOR_USERDATA_METATABLE
    
    // Create metatable for Color instances
    luaL_newmetatable(L, COLOR_INSTANCE_METATABLE);
    
    // __index metamethod for instances - looks up in __godot_variant first, then Color class
    lua_pushcfunction(L, [](lua_State *L) -> int {
        // self is the instance, key is what we're looking for
        const char *key = luaL_checkstring(L, 2);
        
        // Debug: Print what we're looking for
        printf("Color.__index looking for key: %s\n", key);
        
        // First check __godot_variant for properties (r, g, b, a)
        lua_getfield(L, 1, "__godot_variant");
        if (lua_isuserdata(L, -1)) {
            void *udata = lua_touserdata(L, -1);
            ::godot::Color *color = udata ? (::godot::Color*)udata : nullptr;
            if (color) {
                printf("  Found __godot_variant with Color pointer\n");
                if (strcmp(key, "r") == 0) {
                    lua_pushnumber(L, color->r);
                    return 1;
                } else if (strcmp(key, "g") == 0) {
                    lua_pushnumber(L, color->g);
                    return 1;
                } else if (strcmp(key, "b") == 0) {
                    lua_pushnumber(L, color->b);
                    return 1;
                } else if (strcmp(key, "a") == 0) {
                    lua_pushnumber(L, color->a);
                    return 1;
                }
                // Check for methods
                else if (strcmp(key, "to_rgba32") == 0) {
                    lua_pushcfunction(L, [](lua_State *L) -> int {
                        lua_getfield(L, 1, "__godot_variant");
                        void *udata = lua_touserdata(L, -1);
                        ::godot::Color *color = udata ? (::godot::Color*)udata : nullptr;
                        lua_pop(L, 1);
                        if (color) {
                            lua_pushinteger(L, color->to_rgba32());
                            return 1;
                        }
                        return 0;
                    }, "to_rgba32");
                    return 1;
                } else if (strcmp(key, "to_html") == 0) {
                    lua_pushcfunction(L, [](lua_State *L) -> int {
                        lua_getfield(L, 1, "__godot_variant");
                        void *udata = lua_touserdata(L, -1);
                        ::godot::Color *color = udata ? (::godot::Color*)udata : nullptr;
                        lua_pop(L, 1);
                        if (color) {
                            String hex = color->to_html();
                            lua_pushstring(L, hex.utf8().get_data());
                            return 1;
                        }
                        return 0;
                    }, "to_html");
                    return 1;
                } else if (strcmp(key, "lerp") == 0) {
                    printf("  Found lerp method, returning function\n");
                    lua_pushcfunction(L, [](lua_State *L) -> int {
                        // Get self color
                        lua_getfield(L, 1, "__godot_variant");
                        void *udata = lua_touserdata(L, -1);
                        ::godot::Color *color_from = udata ? (::godot::Color*)udata : nullptr;
                        lua_pop(L, 1);
                        
                        if (!color_from) {
                            lua_pushstring(L, "Invalid color instance");
                            lua_error(L);
                            return 0;
                        }
                        
                        // Get target color
                        if (!lua_istable(L, 2)) {
                            lua_pushstring(L, "Expected Color as first argument");
                            lua_error(L);
                            return 0;
                        }
                        
                        lua_getfield(L, 2, "__godot_variant");
                        void *udata2 = lua_touserdata(L, -1);
                        ::godot::Color *color_to = udata2 ? (::godot::Color*)udata2 : nullptr;
                        lua_pop(L, 1);
                        
                        if (!color_to) {
                            lua_pushstring(L, "Expected Color as first argument");
                            lua_error(L);
                            return 0;
                        }
                        
                        // Get weight
                        float weight = luaL_checknumber(L, 3);
                        
                        // Perform lerp
                        ::godot::Color result = color_from->lerp(*color_to, weight);
                        
                        // Create and return new Color instance
                        create_color_variant(L, result);
                        return 1;
                    }, "lerp");
                    return 1;
                }
            }
        }
        lua_pop(L, 1); // Pop __godot_variant
        
        // Then check Color class for methods
        lua_getfield(L, LUA_REGISTRYINDEX, "ColorClass");
        if (lua_istable(L, -1)) {
            lua_pushstring(L, key);
            lua_rawget(L, -2);
            if (!lua_isnil(L, -1)) {
                // Found in Color class
                lua_remove(L, -2); // Remove Color class table
                return 1;
            }
            lua_pop(L, 1); // Pop nil
        }
        lua_pop(L, 1); // Pop Color class
        
        // Not found
        lua_pushnil(L);
        return 1;
    }, "ColorInstance.__index");
    lua_setfield(L, -2, "__index");
    
    // __newindex metamethod for instances - sets values in __godot_variant
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char *key = luaL_checkstring(L, 2);
        float value = luaL_checknumber(L, 3);
        
        // Get the Color userdata
        lua_getfield(L, 1, "__godot_variant");
        if (lua_isuserdata(L, -1)) {
            void *udata = lua_touserdata(L, -1);
            ::godot::Color *color = udata ? (::godot::Color*)udata : nullptr;
            if (color) {
                if (strcmp(key, "r") == 0) {
                    color->r = value;
                } else if (strcmp(key, "g") == 0) {
                    color->g = value;
                } else if (strcmp(key, "b") == 0) {
                    color->b = value;
                } else if (strcmp(key, "a") == 0) {
                    color->a = value;
                }
            }
        }
        lua_pop(L, 1);
        
        return 0;
    }, "ColorInstance.__newindex");
    lua_setfield(L, -2, "__newindex");
    
    // __tostring metamethod for debugging
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_getfield(L, 1, "__godot_variant");
        if (lua_isuserdata(L, -1)) {
            void *udata = lua_touserdata(L, -1);
            ::godot::Color *color = udata ? (::godot::Color*)udata : nullptr;
            lua_pop(L, 1);
            if (color) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "Color(%.3f, %.3f, %.3f, %.3f)", color->r, color->g, color->b, color->a);
                lua_pushstring(L, buffer);
                return 1;
            }
        }
        lua_pushstring(L, "Color(invalid)");
        return 1;
    }, "ColorInstance.__tostring");
    lua_setfield(L, -2, "__tostring");
    
    lua_pop(L, 1); // Pop the instance metatable
    
    // Set Color as global
    lua_setglobal(L, "Color");
}

// Helper function to create a Color instance
void create_color_variant(lua_State *L, const ::godot::Color &c) {
    // Create instance table
    lua_newtable(L);
    
    // Create userdata to hold the actual godot::Color object
    ::godot::Color *color_ptr = (::godot::Color*)lua_newuserdata(L, sizeof(::godot::Color));
    new (color_ptr) ::godot::Color(c); // Placement new to construct Color in allocated memory
    
    // Set metatable for the userdata
    luaL_getmetatable(L, COLOR_USERDATA_METATABLE);
    lua_setmetatable(L, -2);
    
    // Store the userdata as __godot_variant
    lua_setfield(L, -2, "__godot_variant");
    
    // Set metatable for the instance table
    luaL_getmetatable(L, COLOR_INSTANCE_METATABLE);
    lua_setmetatable(L, -2);
}

} // namespace luau
