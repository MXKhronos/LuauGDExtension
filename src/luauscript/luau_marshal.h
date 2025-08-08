#ifndef LUAU_MARSHAL_H
#define LUAU_MARSHAL_H

#include <lua.h>
#include <lualib.h>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/core/object.hpp>

namespace godot {

class LuauMarshal {
public:
    // Push Godot Variant to Lua stack
    static void push_variant(lua_State *L, const Variant &p_var);
    
    // Get Variant from Lua stack
    static Variant get_variant(lua_State *L, int p_index);
    
    // Check if Lua value at index is a Variant
    static bool is_variant(lua_State *L, int p_index);
    
    // Push specific types
    static void push_string(lua_State *L, const String &p_str);
    static void push_object(lua_State *L, Object *p_obj);
    static void push_dictionary(lua_State *L, const Dictionary &p_dict);
    static void push_array(lua_State *L, const Array &p_array);
    
    // Get specific types
    static String get_string(lua_State *L, int p_index);
    static Object* get_object(lua_State *L, int p_index);
    static Dictionary get_dictionary(lua_State *L, int p_index);
    static Array get_array(lua_State *L, int p_index);
};

}

#endif
