
#include "array.h"

#include <godot_cpp/variant/array.hpp>

using namespace godot;

//MARK: Array
template<>
const char* VariantBridge<Array>::variant_name("Array");

const luaL_Reg ArrayBridge::static_library[] = {
	{NULL, NULL}
};

void ArrayBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Array>::on_index(lua_State* L, const Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Array>::on_newindex(lua_State* L, Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Array>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::DICTIONARY: {
                Array array = LuauBridge::get_array(L, 2);
                push_from(L, array);
                return 1;
            }
            case Variant::PACKED_BYTE_ARRAY: {
                push_from(L, v.operator Array());
                return 1;
            }
            case Variant::PACKED_FLOAT32_ARRAY: {
                push_from(L, v.operator Array());
                return 1;
            }
            case Variant::PACKED_FLOAT64_ARRAY: {
                push_from(L, v.operator Array());
                return 1;
            }
            case Variant::PACKED_INT32_ARRAY: {
                push_from(L, v.operator Array());
                return 1;
            }
            case Variant::PACKED_INT64_ARRAY: {
                push_from(L, v.operator Array());
                return 1;
            }
            case Variant::PACKED_STRING_ARRAY: {
                push_from(L, v.operator Array());
                return 1;
            }
            case Variant::PACKED_VECTOR2_ARRAY: {
                push_from(L, v.operator Array());
                return 1;
            }
            case Variant::PACKED_VECTOR3_ARRAY: {
                push_from(L, v.operator Array());
                return 1;
            }
            case Variant::PACKED_VECTOR4_ARRAY: {
                push_from(L, v.operator Array());
                return 1;
            }
            case Variant::PACKED_COLOR_ARRAY: {
                push_from(L, v.operator Array());
                return 1;
            }
        };

    } else if (argc == 4) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);
        Variant v3 = LuauBridge::get_variant(L, 4);
        Variant v4 = LuauBridge::get_variant(L, 5);

        if (v1.get_type() == Variant::DICTIONARY 
            && v2.get_type() == Variant::INT 
            && v3.get_type() == Variant::STRING_NAME 
            && v4.get_type() == Variant::NIL
        ) {
            push_from(L, Array(
                v1.operator Array(), 
                v2.operator int(), 
                v3.operator StringName(), 
                v4
            ));
            return 1;
        }
    }

    is_valid = false;
    return 1;
}




//MARK: PackedByteArray
template<>
const char* VariantBridge<PackedByteArray>::variant_name("PackedByteArray");

const luaL_Reg PackedByteArrayBridge::static_library[] = {
	{NULL, NULL}
};

void PackedByteArrayBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<PackedByteArray>::on_index(lua_State* L, const PackedByteArray& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedByteArray>::on_newindex(lua_State* L, PackedByteArray& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedByteArray>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PACKED_BYTE_ARRAY: {
                push_from(L, v.operator PackedByteArray());
                return 1;
            }
            case Variant::DICTIONARY: {
                Array array = LuauBridge::get_array(L, 2);
                push_from(L, array);
                return 1;
            }
        };
    }
    
    is_valid = false;
    return 1;
}



//MARK: PackedInt32Array
template<>
const char* VariantBridge<PackedInt32Array>::variant_name("PackedInt32Array");

const luaL_Reg PackedInt32ArrayBridge::static_library[] = {
    {NULL, NULL}
};

void PackedInt32ArrayBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<PackedInt32Array>::on_index(lua_State* L, const PackedInt32Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedInt32Array>::on_newindex(lua_State* L, PackedInt32Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedInt32Array>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PACKED_INT32_ARRAY: {
                push_from(L, v.operator PackedInt32Array());
                return 1;
            }
            case Variant::DICTIONARY: {
                Array array = LuauBridge::get_array(L, 2);
                push_from(L, array);
                return 1;
            }
        };
    }
    
    is_valid = false;
    return 1;
}



//MARK: PackedInt64Array
template<>
const char* VariantBridge<PackedInt64Array>::variant_name("PackedInt64Array");

const luaL_Reg PackedInt64ArrayBridge::static_library[] = {
    {NULL, NULL}
};

void PackedInt64ArrayBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<PackedInt64Array>::on_index(lua_State* L, const PackedInt64Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedInt64Array>::on_newindex(lua_State* L, PackedInt64Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedInt64Array>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PACKED_INT64_ARRAY: {
                push_from(L, v.operator PackedInt64Array());
                return 1;
            }
            case Variant::DICTIONARY: {
                Array array = LuauBridge::get_array(L, 2);
                push_from(L, array);
                return 1;
            }
        };
    }
    
    is_valid = false;
    return 1;
}




//MARK: PackedFloat32Array
template<>
const char* VariantBridge<PackedFloat32Array>::variant_name("PackedFloat32Array");

const luaL_Reg PackedFloat32ArrayBridge::static_library[] = {
    {NULL, NULL}
};

