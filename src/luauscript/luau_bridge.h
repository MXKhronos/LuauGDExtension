#ifndef LUAU_BRIDGE_H
#define LUAU_BRIDGE_H

#include <godot_cpp/variant/variant.hpp>
#include <lua.h>
#include <lualib.h>

namespace godot {
namespace luau {


//MARK: LuauBridge
class LuauBridge {
    public:
        static void *luaL_checkudata(lua_State *L, int p_index, const char *p_tname);

        static void push_string(lua_State *L, const String &p_str);
        static void push_dictionary(lua_State *L, const Dictionary &p_dict);
        static void push_array(lua_State *L, const Array &p_array);
        static void push_variant(lua_State *L, const Variant &p_var);

        static String get_string(lua_State *L, int p_index);
        static Dictionary get_dictionary(lua_State *L, int p_index);
        static Array get_array(lua_State *L, int p_index);
        static Variant get_variant(lua_State *L, int p_index);

        static void protect_metatable(lua_State* thread, int index);
};


//MARK: VariantBridge
template<class GDV, bool __eq = true>
class VariantBridge {
public:
    static const char* variantName;

    static GDV* pushNewObject(lua_State* L) {
        GDV* ud = (GDV*)lua_newuserdata(L, sizeof(GDV));
        new (ud) GDV();

        luaL_getmetatable(L, variantName);
        lua_setmetatable(L, -2);

        return ud;
    }

    // 1 args
    static GDV* pushNewObject(lua_State* L, const Variant& arg1) {
        GDV* ud = (GDV*)lua_newuserdata(L, sizeof(GDV));
        new (ud) GDV(arg1);

        luaL_getmetatable(L, variantName);
        lua_setmetatable(L, -2);

        return ud;
    }

    // 2 args
    static GDV* pushNewObject(lua_State* L, const Variant& arg1, const Variant& arg2) {
        GDV* ud = (GDV*)lua_newuserdata(L, sizeof(GDV));
        new (ud) GDV(arg1, arg2);

        luaL_getmetatable(L, variantName);
        lua_setmetatable(L, -2);

        return ud;
    }

    // 3 args
    static GDV* pushNewObject(lua_State* L, const Variant& arg1, const Variant& arg2, const Variant& arg3) {
        GDV* ud = (GDV*)lua_newuserdata(L, sizeof(GDV));
        new (ud) GDV(arg1, arg2, arg3);

        luaL_getmetatable(L, variantName);
        lua_setmetatable(L, -2);

        return ud;
    }

    // 4 args
    static GDV* pushNewObject(lua_State* L, const Variant& arg1, const Variant& arg2, const Variant& arg3, const Variant& arg4) {
        GDV* ud = (GDV*)lua_newuserdata(L, sizeof(GDV));
        new (ud) GDV(arg1, arg2, arg3, arg4);

        luaL_getmetatable(L, variantName);
        lua_setmetatable(L, -2);

        return ud;
    }

    static GDV& getObject(lua_State* L, unsigned int index) {
        void *ud = LuauBridge::luaL_checkudata(L, index, variantName);
        return *reinterpret_cast<GDV*>(ud);
    }

    static void registerVariant (lua_State* L);


    static int on_index(const GDV& object, const char* name, lua_State* L);
    static int on_newindex(GDV& object, const char* name, lua_State* L);
    static int on_call(GDV& object, lua_State* L);

    static int on_gc(lua_State *L) {
		getObject(L, 1).~GDV();
		return 0;
	}

	static int on_tostring(lua_State *L) {
        Variant value = getObject(L, 1);
        LuauBridge::push_string(L, value.stringify());
        return 1;
	}

	static int on_index(lua_State *L) {
		const char* key = lua_tostring(L, 2);

        StringName prop_name(key);

        Variant obj = getObject(L, 1);
        Variant value = obj.get(prop_name); //Get Variant GDV property, type should be Variant
        if (value.get_type() != Variant::NIL) {
            LuauBridge::push_variant(L, value);
            return 1;
        }

        ERR_PRINT("Property not found: " + String(key));
        return 1;
	}

	static int on_newindex(lua_State *L) {
        return 1;
	}

    static int on_call(lua_State *L) {
        const int argc = lua_gettop(L);

        pushNewObject(L, new GDV());
        return 1;
    }

    static int on_eq(lua_State *L) {
        lua_pushboolean(L, getObject(L, 1) == getObject(L, 2));
        return 1;
    }
};


//MARK: Vector2
class Vector2Bridge: public VariantBridge<godot::Vector2> {
    friend class VariantBridge <godot::Vector2>;

    public:
        static void registerVariantClass(lua_State* L);
    private:
        static int from_angle(lua_State* L);
        static const luaL_Reg staticLibrary[];
};


//MARK: Color
class ColorBridge : public VariantBridge<godot::Color> {
    friend class VariantBridge <godot::Color>;

    public:
        static void registerVariantClass(lua_State* L);
    private:
        static int hex(lua_State* L);
        static const luaL_Reg staticLibrary[];
};

}; // namespace luau
}; // namespace godot

#endif // LUAU_BRIDGE_H












