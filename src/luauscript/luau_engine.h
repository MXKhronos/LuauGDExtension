#ifndef LUAU_ENGINE_H
#define LUAU_ENGINE_H

#include <lua.h>
#include <godot_cpp/core/mutex_lock.hpp>
#include <godot_cpp/core/type_info.hpp>

namespace godot {
    class LuauEngine {

    public:
        enum VMType {
            VM_SCRIPT_LOAD = 0, // Runs code for getting basic information for LuauScript.
            VM_CORE, // Runs the core game code.
            VM_USER, // Runs any potentially unsafe user code.
            VM_MAX
        };

    private:
        static LuauEngine *singleton;
        lua_State *vms[VM_MAX];
        void init_vm(VMType p_type);

    public:
        static LuauEngine *get_singleton() { return singleton; };

        lua_State *get_vm(VMType p_type) { 
            if (p_type >= 0 && p_type < VM_MAX) {
                return vms[p_type];
            }
            return nullptr;
        }

        void new_vm();

        LuauEngine();
        ~LuauEngine();
    };

}

#endif