void PackedFloat32ArrayBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<PackedFloat32Array>::on_index(lua_State* L, const PackedFloat32Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedFloat32Array>::on_newindex(lua_State* L, PackedFloat32Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedFloat32Array>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PACKED_FLOAT32_ARRAY: {
                push_from(L, v.operator PackedFloat32Array());
                return 1;
            }
            case Variant::DICTIONARY: {
                Array array = LuauBridge::get_array(L, 2);
                push_from(L, array);
                return 1;
            }
        };
    }
    
    is_valid = false;
    return 1;
}



//MARK: PackedFloat64Array
template<>
const char* VariantBridge<PackedFloat64Array>::variant_name("PackedFloat64Array");

const luaL_Reg PackedFloat64ArrayBridge::static_library[] = {
    {NULL, NULL}
};

void PackedFloat64ArrayBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<PackedFloat64Array>::on_index(lua_State* L, const PackedFloat64Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedFloat64Array>::on_newindex(lua_State* L, PackedFloat64Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedFloat64Array>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PACKED_FLOAT64_ARRAY: {
                push_from(L, v.operator PackedFloat64Array());
                return 1;
            }
            case Variant::DICTIONARY: {
                Array array = LuauBridge::get_array(L, 2);
                push_from(L, array);
                return 1;
            }
        };
    }
    
    is_valid = false;
    return 1;
}




//MARK: PackedStringArray
template<>
const char* VariantBridge<PackedStringArray>::variant_name("PackedStringArray");

const luaL_Reg PackedStringArrayBridge::static_library[] = {
    {NULL, NULL}
};

void PackedStringArrayBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<PackedStringArray>::on_index(lua_State* L, const PackedStringArray& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedStringArray>::on_newindex(lua_State* L, PackedStringArray& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedStringArray>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PACKED_STRING_ARRAY: {
                push_from(L, v.operator PackedStringArray());
                return 1;
            }
            case Variant::DICTIONARY: {
                Array array = LuauBridge::get_array(L, 2);
                push_from(L, array);
                return 1;
            }
        };
    }
    
    is_valid = false;
    return 1;
}



//MARK: PackedVector2Array
template<>
const char* VariantBridge<PackedVector2Array>::variant_name("PackedVector2Array");

const luaL_Reg PackedVector2ArrayBridge::static_library[] = {
    {NULL, NULL}
};

void PackedVector2ArrayBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<PackedVector2Array>::on_index(lua_State* L, const PackedVector2Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedVector2Array>::on_newindex(lua_State* L, PackedVector2Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedVector2Array>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PACKED_VECTOR2_ARRAY: {
                push_from(L, v.operator PackedVector2Array());
                return 1;
            }
            case Variant::DICTIONARY: {
                Array array = LuauBridge::get_array(L, 2);
                push_from(L, array);
                return 1;
            }
        };
    }
    
    is_valid = false;
    return 1;
}



//MARK: PackedVector3Array
template<>
const char* VariantBridge<PackedVector3Array>::variant_name("PackedVector3Array");

const luaL_Reg PackedVector3ArrayBridge::static_library[] = {
    {NULL, NULL}
};

void PackedVector3ArrayBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<PackedVector3Array>::on_index(lua_State* L, const PackedVector3Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedVector3Array>::on_newindex(lua_State* L, PackedVector3Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedVector3Array>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PACKED_VECTOR3_ARRAY: {
                push_from(L, v.operator PackedVector3Array());
                return 1;
            }
            case Variant::DICTIONARY: {
                Array array = LuauBridge::get_array(L, 2);
                push_from(L, array);
                return 1;
            }
        };
    }
    
    is_valid = false;
    return 1;
}



//MARK: PackedVector4Array
template<>
const char* VariantBridge<PackedVector4Array>::variant_name("PackedVector4Array");

const luaL_Reg PackedVector4ArrayBridge::static_library[] = {
    {NULL, NULL}
};

void PackedVector4ArrayBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<PackedVector4Array>::on_index(lua_State* L, const PackedVector4Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedVector4Array>::on_newindex(lua_State* L, PackedVector4Array& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedVector4Array>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PACKED_VECTOR4_ARRAY: {
                push_from(L, v.operator PackedVector4Array());
                return 1;
            }
            case Variant::DICTIONARY: {
                Array array = LuauBridge::get_array(L, 2);
                push_from(L, array);
                return 1;
            }
        };
    }
    
    is_valid = false;
    return 1;
}



//MARK: PackedColorArray
template<>
const char* VariantBridge<PackedColorArray>::variant_name("PackedColorArray");

const luaL_Reg PackedColorArrayBridge::static_library[] = {
    {NULL, NULL}
};

void PackedColorArrayBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<PackedColorArray>::on_index(lua_State* L, const PackedColorArray& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedColorArray>::on_newindex(lua_State* L, PackedColorArray& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<PackedColorArray>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::PACKED_COLOR_ARRAY: {
                push_from(L, v.operator PackedColorArray());
                return 1;
            }
            case Variant::DICTIONARY: {
                Array array = LuauBridge::get_array(L, 2);
                push_from(L, array);
                return 1;
            }
        };
    }
    
    is_valid = false;
    return 1;
}
