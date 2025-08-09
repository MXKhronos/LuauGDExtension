#include "luau_engine.h"

#include <lua.h>
#include <lualib.h>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/color_names.inc.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "nobind.h"
#include "luau_marshal.h"

using namespace godot;

LuauEngine *LuauEngine::singleton = nullptr;

static void *luauGD_alloc(void *, void *ptr, size_t, size_t nsize) {
	if (nsize == 0) {
		if (ptr)
			memfree(ptr);

		return nullptr;
	}

	return memrealloc(ptr, nsize);
}

void luaGD_close(lua_State *L) {
	L = lua_mainthread(L);

	lua_close(L);
}


void LuauEngine::register_color_class(lua_State *L) {
    // Create Color proxy table
    lua_newtable(L);
    
    // Create metatable for Color proxy
    lua_newtable(L);
    
    // __index metamethod to handle property/method access
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char *key = luaL_checkstring(L, 2);
        
        // Check if it's a named color constant
        for (int i = 0; named_colors[i].name != nullptr; i++) {
            if (strcmp(named_colors[i].name, key) == 0) {
                const Color &c = named_colors[i].color;
                // Push color as table with r,g,b,a fields
                lua_newtable(L);
                lua_pushnumber(L, c.r); lua_setfield(L, -2, "r");
                lua_pushnumber(L, c.g); lua_setfield(L, -2, "g");
                lua_pushnumber(L, c.b); lua_setfield(L, -2, "b");
                lua_pushnumber(L, c.a); lua_setfield(L, -2, "a");
                return 1;
            }
        }
        
        // Check for constructor/static methods
        if (strcmp(key, "new") == 0) {
            // Return constructor function
            lua_pushcfunction(L, [](lua_State *L) -> int {
                int nargs = lua_gettop(L);
                Color c;
                
                if (nargs == 0) {
                    // Default black color
                    c = Color(0, 0, 0, 1);
                } else if (nargs == 1 && lua_isstring(L, 1)) {
                    // Color from hex string
                    const char* hex = lua_tostring(L, 1);
                    c = Color::html(hex);
                } else if (nargs >= 3) {
                    // Color(r, g, b, a = 1.0)
                    float r = luaL_checknumber(L, 1);
                    float g = luaL_checknumber(L, 2);
                    float b = luaL_checknumber(L, 3);
                    float a = luaL_optnumber(L, 4, 1.0);
                    c = Color(r, g, b, a);
                }
                
                // Return color as Variant that will be marshaled properly
                LuauMarshal::push_variant(L, Variant(c));
                return 1;
            }, "Color.new");
            return 1;
        }
        
        if (strcmp(key, "from_html") == 0) {
            lua_pushcfunction(L, [](lua_State *L) -> int {
                const char* hex = luaL_checkstring(L, 1);
                Color c = Color::html(hex);
                LuauMarshal::push_variant(L, Variant(c));
                return 1;
            }, "Color.from_html");
            return 1;
        }
        
        if (strcmp(key, "from_hsv") == 0) {
            lua_pushcfunction(L, [](lua_State *L) -> int {
                float h = luaL_checknumber(L, 1);
                float s = luaL_checknumber(L, 2);
                float v = luaL_checknumber(L, 3);
                float a = luaL_optnumber(L, 4, 1.0);
                Color c = Color::from_hsv(h, s, v, a);
                LuauMarshal::push_variant(L, Variant(c));
                return 1;
            }, "Color.from_hsv");
            return 1;
        }
        
        if (strcmp(key, "html") == 0) {
            // Alias for from_html
            lua_pushcfunction(L, [](lua_State *L) -> int {
                const char* hex = luaL_checkstring(L, 1);
                Color c = Color::html(hex);
                LuauMarshal::push_variant(L, Variant(c));
                return 1;
            }, "Color.html");
            return 1;
        }
        
        if (strcmp(key, "hex") == 0) {
            lua_pushcfunction(L, [](lua_State *L) -> int {
                uint32_t hex_val = (uint32_t)luaL_checknumber(L, 1);
                Color c = Color::hex(hex_val);
                LuauMarshal::push_variant(L, Variant(c));
                return 1;
            }, "Color.hex");
            return 1;
        }
        
        if (strcmp(key, "hex64") == 0) {
            lua_pushcfunction(L, [](lua_State *L) -> int {
                uint64_t hex_val = (uint64_t)luaL_checknumber(L, 1);
                Color c = Color::hex64(hex_val);
                LuauMarshal::push_variant(L, Variant(c));
                return 1;
            }, "Color.hex64");
            return 1;
        }
        
        if (strcmp(key, "from_string") == 0) {
            lua_pushcfunction(L, [](lua_State *L) -> int {
                const char* str = luaL_checkstring(L, 1);
                Color default_color(0, 0, 0, 1);
                if (lua_gettop(L) >= 2 && lua_istable(L, 2)) {
                    // Get default color from table
                    Variant v = LuauMarshal::get_variant(L, 2);
                    if (v.get_type() == Variant::COLOR) {
                        default_color = v.operator Color();
                    }
                }
                Color c = Color::from_string(String(str), default_color);
                LuauMarshal::push_variant(L, Variant(c));
                return 1;
            }, "Color.from_string");
            return 1;
        }
        
        // Not found
        lua_pushnil(L);
        return 1;
    }, "Color.__index");
    lua_setfield(L, -2, "__index");
    
    // __call metamethod to make Color() work as constructor
    lua_pushcfunction(L, [](lua_State *L) -> int {
        // Skip the table itself (first argument)
        int nargs = lua_gettop(L) - 1;
        Color c;
        
        if (nargs == 0) {
            c = Color(0, 0, 0, 1);
        } else if (nargs == 1 && lua_isstring(L, 2)) {
            const char* hex = lua_tostring(L, 2);
            c = Color::html(hex);
        } else if (nargs >= 3) {
            float r = luaL_checknumber(L, 2);
            float g = luaL_checknumber(L, 3);
            float b = luaL_checknumber(L, 4);
            float a = luaL_optnumber(L, 5, 1.0);
            c = Color(r, g, b, a);
        }
        
        LuauMarshal::push_variant(L, Variant(c));
        return 1;
    }, "Color.__call");
    lua_setfield(L, -2, "__call");
    
    // Set metatable
    lua_setmetatable(L, -2);
    
    // Set as global
    lua_setglobal(L, "Color");
}

