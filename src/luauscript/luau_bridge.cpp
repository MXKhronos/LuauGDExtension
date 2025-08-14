#include "luau_bridge.h"

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
            Vector2Bridge::pushNewObject(L, p_var.operator Vector2());
            break;
        }

        case Variant::COLOR: {
            ColorBridge::pushNewObject(L, p_var.operator Color());
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
            // Get the type name from the metatable
            if (!lua_getmetatable(L, p_index)) {
                return Variant();
            }
            lua_getfield(L, -1, "__type");

            if (lua_isnil(L, -1)) {
                lua_pop(L, 2); // Pop metatable and nil __type
                return Variant();
            }

            const char* type_name = lua_tostring(L, -1);
            lua_pop(L, 2); // Pop metatable and __type string

            Variant varient = LuauBridge::luaL_checkudata(L, p_index, type_name);
            return varient;
        }
        
        default:
            return Variant();
    }
};

void LuauBridge::protect_metatable(lua_State* thread, int index) {
	lua_pushstring(thread, "The metatable is locked");
	lua_setfield(thread, index-1, "__metatable");
}


template <class Variant, bool __eq>
inline void VariantBridge<Variant, __eq>::registerVariant(lua_State *L) {
    luaL_newmetatable(L, variantName);
    LuauBridge::protect_metatable(L, -1);

    lua_pushstring(L, "__type");
    lua_pushstring(L, variantName);
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


//MARK: Vector2
template void VariantBridge<godot::Vector2>::registerVariant(lua_State *);

template<>
const char* VariantBridge<godot::Vector2>::variantName("Vector2");

const luaL_Reg Vector2Bridge::staticLibrary[] = {
    {"from_angle", from_angle},
	{NULL, NULL}
};

void Vector2Bridge::registerVariantClass(lua_State* L) {
    luaL_register(L, variantName, staticLibrary);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

int Vector2Bridge::from_angle(lua_State* L) {
    float angle = luaL_checknumber(L, 1);
    Vector2 result = Vector2::from_angle(angle);
    pushNewObject(L, result);
    return 1;
}


//MARK: Color
template void VariantBridge<godot::Color>::registerVariant(lua_State *);

template<>
const char* VariantBridge<godot::Color>::variantName("Color");

const luaL_Reg ColorBridge::staticLibrary[] = {
    {"hex", hex},
	{NULL, NULL}
};

void ColorBridge::registerVariantClass(lua_State* L) {
    luaL_register(L, variantName, staticLibrary);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

int ColorBridge::hex(lua_State* L) {
    uint32_t hex_val = (uint32_t)luaL_checknumber(L, 1);
    Color result = Color::hex(hex_val);
    pushNewObject(L, result);
    return 1;
}

template<>
int VariantBridge<godot::Color>::on_newindex(godot::Color& object, const char* name, lua_State* L) {
    return 1;
}