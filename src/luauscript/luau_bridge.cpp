#include "luau_bridge.h"

#include <cstring>
#include <godot_cpp/variant/variant.hpp>
#include "variant/builtin_types.h"

using namespace godot;
using namespace luau;

void *LuauBridge::luaL_checkudata(lua_State *L, int p_index, const char *p_tname) {
    void *p = lua_touserdata(L, p_index);

    if (p != NULL) {
        if (lua_getmetatable(L, p_index)) {
            lua_getfield(L, LUA_REGISTRYINDEX, p_tname);
            if (lua_rawequal(L, -1, -2)) {
                lua_pop(L, 2);
                return p;
            }
        }
    }

    luaL_typeerror(L, p_index, p_tname);
    return NULL;
}

void LuauBridge::push_string(lua_State *L, const String &p_str) {
    CharString utf8 = p_str.utf8();
    lua_pushlstring(L, utf8.get_data(), utf8.length());
}

void LuauBridge::push_dictionary(lua_State *L, const Dictionary &p_dict) {
    lua_newtable(L);
    
    Array keys = p_dict.keys();
    for (int i = 0; i < keys.size(); i++) {
        push_variant(L, keys[i]);
        push_variant(L, p_dict[keys[i]]);
        lua_settable(L, -3);
    }
}

void LuauBridge::push_array(lua_State *L, const Array &p_array) {
    lua_newtable(L);
    
    for (int i = 0; i < p_array.size(); i++) {
        lua_pushinteger(L, i + 1); // Lua arrays start at 1
        push_variant(L, p_array[i]);
        lua_settable(L, -3);
    }
}

void LuauBridge::push_variant(lua_State *L, const Variant &p_var) {
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
            lua_pushnumber(L, p_var.operator float());
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
            
        case Variant::DICTIONARY:
            push_dictionary(L, p_var.operator Dictionary());
            break;
            
        case Variant::ARRAY:
            push_array(L, p_var.operator Array());
            break;
            
        case Variant::VECTOR2: {
            Vector2Bridge::push_from(L, p_var.operator Vector2());
            break;
        }

        case Variant::COLOR: {
            ColorBridge::push_from(L, p_var.operator Color());
            break;
        }
        
        // case Variant::OBJECT: { // GD Class
        //     Object *obj = p_var.operator Object*();
        //     if (obj) {
        //         push_object(L, obj);
        //     } else {
        //         lua_pushnil(L);
        //     }
        //     break;
        // }
        
        default: {
            WARN_PRINT("Unsupported Variant type: " + Variant::get_type_name(p_var.get_type()));
            break;
        }
    }
}

String LuauBridge::get_string(lua_State *L, int p_index) {
    size_t len;
    const char *str = lua_tolstring(L, p_index, &len);
    if (str) {
        return String::utf8(str, len);
    }
    return String();
}

