#include "luau_marshal.h"
#include "nobind.h"
#include <lua.h>
#include <lualib.h>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include "variant/color.h"

using namespace godot;

// Metatables for userdata types
static const char *GODOT_OBJECT_METATABLE = "Godot.Object";
static const char *GODOT_VARIANT_METATABLE = "Godot.Variant";

void LuauMarshal::push_variant(lua_State *L, const Variant &p_var) {
    switch (p_var.get_type()) {
        case Variant::NIL:
            lua_pushnil(L);
            break;
            
        case Variant::BOOL:
            lua_pushboolean(L, p_var.operator bool());
            break;
            
        case Variant::INT:
            lua_pushinteger(L, p_var.operator int64_t());
            break;
            
        case Variant::FLOAT:
            lua_pushnumber(L, p_var.operator double());
            break;
            
        case Variant::STRING:
            push_string(L, p_var.operator String());
            break;
            
        case Variant::STRING_NAME:
            push_string(L, String(p_var.operator StringName()));
            break;
            
        case Variant::NODE_PATH:
            push_string(L, String(p_var.operator NodePath()));
            break;
            
        case Variant::VECTOR2: {
            Vector2 v = p_var.operator Vector2();
            lua_newtable(L);
            lua_pushnumber(L, v.x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, v.y);
            lua_setfield(L, -2, "y");
            break;
        }
        
        case Variant::VECTOR3: {
            Vector3 v = p_var.operator Vector3();
            lua_newtable(L);
            lua_pushnumber(L, v.x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, v.y);
            lua_setfield(L, -2, "y");
            lua_pushnumber(L, v.z);
            lua_setfield(L, -2, "z");
            break;
        }
        
        case Variant::COLOR: {
            Color c = p_var.operator Color();
            luau::Color(L, c);
            break;
        }
        
        case Variant::DICTIONARY:
            push_dictionary(L, p_var.operator Dictionary());
            break;
            
        case Variant::ARRAY:
            push_array(L, p_var.operator Array());
            break;
            
        case Variant::OBJECT: {
            Object *obj = p_var.operator Object*();
            if (obj) {
                push_object(L, obj);
            } else {
                lua_pushnil(L);
            }
            break;
        }
        
        default: {
            // For complex types not directly supported, store as userdata
            Variant **udata = (Variant**)lua_newuserdata(L, sizeof(Variant*));
            *udata = memnew(Variant(p_var));
            
            // Set metatable for GC and operations
            luaL_getmetatable(L, GODOT_VARIANT_METATABLE);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                luaL_newmetatable(L, GODOT_VARIANT_METATABLE);
                
                // Add __gc metamethod for cleanup
                lua_pushcfunction(L, [](lua_State *L) -> int {
                    Variant **v = (Variant**)lua_touserdata(L, 1);
                    if (v && *v) {
                        memdelete(*v);
                        *v = nullptr;
                    }
                    return 0;
                }, "__gc");
                lua_setfield(L, -2, "__gc");
            }
            lua_setmetatable(L, -2);
            break;
        }
    }
}

Variant LuauMarshal::get_variant(lua_State *L, int p_index) {
    int type = lua_type(L, p_index);
    
    switch (type) {
        case LUA_TNIL:
            return Variant();
            
        case LUA_TBOOLEAN:
            return Variant(lua_toboolean(L, p_index) != 0);
            
        case LUA_TNUMBER: {
            double num = lua_tonumber(L, p_index);
            // Check if it's effectively an integer by comparing with truncated value
            int64_t int_val = (int64_t)num;
            if (num == (double)int_val) {
                return Variant(int_val);
            } else {
                return Variant(num);
            }
        }
            
        case LUA_TSTRING:
            return Variant(get_string(L, p_index));
            
        case LUA_TTABLE: {
            // First check if it has a __godot_variant field (wrapped Godot object)
            lua_getfield(L, p_index, "__godot_variant");
            if (lua_isuserdata(L, -1)) {
                // This is a wrapped Godot object, extract the actual object
                void *udata = lua_touserdata(L, -1);
                lua_pop(L, 1); // Pop the userdata
                
                // Try to determine what kind of object it is by checking for known properties
                // Check for Color (has r, g, b, a)
                lua_getfield(L, p_index, "__godot_variant");
                if (lua_isuserdata(L, -1)) {
                    // For now, we'll check if it's a Color by trying to access color properties
                    // through the userdata (this assumes it's a Color)
                    ::godot::Color *color = (::godot::Color*)udata;
                    if (color) {
                        lua_pop(L, 1);
                        return Variant(Color(color->r, color->g, color->b, color->a));
                    }
                }
                lua_pop(L, 1);
                
                // TODO: Add checks for other wrapped types (Vector2, Vector3, etc.)
                // For now, fall through to regular table handling
            } else {
                lua_pop(L, 1); // Pop nil
            }
            
            // Check if it's an array (has integer indices)
            lua_pushinteger(L, 1);
            lua_gettable(L, p_index);
            bool is_array = !lua_isnil(L, -1);
            lua_pop(L, 1);
            
            if (is_array) {
                return Variant(get_array(L, p_index));
            } else {
                return Variant(get_dictionary(L, p_index));
            }
        }
        
        case LUA_TUSERDATA: {
            // Check if it's a Godot Object
            if (luaL_getmetatable(L, GODOT_OBJECT_METATABLE)) {
                lua_getmetatable(L, p_index);
                if (lua_rawequal(L, -1, -2)) {
                    lua_pop(L, 2);
                    Object **obj = (Object**)lua_touserdata(L, p_index);
                    return Variant(*obj);
                }
                lua_pop(L, 2);
            }
            
            // Check if it's a wrapped Variant
            if (luaL_getmetatable(L, GODOT_VARIANT_METATABLE)) {
                lua_getmetatable(L, p_index);
                if (lua_rawequal(L, -1, -2)) {
                    lua_pop(L, 2);
                    Variant **var = (Variant**)lua_touserdata(L, p_index);
                    return **var;
                }
                lua_pop(L, 2);
            }
            
            return Variant();
        }
        
        default:
            return Variant();
    }
}

