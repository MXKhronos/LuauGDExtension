#ifndef LUAU_VARIANT_ARRAY_H
#define LUAU_VARIANT_ARRAY_H

#include <lua.h>
#include <lualib.h>
#include "luauscript/luau_bridge.h"

namespace godot {


//MARK: Array
class ArrayBridge: public VariantBridge<Array> {
    friend class VariantBridge <Array>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};



//MARK: PackedByteArray
class PackedByteArrayBridge: public VariantBridge<PackedByteArray> {
    friend class VariantBridge <PackedByteArray>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};



//MARK: PackedInt32Array
class PackedInt32ArrayBridge: public VariantBridge<PackedInt32Array> {
    friend class VariantBridge <PackedInt32Array>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};



//MARK: PackedInt64Array
class PackedInt64ArrayBridge: public VariantBridge<PackedInt64Array> {
    friend class VariantBridge <PackedInt64Array>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};



//MARK: PackedFloat32Array
class PackedFloat32ArrayBridge: public VariantBridge<PackedFloat32Array> {
    friend class VariantBridge <PackedFloat32Array>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};



//MARK: PackedFloat64Array
class PackedFloat64ArrayBridge: public VariantBridge<PackedFloat64Array> {
    friend class VariantBridge <PackedFloat64Array>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};



//MARK: PackedStringArray
class PackedStringArrayBridge: public VariantBridge<PackedStringArray> {
    friend class VariantBridge <PackedStringArray>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};



//MARK: PackedVector2Array
class PackedVector2ArrayBridge: public VariantBridge<PackedVector2Array> {
    friend class VariantBridge <PackedVector2Array>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};



//MARK: PackedVector3Array
class PackedVector3ArrayBridge: public VariantBridge<PackedVector3Array> {
    friend class VariantBridge <PackedVector3Array>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};



//MARK: PackedVector4Array
class PackedVector4ArrayBridge: public VariantBridge<PackedVector4Array> {
    friend class VariantBridge <PackedVector4Array>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};



//MARK: PackedColorArray
class PackedColorArrayBridge: public VariantBridge<PackedColorArray> {
    friend class VariantBridge <PackedColorArray>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};


};

#endif // LUAU_VARIANT_ARRAY_H