void LuauEngine::register_vector2_class(lua_State *L) {
    // Create Vector2 table
    lua_newtable(L);
    
    // Vector2 constants
    lua_newtable(L);
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "y");
    lua_setfield(L, -2, "ZERO");
    
    lua_newtable(L);
    lua_pushnumber(L, 1.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 1.0); lua_setfield(L, -2, "y");
    lua_setfield(L, -2, "ONE");
    
    lua_newtable(L);
    lua_pushnumber(L, Math_INF); lua_setfield(L, -2, "x");
    lua_pushnumber(L, Math_INF); lua_setfield(L, -2, "y");
    lua_setfield(L, -2, "INF");
    
    lua_newtable(L);
    lua_pushnumber(L, -1.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "y");
    lua_setfield(L, -2, "LEFT");
    
    lua_newtable(L);
    lua_pushnumber(L, 1.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "y");
    lua_setfield(L, -2, "RIGHT");
    
    lua_newtable(L);
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, -1.0); lua_setfield(L, -2, "y");
    lua_setfield(L, -2, "UP");
    
    lua_newtable(L);
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 1.0); lua_setfield(L, -2, "y");
    lua_setfield(L, -2, "DOWN");
    
    // Add constructor function
    lua_pushcfunction(L, [](lua_State *L) -> int {
        float x = luaL_optnumber(L, 1, 0.0);
        float y = luaL_optnumber(L, 2, 0.0);
        
        lua_newtable(L);
        lua_pushnumber(L, x); lua_setfield(L, -2, "x");
        lua_pushnumber(L, y); lua_setfield(L, -2, "y");
        
        return 1;
    }, "Vector2");
    lua_setfield(L, -2, "new");
    
    // Set as global
    lua_setglobal(L, "Vector2");
}

