#ifndef LUAU_AWAIT_H
#define LUAU_AWAIT_H

#include <lua.h>

namespace godot {

void luau_register_await(lua_State *L);
void luau_track_suspended_thread(lua_State *ET, lua_State *owner_T, int stack_index);
void luau_resume_suspended(lua_State *ET);
void luau_release_suspended(lua_State *ET);

} // namespace godot

#endif // LUAU_AWAIT_H
