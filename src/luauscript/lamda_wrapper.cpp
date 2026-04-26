#include "lamda_wrapper.h"
#include "luau_bridge.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

Variant LuaFunctionWrapper::invoke(const Variant** p_args, GDExtensionInt p_arg_count, GDExtensionCallError& r_error) {
    if (!L || function_ref == LUA_NOREF) {
        r_error.error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
        return Variant();
    }

    // Get the function from registry
    lua_getref(L, function_ref);
    
    if (lua_isnil(L, -1)) {
        UtilityFunctions::push_error(vformat("Lua function not longer exist."));
        lua_pop(L, 1);
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        return Variant();
    }

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        return Variant();
    }

    // Push all arguments to the Lua stack
    for (int i = 0; i < p_arg_count; i++) {
        LuauBridge::push_variant(L, *p_args[i]);
    }

    // Call the function with p_arg_count arguments and 1 expected result
    int status = lua_pcall(L, static_cast<int>(p_arg_count), 1, 0);
    
    if (status != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        UtilityFunctions::push_error(vformat("Lua function call error: %s", err ? err : "unknown error"));
        lua_pop(L, 1);
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        return Variant();
    }

    // Get the result
    Variant result = LuauBridge::get_variant(L, -1);
    lua_pop(L, 1);

    r_error.error = GDEXTENSION_CALL_OK;
    return result;
}
