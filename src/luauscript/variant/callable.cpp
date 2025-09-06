
#include "callable.h"

#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/signal.hpp>

using namespace godot;

template<>
const char* VariantBridge<Callable>::variant_name("Callable");

const luaL_Reg CallableBridge::static_library[] = {
    {"create", create},
	{NULL, NULL}
};

int CallableBridge::create(lua_State* L) {
    if (lua_gettop(L) != 2) {
        luaL_error(L, "create requires 2 arguments");
        return 1;
    }
    
    Variant a1 = LuauBridge::get_variant(L, 1);
    Variant a2 = LuauBridge::get_variant(L, 2);

    if (a2.get_type() != Variant::STRING_NAME) {
        luaL_error(L, "create requires a StringName as the second argument");
        return 1;
    }
    
    push_from(L, Callable(a1.get_validated_object(), a2.operator StringName()));
    return 1;
}

void CallableBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Callable>::on_index(lua_State* L, const Callable& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Callable>::on_newindex(lua_State* L, Callable& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Callable>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::CALLABLE: {
                push_from(L, v.operator Callable());
                return 1;
            }
        };

    } else if (argc == 2) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);

        if (v1.get_type() == Variant::OBJECT && v2.get_type() == Variant::STRING_NAME) {
            push_from(L, Callable(v1.get_validated_object(), v2.operator StringName()));
            return 1;
        }
    }

    is_valid = false;
    return 1;
}




//MARK: Signal
template<>
const char* VariantBridge<Signal>::variant_name("Signal");

const luaL_Reg SignalBridge::static_library[] = {
	{NULL, NULL}
};

void SignalBridge::register_variant_class(lua_State* L) {
    luaL_register(L, variant_name, static_library);

    luaL_getmetatable(L, variant_name);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
int VariantBridge<Signal>::on_index(lua_State* L, const Signal& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Signal>::on_newindex(lua_State* L, Signal& object, const char* key) {
    return 1;
}

template<>
int VariantBridge<Signal>::on_call(lua_State* L, bool& is_valid) {
    const int argc = lua_gettop(L)-1;

    if (argc == 0) {
        push_new(L);
        return 1;
        
    } else if (argc == 1) {
        Variant v = LuauBridge::get_variant(L, 2);

        switch(v.get_type()) {
            case Variant::SIGNAL: {
                push_from(L, v.operator Signal());
                return 1;
            }
        };

    } else if (argc == 2) {
        Variant v1 = LuauBridge::get_variant(L, 2);
        Variant v2 = LuauBridge::get_variant(L, 3);

        if (v1.get_type() == Variant::OBJECT && v2.get_type() == Variant::STRING_NAME) {
            push_from(L, Signal(v1.get_validated_object(), v2.operator StringName()));
            return 1;
        }
    }

    is_valid = false;
    return 1;
}