bool LuauMarshal::is_variant(lua_State *L, int p_index) {
    int type = lua_type(L, p_index);
    return type != LUA_TNONE && type != LUA_TFUNCTION && type != LUA_TTHREAD;
}

void LuauMarshal::push_string(lua_State *L, const String &p_str) {
    CharString utf8 = p_str.utf8();
    lua_pushlstring(L, utf8.get_data(), utf8.length());
}

void LuauMarshal::push_object(lua_State *L, Object *p_obj) {
    if (!p_obj) {
        lua_pushnil(L);
        return;
    }
    
    Object **udata = (Object**)lua_newuserdata(L, sizeof(Object*));
    *udata = p_obj;
    
    // Set metatable for the object
    luaL_getmetatable(L, GODOT_OBJECT_METATABLE);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        luaL_newmetatable(L, GODOT_OBJECT_METATABLE);
        
        // Add __index for property/method access
        lua_pushcfunction(L, [](lua_State *L) -> int {
            Object **obj = (Object**)lua_touserdata(L, 1);
            const char *key = lua_tostring(L, 2);
            
            if (obj && *obj && key) {
                StringName prop_name(key);
                
                // Try to get property
                Variant value = (*obj)->get(prop_name);
                if (value.get_type() != Variant::NIL) {
                    LuauMarshal::push_variant(L, value);
                    return 1;
                }
                
                // Check for methods
                if (nobind::ClassDB::get_singleton()->class_has_method((*obj)->get_class(), prop_name, false)) {
                    // Push upvalues first (before creating closure)
                    lua_pushlightuserdata(L, *obj);
                    lua_pushstring(L, key);
                    
                    // Return a function that will call the method
                    lua_pushcclosure(L, [](lua_State *L) -> int {
                        // Get the object from the upvalue
                        Object *obj = (Object*)lua_touserdata(L, lua_upvalueindex(1));
                        const char *method_name = lua_tostring(L, lua_upvalueindex(2));
                        
                        if (obj && method_name) {
                            // Collect arguments
                            int arg_count = lua_gettop(L);
                            Array args;
                            for (int i = 1; i <= arg_count; i++) {
                                args.append(LuauMarshal::get_variant(L, i));
                            }
                            
                            // Call the method
                            Variant result = obj->callv(StringName(method_name), args);
                            
                            // Push result
                            LuauMarshal::push_variant(L, result);
                            return 1;
                        }
                        
                        return 0;
                    }, "method_call", 2);
                    
                    return 1;
                }
            }
            
            lua_pushnil(L);
            return 1;
        }, "__index");
        lua_setfield(L, -2, "__index");
        
        // Add __newindex for property setting
        lua_pushcfunction(L, [](lua_State *L) -> int {
            Object **obj = (Object**)lua_touserdata(L, 1);
            const char *key = lua_tostring(L, 2);
            
            if (obj && *obj && key) {
                StringName prop_name(key);
                Variant value = LuauMarshal::get_variant(L, 3);
                (*obj)->set(prop_name, value);
            }
            
            return 0;
        }, "__newindex");
        lua_setfield(L, -2, "__newindex");
    }
    lua_setmetatable(L, -2);
}

void LuauMarshal::push_dictionary(lua_State *L, const Dictionary &p_dict) {
    lua_newtable(L);
    
    Array keys = p_dict.keys();
    for (int i = 0; i < keys.size(); i++) {
        push_variant(L, keys[i]);
        push_variant(L, p_dict[keys[i]]);
        lua_settable(L, -3);
    }
}

void LuauMarshal::push_array(lua_State *L, const Array &p_array) {
    lua_newtable(L);
    
    for (int i = 0; i < p_array.size(); i++) {
        lua_pushinteger(L, i + 1); // Lua arrays start at 1
        push_variant(L, p_array[i]);
        lua_settable(L, -3);
    }
}

String LuauMarshal::get_string(lua_State *L, int p_index) {
    size_t len;
    const char *str = lua_tolstring(L, p_index, &len);
    if (str) {
        return String::utf8(str, len);
    }
    return String();
}

Object* LuauMarshal::get_object(lua_State *L, int p_index) {
    if (lua_type(L, p_index) != LUA_TUSERDATA) {
        return nullptr;
    }
    
    Object **obj = (Object**)lua_touserdata(L, p_index);
    return obj ? *obj : nullptr;
}

Dictionary LuauMarshal::get_dictionary(lua_State *L, int p_index) {
    Dictionary dict;
    
    if (lua_type(L, p_index) != LUA_TTABLE) {
        return dict;
    }
    
    lua_pushnil(L); // First key
    while (lua_next(L, p_index) != 0) {
        // Key is at -2, value at -1
        Variant key = get_variant(L, -2);
        Variant value = get_variant(L, -1);
        dict[key] = value;
        
        lua_pop(L, 1); // Remove value, keep key for next iteration
    }
    
    return dict;
}

Array LuauMarshal::get_array(lua_State *L, int p_index) {
    Array arr;
    
    if (lua_type(L, p_index) != LUA_TTABLE) {
        return arr;
    }
    
    // Get length of array
    int len = lua_objlen(L, p_index);
    
    for (int i = 1; i <= len; i++) {
        lua_pushinteger(L, i);
        lua_gettable(L, p_index);
        arr.append(get_variant(L, -1));
        lua_pop(L, 1);
    }
    
    return arr;
}