void LuauEngine::register_vector3_class(lua_State *L) {
    // Create Vector3 table
    lua_newtable(L);
    
    // Vector3 constants
    lua_newtable(L);
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "y");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "ZERO");
    
    lua_newtable(L);
    lua_pushnumber(L, 1.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 1.0); lua_setfield(L, -2, "y");
    lua_pushnumber(L, 1.0); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "ONE");
    
    lua_newtable(L);
    lua_pushnumber(L, Math_INF); lua_setfield(L, -2, "x");
    lua_pushnumber(L, Math_INF); lua_setfield(L, -2, "y");
    lua_pushnumber(L, Math_INF); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "INF");
    
    lua_newtable(L);
    lua_pushnumber(L, -1.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "y");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "LEFT");
    
    lua_newtable(L);
    lua_pushnumber(L, 1.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "y");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "RIGHT");
    
    lua_newtable(L);
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 1.0); lua_setfield(L, -2, "y");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "UP");
    
    lua_newtable(L);
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, -1.0); lua_setfield(L, -2, "y");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "DOWN");
    
    lua_newtable(L);
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "y");
    lua_pushnumber(L, 1.0); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "FORWARD");
    
    lua_newtable(L);
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0.0); lua_setfield(L, -2, "y");
    lua_pushnumber(L, -1.0); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "BACK");
    
    // Add constructor function
    lua_pushcfunction(L, [](lua_State *L) -> int {
        float x = luaL_optnumber(L, 1, 0.0);
        float y = luaL_optnumber(L, 2, 0.0);
        float z = luaL_optnumber(L, 3, 0.0);
        
        lua_newtable(L);
        lua_pushnumber(L, x); lua_setfield(L, -2, "x");
        lua_pushnumber(L, y); lua_setfield(L, -2, "y");
        lua_pushnumber(L, z); lua_setfield(L, -2, "z");
        
        return 1;
    }, "Vector3");
    lua_setfield(L, -2, "new");
    
    // Set as global
    lua_setglobal(L, "Vector3");
}

void LuauEngine::register_rect2_class(lua_State *L) {
    // Register Rect2 constructor
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int nargs = lua_gettop(L);
        lua_newtable(L);
        
        if (nargs == 0) {
            // Default Rect2()
            lua_newtable(L);
            lua_pushnumber(L, 0.0); lua_setfield(L, -2, "x");
            lua_pushnumber(L, 0.0); lua_setfield(L, -2, "y");
            lua_setfield(L, -2, "position");
            
            lua_newtable(L);
            lua_pushnumber(L, 0.0); lua_setfield(L, -2, "x");
            lua_pushnumber(L, 0.0); lua_setfield(L, -2, "y");
            lua_setfield(L, -2, "size");
        } else if (nargs == 4) {
            // Four arguments: x, y, width, height
            lua_newtable(L);
            lua_pushnumber(L, luaL_checknumber(L, 1)); lua_setfield(L, -2, "x");
            lua_pushnumber(L, luaL_checknumber(L, 2)); lua_setfield(L, -2, "y");
            lua_setfield(L, -2, "position");
            
            lua_newtable(L);
            lua_pushnumber(L, luaL_checknumber(L, 3)); lua_setfield(L, -2, "x");
            lua_pushnumber(L, luaL_checknumber(L, 4)); lua_setfield(L, -2, "y");
            lua_setfield(L, -2, "size");
        } else if (nargs == 2) {
            // Two arguments: position and size tables
            lua_pushvalue(L, 1);
            lua_setfield(L, -2, "position");
            
            lua_pushvalue(L, 2);
            lua_setfield(L, -2, "size");
        }
        
        return 1;
    }, "Rect2");
    lua_setglobal(L, "Rect2");
}

void LuauEngine::register_math_constants(lua_State *L) {
    // Math constants
    lua_pushnumber(L, Math_PI);
    lua_setglobal(L, "PI");
    
    lua_pushnumber(L, Math_TAU);
    lua_setglobal(L, "TAU");
    
    lua_pushnumber(L, Math_INF);
    lua_setglobal(L, "INF");
    
    lua_pushnumber(L, Math_NAN);
    lua_setglobal(L, "NAN");
}

