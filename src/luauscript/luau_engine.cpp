#include "luau_engine.h"

#include <lua.h>
#include <lualib.h>
#include <godot_cpp/core/memory.hpp>

using namespace godot;

LuauEngine *LuauEngine::singleton = nullptr;

static void *luauGD_alloc(void *, void *ptr, size_t, size_t nsize) {
	if (nsize == 0) {
		if (ptr)
			memfree(ptr);

		return nullptr;
	}

	return memrealloc(ptr, nsize);
}

void luaGD_close(lua_State *L) {
	L = lua_mainthread(L);

	lua_close(L);
}


void LuauEngine::init_vm(VMType p_type) {
    lua_State *L = lua_newstate(luauGD_alloc, nullptr);

    luaL_openlibs(L);

    luaL_sandbox(L);

    vms[p_type] = L;
}

void LuauEngine::new_vm() {
    lua_State *L = lua_newstate(luauGD_alloc, nullptr);

    luaL_openlibs(L);
}

LuauEngine::LuauEngine() {
	init_vm(VM_SCRIPT_LOAD);
	init_vm(VM_CORE);
	init_vm(VM_USER);

    if (!singleton) {
        singleton = this;
    }
}

LuauEngine::~LuauEngine() {
	if (singleton == this) {
		singleton = nullptr;
	}

    for (lua_State *&L : vms) {
		luaGD_close(L);
		L = nullptr;
	}
}
