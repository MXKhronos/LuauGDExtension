#ifndef LAMDA_WRAPPER_H
#define LAMDA_WRAPPER_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/array.hpp>
#include <functional>

#include <lua.h>
#include <lualib.h>

namespace godot {
    class LambdaWrapper : public godot::Object {
        GDCLASS(LambdaWrapper, godot::Object);

    private:
        std::function<void()> func;

    protected:
        static void _bind_methods() {
            godot::ClassDB::bind_method(godot::D_METHOD("execute"), &LambdaWrapper::execute);
        }

    public:
        void set_function(std::function<void()> p_func) { func = p_func; }
        
        void execute() {
            if (func) func();
        }
    };

    // Wrapper for Lua functions to be used as Godot Callables
    class LuaFunctionWrapper : public godot::Object {
        GDCLASS(LuaFunctionWrapper, godot::Object);

    private:
        lua_State* L = nullptr;
        int function_ref = LUA_NOREF;

    protected:
        static void _bind_methods() {
            godot::ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "invoke", &LuaFunctionWrapper::invoke, MethodInfo("invoke"));
        }

    public:
        LuaFunctionWrapper() = default;
        
        ~LuaFunctionWrapper() {
            if (L && function_ref != LUA_NOREF) {
                lua_unref(L, function_ref);
                function_ref = LUA_NOREF;
            }
        }

        void set_lua_state(lua_State* p_L) { L = p_L; }
        void set_function_ref(int p_ref) { function_ref = p_ref; }
        
        lua_State* get_lua_state() const { return L; }
        int get_function_ref() const { return function_ref; }

        Variant invoke(const Variant** p_args, GDExtensionInt p_arg_count, GDExtensionCallError& r_error);
    };
}

#endif // LAMDA_WRAPPER_H