void LuauEngine::register_godot_functions(lua_State *L) {
    // Override print function to use Godot's print
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int n = lua_gettop(L);  // Number of arguments
        String output;
        
        for (int i = 1; i <= n; i++) {
            if (i > 1) output += " ";
            
            // Convert each argument to string
            if (lua_isnil(L, i)) {
                output += "nil";
            } else if (lua_isboolean(L, i)) {
                output += lua_toboolean(L, i) ? "true" : "false";
            } else if (lua_isnumber(L, i)) {
                double num = lua_tonumber(L, i);
                // Check if it's an integer
                int64_t int_val = (int64_t)num;
                if (num == (double)int_val) {
                    output += String::num_int64(int_val);
                } else {
                    output += String::num(num);
                }
            } else if (lua_isstring(L, i)) {
                output += lua_tostring(L, i);
            } else if (lua_istable(L, i)) {
                // For tables, try to convert to Variant first
                Variant v = LuauMarshal::get_variant(L, i);
                output += String(v);
            } else if (lua_isuserdata(L, i)) {
                // Try to get as variant
                Variant v = LuauMarshal::get_variant(L, i);
                output += String(v);
            } else if (lua_isfunction(L, i)) {
                output += "<function>";
            } else if (lua_isthread(L, i)) {
                output += "<thread>";
            } else {
                output += "<unknown>";
            }
        }
        
        // Use Godot's print function
        UtilityFunctions::print(output);
        
        return 0;
    }, "print");
    lua_setglobal(L, "print");
    
    // Add print_error function
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int n = lua_gettop(L);
        String output;
        
        for (int i = 1; i <= n; i++) {
            if (i > 1) output += " ";
            
            if (lua_isnil(L, i)) {
                output += "nil";
            } else if (lua_isboolean(L, i)) {
                output += lua_toboolean(L, i) ? "true" : "false";
            } else if (lua_isnumber(L, i)) {
                output += String::num(lua_tonumber(L, i));
            } else if (lua_isstring(L, i)) {
                output += lua_tostring(L, i);
            } else {
                // Try to convert to string via Variant
                Variant v = LuauMarshal::get_variant(L, i);
                output += String(v);
            }
        }
        
        UtilityFunctions::printerr(output);
        
        return 0;
    }, "print_error");
    lua_setglobal(L, "print_error");
    
    // Add print_warning function (using push_warning)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int n = lua_gettop(L);
        String output;
        
        for (int i = 1; i <= n; i++) {
            if (i > 1) output += " ";
            
            if (lua_isnil(L, i)) {
                output += "nil";
            } else if (lua_isboolean(L, i)) {
                output += lua_toboolean(L, i) ? "true" : "false";
            } else if (lua_isnumber(L, i)) {
                output += String::num(lua_tonumber(L, i));
            } else if (lua_isstring(L, i)) {
                output += lua_tostring(L, i);
            } else {
                // Try to convert to string via Variant
                Variant v = LuauMarshal::get_variant(L, i);
                output += String(v);
            }
        }
        
        UtilityFunctions::push_warning(output);
        
        return 0;
    }, "print_warning");
    lua_setglobal(L, "print_warning");
}

void LuauEngine::register_godot_globals(lua_State *L) {
    // Register each Godot type
    register_color_class(L);
    register_vector2_class(L);
    register_vector3_class(L);
    register_rect2_class(L);
    register_math_constants(L);
    
    // Register Godot functions (print, etc.)
    register_godot_functions(L);
    
    // TODO: Add more Godot types as needed:
    // - Transform2D
    // - Transform3D
    // - Basis
    // - Quaternion
    // - AABB
    // - Plane
    // - etc.
}

void LuauEngine::init_vm(VMType p_type) {
    lua_State *L = lua_newstate(luauGD_alloc, nullptr);

    luaL_openlibs(L);
    
    // Register Godot globals before sandboxing
    register_godot_globals(L);

    luaL_sandbox(L);

    vms[p_type] = L;
}

void LuauEngine::new_vm() {
    lua_State *L = lua_newstate(luauGD_alloc, nullptr);

    luaL_openlibs(L);
}

LuauEngine::LuauEngine() {
	init_vm(VM_SCRIPT_LOAD);
	init_vm(VM_CORE);
	init_vm(VM_USER);

    if (!singleton) {
        singleton = this;
    }
}

LuauEngine::~LuauEngine() {
	if (singleton == this) {
		singleton = nullptr;
	}

    for (lua_State *&L : vms) {
		luaGD_close(L);
		L = nullptr;
	}
}
