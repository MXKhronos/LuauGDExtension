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
#include "variant/color.h"

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
    
    // ===== MATH FUNCTIONS =====
    
    // Trigonometric functions
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double angle = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::sin(angle));
        return 1;
    }, "sin");
    lua_setglobal(L, "sin");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double angle = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::cos(angle));
        return 1;
    }, "cos");
    lua_setglobal(L, "cos");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double angle = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::tan(angle));
        return 1;
    }, "tan");
    lua_setglobal(L, "tan");
    
    // Hyperbolic functions
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double x = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::sinh(x));
        return 1;
    }, "sinh");
    lua_setglobal(L, "sinh");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double x = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::cosh(x));
        return 1;
    }, "cosh");
    lua_setglobal(L, "cosh");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double x = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::tanh(x));
        return 1;
    }, "tanh");
    lua_setglobal(L, "tanh");
    
    // Inverse trig functions
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double x = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::asin(x));
        return 1;
    }, "asin");
    lua_setglobal(L, "asin");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double x = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::acos(x));
        return 1;
    }, "acos");
    lua_setglobal(L, "acos");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double x = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::atan(x));
        return 1;
    }, "atan");
    lua_setglobal(L, "atan");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double y = luaL_checknumber(L, 1);
        double x = luaL_checknumber(L, 2);
        lua_pushnumber(L, UtilityFunctions::atan2(y, x));
        return 1;
    }, "atan2");
    lua_setglobal(L, "atan2");
    
    // Square root and power
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double x = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::sqrt(x));
        return 1;
    }, "sqrt");
    lua_setglobal(L, "sqrt");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double base = luaL_checknumber(L, 1);
        double exp = luaL_checknumber(L, 2);
        lua_pushnumber(L, UtilityFunctions::pow(base, exp));
        return 1;
    }, "pow");
    lua_setglobal(L, "pow");
    
    // Logarithms
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double x = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::log(x));
        return 1;
    }, "log");
    lua_setglobal(L, "log");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double x = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::exp(x));
        return 1;
    }, "exp");
    lua_setglobal(L, "exp");
    
    // Rounding functions
    lua_pushcfunction(L, [](lua_State *L) -> int {
        if (lua_isnumber(L, 1)) {
            double x = lua_tonumber(L, 1);
            lua_pushnumber(L, UtilityFunctions::floorf(x));
        } else {
            Variant v = LuauMarshal::get_variant(L, 1);
            LuauMarshal::push_variant(L, UtilityFunctions::floor(v));
        }
        return 1;
    }, "floor");
    lua_setglobal(L, "floor");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        if (lua_isnumber(L, 1)) {
            double x = lua_tonumber(L, 1);
            lua_pushnumber(L, UtilityFunctions::ceilf(x));
        } else {
            Variant v = LuauMarshal::get_variant(L, 1);
            LuauMarshal::push_variant(L, UtilityFunctions::ceil(v));
        }
        return 1;
    }, "ceil");
    lua_setglobal(L, "ceil");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        if (lua_isnumber(L, 1)) {
            double x = lua_tonumber(L, 1);
            lua_pushnumber(L, UtilityFunctions::roundf(x));
        } else {
            Variant v = LuauMarshal::get_variant(L, 1);
            LuauMarshal::push_variant(L, UtilityFunctions::round(v));
        }
        return 1;
    }, "round");
    lua_setglobal(L, "round");
    
    // Absolute value
    lua_pushcfunction(L, [](lua_State *L) -> int {
        if (lua_isnumber(L, 1)) {
            double x = lua_tonumber(L, 1);
            lua_pushnumber(L, UtilityFunctions::absf(x));
        } else {
            Variant v = LuauMarshal::get_variant(L, 1);
            LuauMarshal::push_variant(L, UtilityFunctions::abs(v));
        }
        return 1;
    }, "abs");
    lua_setglobal(L, "abs");
    
    // Sign function
    lua_pushcfunction(L, [](lua_State *L) -> int {
        if (lua_isnumber(L, 1)) {
            double x = lua_tonumber(L, 1);
            lua_pushnumber(L, UtilityFunctions::signf(x));
        } else {
            Variant v = LuauMarshal::get_variant(L, 1);
            LuauMarshal::push_variant(L, UtilityFunctions::sign(v));
        }
        return 1;
    }, "sign");
    lua_setglobal(L, "sign");
    
    // Min/Max functions
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int nargs = lua_gettop(L);
        if (nargs < 2) {
            luaL_error(L, "min requires at least 2 arguments");
        }
        
        if (lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
            double result = UtilityFunctions::minf(lua_tonumber(L, 1), lua_tonumber(L, 2));
            for (int i = 3; i <= nargs; i++) {
                if (lua_isnumber(L, i)) {
                    result = UtilityFunctions::minf(result, lua_tonumber(L, i));
                }
            }
            lua_pushnumber(L, result);
        } else {
            // Use variant min for mixed types
            Variant result = LuauMarshal::get_variant(L, 1);
            for (int i = 2; i <= nargs; i++) {
                Variant v = LuauMarshal::get_variant(L, i);
                result = UtilityFunctions::min(result, v);
            }
            LuauMarshal::push_variant(L, result);
        }
        return 1;
    }, "min");
    lua_setglobal(L, "min");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int nargs = lua_gettop(L);
        if (nargs < 2) {
            luaL_error(L, "max requires at least 2 arguments");
        }
        
        if (lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
            double result = UtilityFunctions::maxf(lua_tonumber(L, 1), lua_tonumber(L, 2));
            for (int i = 3; i <= nargs; i++) {
                if (lua_isnumber(L, i)) {
                    result = UtilityFunctions::maxf(result, lua_tonumber(L, i));
                }
            }
            lua_pushnumber(L, result);
        } else {
            // Use variant max for mixed types
            Variant result = LuauMarshal::get_variant(L, 1);
            for (int i = 2; i <= nargs; i++) {
                Variant v = LuauMarshal::get_variant(L, i);
                result = UtilityFunctions::max(result, v);
            }
            LuauMarshal::push_variant(L, result);
        }
        return 1;
    }, "max");
    lua_setglobal(L, "max");
    
    // Clamp function
    lua_pushcfunction(L, [](lua_State *L) -> int {
        if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
            double value = lua_tonumber(L, 1);
            double min_val = lua_tonumber(L, 2);
            double max_val = lua_tonumber(L, 3);
            lua_pushnumber(L, UtilityFunctions::clampf(value, min_val, max_val));
        } else {
            Variant value = LuauMarshal::get_variant(L, 1);
            Variant min_val = LuauMarshal::get_variant(L, 2);
            Variant max_val = LuauMarshal::get_variant(L, 3);
            LuauMarshal::push_variant(L, UtilityFunctions::clamp(value, min_val, max_val));
        }
        return 1;
    }, "clamp");
    lua_setglobal(L, "clamp");
    
    // Interpolation functions
    lua_pushcfunction(L, [](lua_State *L) -> int {
        if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
            double from = lua_tonumber(L, 1);
            double to = lua_tonumber(L, 2);
            double weight = lua_tonumber(L, 3);
            lua_pushnumber(L, UtilityFunctions::lerpf(from, to, weight));
        } else {
            Variant from = LuauMarshal::get_variant(L, 1);
            Variant to = LuauMarshal::get_variant(L, 2);
            Variant weight = LuauMarshal::get_variant(L, 3);
            LuauMarshal::push_variant(L, UtilityFunctions::lerp(from, to, weight));
        }
        return 1;
    }, "lerp");
    lua_setglobal(L, "lerp");
    
    // Angle conversions
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double deg = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::deg_to_rad(deg));
        return 1;
    }, "deg_to_rad");
    lua_setglobal(L, "deg_to_rad");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double rad = luaL_checknumber(L, 1);
        lua_pushnumber(L, UtilityFunctions::rad_to_deg(rad));
        return 1;
    }, "rad_to_deg");
    lua_setglobal(L, "rad_to_deg");
    
    // Random functions
    lua_pushcfunction(L, [](lua_State *L) -> int {
        UtilityFunctions::randomize();
        return 0;
    }, "randomize");
    lua_setglobal(L, "randomize");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushinteger(L, UtilityFunctions::randi());
        return 1;
    }, "randi");
    lua_setglobal(L, "randi");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushnumber(L, UtilityFunctions::randf());
        return 1;
    }, "randf");
    lua_setglobal(L, "randf");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int64_t from = luaL_checkinteger(L, 1);
        int64_t to = luaL_checkinteger(L, 2);
        lua_pushinteger(L, UtilityFunctions::randi_range(from, to));
        return 1;
    }, "randi_range");
    lua_setglobal(L, "randi_range");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double from = luaL_checknumber(L, 1);
        double to = luaL_checknumber(L, 2);
        lua_pushnumber(L, UtilityFunctions::randf_range(from, to));
        return 1;
    }, "randf_range");
    lua_setglobal(L, "randf_range");
    
    // Type checking functions
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double x = luaL_checknumber(L, 1);
        lua_pushboolean(L, UtilityFunctions::is_nan(x));
        return 1;
    }, "is_nan");
    lua_setglobal(L, "is_nan");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        double x = luaL_checknumber(L, 1);
        lua_pushboolean(L, UtilityFunctions::is_inf(x));
        return 1;
    }, "is_inf");
    lua_setglobal(L, "is_inf");
    
    // Type conversion
    lua_pushcfunction(L, [](lua_State *L) -> int {
        Variant v = LuauMarshal::get_variant(L, 1);
        LuauMarshal::push_variant(L, Variant::get_type_name(v.get_type()));
        return 1;
    }, "typeof");
    lua_setglobal(L, "typeof");
    
    // String conversion
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int nargs = lua_gettop(L);
        String result;
        for (int i = 1; i <= nargs; i++) {
            Variant v = LuauMarshal::get_variant(L, i);
            result += UtilityFunctions::str(v);
        }
        LuauMarshal::push_variant(L, result);
        return 1;
    }, "str");
    lua_setglobal(L, "str");
    
    // Instance handling
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int64_t id = luaL_checkinteger(L, 1);
        Object *obj = UtilityFunctions::instance_from_id(id);
        if (obj) {
            LuauMarshal::push_variant(L, Variant(obj));
        } else {
            lua_pushnil(L);
        }
        return 1;
    }, "instance_from_id");
    lua_setglobal(L, "instance_from_id");
    
    // Hash function
    lua_pushcfunction(L, [](lua_State *L) -> int {
        Variant v = LuauMarshal::get_variant(L, 1);
        lua_pushinteger(L, UtilityFunctions::hash(v));
        return 1;
    }, "hash");
    lua_setglobal(L, "hash");
    
    // Variant conversion
    lua_pushcfunction(L, [](lua_State *L) -> int {
        String str = luaL_checkstring(L, 1);
        Variant v = UtilityFunctions::str_to_var(str);
        LuauMarshal::push_variant(L, v);
        return 1;
    }, "str_to_var");
    lua_setglobal(L, "str_to_var");
    
    lua_pushcfunction(L, [](lua_State *L) -> int {
        Variant v = LuauMarshal::get_variant(L, 1);
        String str = UtilityFunctions::var_to_str(v);
        LuauMarshal::push_variant(L, str);
        return 1;
    }, "var_to_str");
    lua_setglobal(L, "var_to_str");
}

void LuauEngine::register_godot_globals(lua_State *L) {
    luau::register_color_class(L);

    // Register each Godot type
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
