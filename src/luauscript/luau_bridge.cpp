#include "luau_bridge.h"

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/color_names.inc.hpp>

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



//MARK: Vector2
template void VariantBridge<Vector2>::register_variant(lua_State *);

template<>
const char* VariantBridge<Vector2>::variant_name("Vector2");

const luaL_Reg Vector2Bridge::static_library[] = {
    {"from_angle", from_angle},
	{NULL, NULL}
};

void Vector2Bridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

int Vector2Bridge::from_angle(lua_State* L) {
    float angle = luaL_checknumber(L, 1);
    Vector2 result = Vector2::from_angle(angle);
    push_from(L, result);
    return 1;
}

template<>
int VariantBridge<Vector2>::on_index(lua_State* L, const Vector2& object, const char* key) {
    WARN_PRINT("on_index Vector2: " + String(key));
    return 1;
}

template<>
int VariantBridge<Vector2>::on_newindex(lua_State* L, Vector2& object, const char* key) {
    WARN_PRINT("on_newindex Vector2: " + String(key));
    return 1;
}

template<>
int VariantBridge<Vector2>::on_call(lua_State* L) {
    const int argc = lua_gettop(L)-1;

    WARN_PRINT("on_call Vector2 with " + itos(argc) + " arguments");

    return 1;
}

//MARK: Rect2
template void VariantBridge<Rect2>::register_variant(lua_State *);

template<>
const char* VariantBridge<Rect2>::variant_name("Rect2");

const luaL_Reg Rect2Bridge::static_library[] = {
	{NULL, NULL}
};

void Rect2Bridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Rect2>::on_index(lua_State* L, const Rect2& object, const char* key) {
    WARN_PRINT("on_index Rect2: " + String(key));
    return 1;
}

template<>
int VariantBridge<Rect2>::on_newindex(lua_State* L, Rect2& object, const char* key) {
    WARN_PRINT("on_newindex Rect2: " + String(key));
    return 1;
}

template<>
int VariantBridge<Rect2>::on_call(lua_State* L) {
    const int argc = lua_gettop(L)-1;

    WARN_PRINT("on_call Rect2 with " + itos(argc) + " arguments");

    return 1;
}

//MARK: Color
template void VariantBridge<Color>::register_variant(lua_State *);

template<>
const char* VariantBridge<Color>::variant_name("Color");

const luaL_Reg ColorBridge::static_library[] = {
    {"hex", hex},
    {"lerp", lerp},
	{NULL, NULL}
};

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

int ColorBridge::hex(lua_State* L) {
    uint32_t hex_val = (uint32_t)luaL_checknumber(L, 1);
    Color result = Color::hex(hex_val);
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

template<>
int VariantBridge<Color>::on_index(lua_State* L, const Color& object, const char* key) {
    WARN_PRINT("on_index Color: " + String(key));
    lua_pushnil(L);
    return 1;
}

template<>
int VariantBridge<Color>::on_newindex(lua_State* L, Color& object, const char* key) {
    WARN_PRINT("on_newindex Color: " + String(key));
    return 1;
}

template<>
int VariantBridge<Color>::on_call(lua_State* L) {
    const int argc = lua_gettop(L)-1;

    if (argc == 1) {
        Variant a1 = LuauBridge::get_variant(L, 2);
        if (a1.get_type() == Variant::COLOR) {
            push_from(L, Color(a1));
            return 1;

        } else if (a1.get_type() == Variant::STRING) {
            push_from(L, Color(String(a1)));
            return 1;

        } else {
            luaL_error(L, "Invalid argument type for Color constructor");
            return 1;
        }

    } else if (argc == 2) {
        Variant a1 = LuauBridge::get_variant(L, 2);
        Variant a2 = LuauBridge::get_variant(L, 3);
        if (a1.get_type() == Variant::COLOR && a2.get_type() == Variant::FLOAT) {
            push_from(L, Color(a1.operator Color(), a2.operator float()));
            return 1;
            
        } else if (a1.get_type() == Variant::STRING && a2.get_type() == Variant::FLOAT) {
            push_from(L, Color(String(a1), a2.operator float()));
            return 1;
            
        } else {
            luaL_error(L, "Invalid argument type for Color constructor");
            return 1;
        }

    } else if (argc == 3) {
        Variant a1 = LuauBridge::get_variant(L, 2);
        Variant a2 = LuauBridge::get_variant(L, 3);
        Variant a3 = LuauBridge::get_variant(L, 4);

        if (a1.get_type() == Variant::FLOAT && a2.get_type() == Variant::FLOAT && a3.get_type() == Variant::FLOAT) {
            push_from(L, Color(a1.operator float(), a2.operator float(), a3.operator float()));
            return 1;
            
        } else {
            luaL_error(L, "Invalid argument type for Color constructor");
            return 1;
        }

    } else if (argc == 4) {
        Variant a1 = LuauBridge::get_variant(L, 2);
        Variant a2 = LuauBridge::get_variant(L, 3);
        Variant a3 = LuauBridge::get_variant(L, 4);
        Variant a4 = LuauBridge::get_variant(L, 5);

        if (a1.get_type() == Variant::FLOAT && a2.get_type() == Variant::FLOAT && a3.get_type() == Variant::FLOAT && a4.get_type() == Variant::FLOAT) {
            push_from(L, Color(a1.operator float(), a2.operator float(), a3.operator float(), a4.operator float()));
            return 1;
            
        } else {
            luaL_error(L, "Invalid argument type for Color constructor");
            return 1;
        }
    }

    push_new(L);
    return 1;
}