Dictionary LuauBridge::get_dictionary(lua_State *L, int p_index) {
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

Array LuauBridge::get_array(lua_State *L, int p_index) {
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

Variant LuauBridge::get_variant(lua_State *L, int p_index) {
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
            // Get the userdata's metatable
            if (!lua_getmetatable(L, p_index)) {
                return Variant(get_dictionary(L, p_index));
            }

            lua_getfield(L, -1, "__type");
            if (lua_isnil(L, -1)) {
                lua_pop(L, 2); // Pop metatable and nil __type
                return Variant(get_dictionary(L, p_index));
            }
            const char* type_name = lua_tostring(L, -1);

            lua_getfield(L, LUA_REGISTRYINDEX, type_name);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 3); // Pop registered metatable, type name, and userdata metatable
                WARN_PRINT("Type not registered: " + String(type_name));
                return Variant(get_dictionary(L, p_index));
            }
        
            bool equal = lua_rawequal(L, -1, -3);
            lua_pop(L, 3);

            if (!equal) {
                WARN_PRINT("LUA_TTABLE metatable does not match registered type: " + String(type_name));
                return Variant(get_dictionary(L, p_index));
            }

            WARN_PRINT("LUA_TTABLE Type is: " + String(type_name));
            if (strcmp(type_name, "Vector2") == 0) {
                return Vector2();
            } else if (strcmp(type_name, "Color") == 0) {
                return Color();
            }

            WARN_PRINT("Unhandled table type: " + String(type_name));
            return Variant(get_dictionary(L, p_index));
        }
        case LUA_TUSERDATA: {
            void* ud = lua_touserdata(L, p_index);
            if (!ud) {
                WARN_PRINT("Invalid userdata");
                return Variant();
            }

            // Get the userdata's metatable
            if (!lua_getmetatable(L, p_index)) {
                WARN_PRINT("Userdata without metatable");
                return Variant();
            }

            // Get the type name
            lua_getfield(L, -1, "__type");
            if (lua_isnil(L, -1)) {
                lua_pop(L, 2); // Pop metatable and nil __type
                WARN_PRINT("Userdata metatable without __type");
                return Variant();
            }
            const char* type_name = lua_tostring(L, -1);

            // Get the registered metatable for this type
            lua_getfield(L, LUA_REGISTRYINDEX, type_name);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 3); // Pop registered metatable, type name, and userdata metatable
                WARN_PRINT("Type not registered: " + String(type_name));
                return Variant();
            }

            // Compare the metatables
            bool equal = lua_rawequal(L, -1, -3);
            lua_pop(L, 3);

            if (!equal) {
                WARN_PRINT("Userdata metatable does not match registered type: " + String(type_name));
                return Variant();
            }

            // Now we know it's a valid userdata of our type
            if (strcmp(type_name, "Vector2") == 0) {
                return Vector2Bridge::get_object(L, p_index);
            } else if (strcmp(type_name, "Color") == 0) {
                return ColorBridge::get_object(L, p_index);
            }

            WARN_PRINT("Unhandled userdata type: " + String(type_name));
            return Variant();
        }
        default:
            return Variant();
    }
};

void LuauBridge::protect_metatable(lua_State* L, int index) {
	lua_pushstring(L, "The metatable is locked");
	lua_setfield(L, index-1, "__metatable");
}


template <class GDV, bool __eq>
void VariantBridge<GDV, __eq>::register_variant (lua_State *L) {
    // Create the metatable
    luaL_newmetatable(L, variant_name);
    LuauBridge::protect_metatable(L, -1);

    lua_pushstring(L, "__type");
    lua_pushstring(L, variant_name);
    lua_settable(L, -3);

    lua_pushstring(L, "__index");
    lua_pushcfunction(L, on_index, "__index");
    lua_settable(L, -3);

    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, on_newindex, "__newindex");
    lua_settable(L, -3);

    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, on_gc, "__gc");
    lua_settable(L, -3);

    lua_pushstring(L, "__call");
    lua_pushcfunction(L, on_call, "__call");
    lua_settable(L, -3);

    if (__eq) {
        lua_pushstring(L, "__eq");
        lua_pushcfunction(L, on_eq, "__eq");
        lua_settable(L, -3);
    }

    lua_pushstring(L, "__tostring");
    lua_pushcfunction(L, on_tostring, "__tostring");
    lua_settable(L, -3);

    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}


//MARK: register_variants
template void VariantBridge<String>::register_variant(lua_State *);
template void VariantBridge<Vector2>::register_variant(lua_State *);
template void VariantBridge<Vector2i>::register_variant(lua_State *);
template void VariantBridge<Rect2>::register_variant(lua_State *);
template void VariantBridge<Rect2i>::register_variant(lua_State *);
template void VariantBridge<Vector3>::register_variant(lua_State *);
template void VariantBridge<Vector3i>::register_variant(lua_State *);
template void VariantBridge<Color>::register_variant(lua_State *);