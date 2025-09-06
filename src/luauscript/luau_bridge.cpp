#include "luau_bridge.h"

#include <godot_cpp/variant/variant.hpp>
#include "variant/builtin_types.h"

using namespace godot;

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
            return Variant(num); //Default to VARIANT::FLOAT
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
            String type_str(type_name);
            if (type_str == "Vector2") {
                return Vector2();
            } else if (type_str == "Color") {
                return Color();
            }

            WARN_PRINT("Unhandled table type: " + String(type_name) + " -> defaulted as dictionary");
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

            String type_str(type_name);
            if (type_str == "Vector2") {
                return Vector2Bridge::get_object(L, p_index);
            } else if (type_str == "Vector2i") {
                return Vector2iBridge::get_object(L, p_index);
            } else if (type_str == "Rect2") {
                return Rect2Bridge::get_object(L, p_index);
            } else if (type_str == "Rect2i") {
                return Rect2iBridge::get_object(L, p_index);
            } else if (type_str == "Vector3") {
                return Vector3Bridge::get_object(L, p_index);
            } else if (type_str == "Vector3i") {
                return Vector3iBridge::get_object(L, p_index);
            } else if (type_str == "Transform2D") {
                return Transform2DBridge::get_object(L, p_index);
            } else if (type_str == "Vector4") {
                return Vector4Bridge::get_object(L, p_index);
            } else if (type_str == "Vector4i") {
                return Vector4iBridge::get_object(L, p_index);
            } else if (type_str == "Plane") {
                return PlaneBridge::get_object(L, p_index);
            } else if (type_str == "Quaternion") {
                return QuaternionBridge::get_object(L, p_index);
            } else if (type_str == "AABB") {
                return AABBBridge::get_object(L, p_index);
            } else if (type_str == "Basis") {
                return BasisBridge::get_object(L, p_index);
            } else if (type_str == "Transform3D") {
                return Transform3DBridge::get_object(L, p_index);
            } else if (type_str == "Projection") {
                return ProjectionBridge::get_object(L, p_index);

            } else if (type_str == "Color") {
                return ColorBridge::get_object(L, p_index);
            } else if (type_str == "StringName") {
                return StringNameBridge::get_object(L, p_index);
            } else if (type_str == "RID") {
                return RIDBridge::get_object(L, p_index);
            } else if (type_str == "Callable") {
                return CallableBridge::get_object(L, p_index);
            } else if (type_str == "Signal") {
                return SignalBridge::get_object(L, p_index);
            } else if (type_str == "Dictionary") {
                return DictionaryBridge::get_object(L, p_index);
            } else if (type_str == "Array") {
                return ArrayBridge::get_object(L, p_index);

            } else if (type_str == "PackedByteArray") {
                return PackedByteArrayBridge::get_object(L, p_index);
            } else if (type_str == "PackedInt32Array") {
                return PackedInt32ArrayBridge::get_object(L, p_index);
            } else if (type_str == "PackedInt64Array") {
                return PackedInt64ArrayBridge::get_object(L, p_index);
            } else if (type_str == "PackedFloat32Array") {
                return PackedFloat32ArrayBridge::get_object(L, p_index);
            } else if (type_str == "PackedFloat64Array") {
                return PackedFloat64ArrayBridge::get_object(L, p_index);
            } else if (type_str == "PackedStringArray") {
                return PackedStringArrayBridge::get_object(L, p_index);
            } else if (type_str == "PackedVector2Array") {
                return PackedVector2ArrayBridge::get_object(L, p_index);
            } else if (type_str == "PackedVector3Array") {
                return PackedVector3ArrayBridge::get_object(L, p_index);
            } else if (type_str == "PackedVector4Array") {
                return PackedVector4ArrayBridge::get_object(L, p_index);
            } else if (type_str == "PackedColorArray") {
                return PackedColorArrayBridge::get_object(L, p_index);
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
template void VariantBridge<Transform2D>::register_variant(lua_State *);
template void VariantBridge<Vector4>::register_variant(lua_State *);
template void VariantBridge<Vector4i>::register_variant(lua_State *);
template void VariantBridge<Plane>::register_variant(lua_State *);
template void VariantBridge<Quaternion>::register_variant(lua_State *);
template void VariantBridge<AABB>::register_variant(lua_State *);
template void VariantBridge<Basis>::register_variant(lua_State *);
template void VariantBridge<Transform3D>::register_variant(lua_State *);
template void VariantBridge<Projection>::register_variant(lua_State *);

template void VariantBridge<Color>::register_variant(lua_State *);
template void VariantBridge<StringName>::register_variant(lua_State *);
template void VariantBridge<NodePath>::register_variant(lua_State *);
template void VariantBridge<RID>::register_variant(lua_State *);
template void VariantBridge<Callable>::register_variant(lua_State *);
template void VariantBridge<Signal>::register_variant(lua_State *);
template void VariantBridge<Dictionary>::register_variant(lua_State *);
template void VariantBridge<Array>::register_variant(lua_State *);

template void VariantBridge<PackedByteArray>::register_variant(lua_State *);
template void VariantBridge<PackedInt32Array>::register_variant(lua_State *);
template void VariantBridge<PackedInt64Array>::register_variant(lua_State *);
template void VariantBridge<PackedFloat32Array>::register_variant(lua_State *);
template void VariantBridge<PackedFloat64Array>::register_variant(lua_State *);
template void VariantBridge<PackedStringArray>::register_variant(lua_State *);
template void VariantBridge<PackedVector2Array>::register_variant(lua_State *);
template void VariantBridge<PackedVector3Array>::register_variant(lua_State *);
template void VariantBridge<PackedVector4Array>::register_variant(lua_State *);
template void VariantBridge<PackedColorArray>::register_variant(lua_State *);
