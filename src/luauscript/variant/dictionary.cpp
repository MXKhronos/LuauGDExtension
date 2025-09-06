
#include "dictionary.h"

#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

template<>
const char* VariantBridge<Dictionary>::variant_name("Dictionary");

const luaL_Reg DictionaryBridge::static_library[] = {
	{NULL, NULL}
};

void DictionaryBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Dictionary>::on_index(lua_State* L, const Dictionary& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Dictionary>::on_newindex(lua_State* L, Dictionary& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Dictionary>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::DICTIONARY: {
                push_from(L, v.operator Dictionary());
                return 1;
            }
        };

    } else if (argc == 7) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);
        Variant v3 = LuauBridge::get_variant(L, 4);        
        Variant v4 = LuauBridge::get_variant(L, 5);        
        Variant v5 = LuauBridge::get_variant(L, 6);        
        Variant v6 = LuauBridge::get_variant(L, 7);
        Variant v7 = LuauBridge::get_variant(L, 8);      

        //Dictionary, int, StringName, Variant, int, StringName, Variant
        if (v1.get_type() == Variant::DICTIONARY 
            && v2.get_type() == Variant::INT 
            && v3.get_type() == Variant::STRING_NAME 
            && v4.get_type() == Variant::NIL 
            && v5.get_type() == Variant::INT 
            && v6.get_type() == Variant::STRING_NAME 
            && v7.get_type() == Variant::NIL
        ) {
            push_from(L, Dictionary(
                v1.operator Dictionary(), 
                v2.operator int(), 
                v3.operator StringName(), 
                v4, 
                v5.operator int(), 
                v6.operator StringName(), 
                v7
            ));
            return 1;
        }

    }

    is_valid = false;
    return 1;
}