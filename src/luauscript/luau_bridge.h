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
    static const char* variant_name;

    static GDV* push_new(lua_State* L) {
        GDV* ud = (GDV*)lua_newuserdata(L, sizeof(GDV));
        new (ud) GDV();

        luaL_getmetatable(L, variant_name);
        lua_setmetatable(L, -2);

        return ud;
    }

    static GDV* push_from(lua_State* L, const Variant& v) {
        GDV* ud = (GDV*)lua_newuserdata(L, sizeof(GDV));
        new (ud) GDV(v.operator GDV());

        luaL_getmetatable(L, variant_name);
        lua_setmetatable(L, -2);

        return ud;
    }

    static GDV& get_object(lua_State* L, unsigned int index) {
        void *ud = LuauBridge::luaL_checkudata(L, index, variant_name);

        if (!ud) {
            luaL_error(L, "Invalid userdata");
        }

        return *reinterpret_cast<GDV*>(ud);
    }

    static void register_variant(lua_State* L);


    static int on_index(lua_State* L, const GDV& object, const char* key);
    static int on_newindex(lua_State* L, GDV& object, const char* key);
    static int on_call(lua_State* L);

    static int on_gc(lua_State *L) {
		get_object(L, 1).~GDV();
		return 0;
	}

	static int on_tostring(lua_State *L) {
        Variant value = get_object(L, 1);
        LuauBridge::push_string(L, value.stringify());
        return 1;
	}

	static int on_index(lua_State *L) {
        Variant obj = get_object(L, 1);

		const char* key = lua_tostring(L, 2);
        StringName prop_name(key);

        bool valid;
        Variant value = obj.get(prop_name, &valid); //Get Variant GDV property

        if (!valid) {
            luaL_error(L, ("Invalid property: " + String(variant_name) + "." + String(key)).utf8().get_data());
            return 1;
        }

        if (value.get_type() == Variant::CALLABLE) {    
            // Get the metatable methods for this variant type
            lua_getglobal(L, variant_name);
            if (lua_isnil(L, -1)) {
                luaL_error(L, ("No metatable found for: " + String(variant_name)).utf8().get_data());
                return 1;
            }

            lua_pushstring(L, key);
            lua_rawget(L, -2);
            if (lua_isnil(L, -1)) {
                luaL_error(L, ("No method found for: " + String(variant_name) + "." + String(key)).utf8().get_data());
                return 1;
            }
            
            lua_remove(L, -2); // Remove global table

            return 1;

        } else if (value.get_type() != Variant::NIL) {
            LuauBridge::push_variant(L, value);
            return 1;
        }

        return on_index(L, obj, key);
	}

	static int on_newindex(lua_State *L) {
        Variant obj = get_object(L, 1);
		const char* key = lua_tostring(L, 2);
        Variant value = LuauBridge::get_variant(L, 3);

        StringName prop_name(key);

        // Try to set the property
        bool valid;
        obj.set(prop_name, value, &valid);
        if (!valid) {
            WARN_PRINT("Failed to set property: " + String(key));
            return 1;
        }

        // Get the userdata pointer and update it
        void* ud = lua_touserdata(L, 1);
        if (ud) {
            GDV* ptr = static_cast<GDV*>(ud);
            *ptr = obj;
        }

        return 1;
	}

    static int on_eq(lua_State *L) {
        lua_pushboolean(L, get_object(L, 1) == get_object(L, 2));
        return 1;
    }
};



//MARK: Vector2
class Vector2Bridge: public VariantBridge<Vector2> {
    friend class VariantBridge <Vector2>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
        static int from_angle(lua_State* L);
};



//MARK: Rect2
class Rect2Bridge : public VariantBridge<Rect2> {
    friend class VariantBridge <Rect2>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
};



//MARK: Color
class ColorBridge : public VariantBridge<Color> {
    friend class VariantBridge <Color>;

    public:
        static void register_variant_class(lua_State* L);
    private:
        static const luaL_Reg static_library[];
        static int hex(lua_State* L);
        static int lerp(lua_State* L);
};

}; // namespace luau
}; // namespace godot

#endif // LUAU_BRIDGE_H












