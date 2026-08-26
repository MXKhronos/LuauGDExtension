#include "lamda_wrapper.h"
#include "luau_bridge.h"
#include "luauscript/luau_await.h"
#include "luauscript/luau_reentry_guard.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

Variant LuaFunctionWrapper::invoke(const Variant** p_args, GDExtensionInt p_arg_count, GDExtensionCallError& r_error) {
    if (!L || func_ref == LUA_NOREF) {
        r_error.error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
        return Variant();
    }

    if (LuauReentryGuard::would_exceed_limit()) {
        String overflow_msg = vformat(
            "Lua function call recursion limit exceeded (%d)", LUAU_MAX_REENTRY_DEPTH);
        LuauReentryGuard::raise_overflow(overflow_msg);
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        return Variant();
    }
    LuauReentryGuard invoke_guard;

    lua_State *ET = lua_newthread(L);

    // Get the function from registry (pushed on top of the thread value)
    lua_getref(L, func_ref);

    if (lua_isnil(L, -1)) {
        UtilityFunctions::push_error(vformat("Lua function not longer exist."));
        lua_pop(L, 2);
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        return Variant();
    }

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        return Variant();
    }

    // Move the function onto the callback thread
    lua_xmove(L, ET, 1);

    for (int i = 0; i < p_arg_count; i++) {
        LuauBridge::push_variant(ET, *p_args[i]);
    }

    // Pin the thread while it runs; luau_release_suspended releases the pin
    // when the coroutine finishes.
    luau_track_suspended_thread(ET, L, lua_gettop(L));
    lua_pop(L, 1); // remove the thread value from L (kept alive by the pin)

    // Reconcile the re-entrancy depth afterwards: Lua errors unwind via
    // longjmp and skip RAII guards (see luau_reentry_guard.h).
    int reentry_depth_before = LuauReentryGuard::depth();
    int status = lua_resume(ET, L, static_cast<int>(p_arg_count));
    LuauReentryGuard::restore(reentry_depth_before);

    if (status == LUA_YIELD) {
        // Suspended by await(): fire-and-forget. The wake-up source resumes
        // it later; results are discarded.
        r_error.error = GDEXTENSION_CALL_OK;
        return Variant();
    }

    Variant result;
    if (status == LUA_OK && lua_gettop(ET) > 0) {
        result = LuauBridge::get_variant(ET, -1);
    } else if (status != LUA_OK) {
        const char* err = lua_tostring(ET, -1);
        UtilityFunctions::push_error(vformat("Lua function call error: %s", err ? err : "unknown error"));
    }

    luau_release_suspended(ET);

    r_error.error = GDEXTENSION_CALL_OK;
    return result;
}
