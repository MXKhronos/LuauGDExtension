#include "luauscript/luau_await.h"
#include "luauscript/lamda_wrapper.h"
#include "luauscript/luau_bridge.h"
#include "luauscript/luau_reentry_guard.h"

#include <lua.h>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <unordered_map>

using namespace godot;

namespace {

struct SuspendedThread {
	lua_State *owner = nullptr;
	int ref = LUA_NOREF;
};

static std::unordered_map<lua_State *, SuspendedThread> g_suspended_threads;
static std::vector<Ref<LambdaWrapper>> g_active_resumes;

static void connect_resume(lua_State *L, Object *target, const StringName &signal_name, bool disconnect_after) {
	Ref<LambdaWrapper> resume;
	resume.instantiate();

	lua_State *thread = L;

	if (disconnect_after && target != nullptr) {
		Callable self_callable(resume.ptr(), "execute");
		resume->set_function([thread, target, signal_name, self_callable]() {
			target->disconnect(signal_name, self_callable);
			luau_resume_suspended(thread);
		});
	} else {
		resume->set_function([thread]() {
			luau_resume_suspended(thread);
		});
	}

	target->connect(signal_name, Callable(resume.ptr(), "execute"));
	g_active_resumes.push_back(resume);
}

static int luau_await(lua_State *L) {
	SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	if (tree == nullptr) {
		luaL_error(L, "await requires a running SceneTree");
		return 0;
	}

	switch (lua_type(L, 1)) {
		case LUA_TNUMBER: {
			double seconds = lua_tonumber(L, 1);
			Ref<SceneTreeTimer> timer = tree->create_timer(seconds);

			if (!timer.is_valid()) {
				luaL_error(L, "await: failed to create timer");
				return 0;
			}

			connect_resume(L, timer.ptr(), StringName("timeout"), false);

			break;
		}

		case LUA_TUSERDATA: {
			Variant value = LuauBridge::get_variant(L, 1);
			if (value.get_type() != Variant::SIGNAL) {
				luaL_error(L, "await expects a Signal, a number or nil");
				return 0;
			}

			Signal sig = value.operator Signal();
			Object *sig_obj = sig.get_object();
			if (sig_obj == nullptr) {
				luaL_error(L, "await: signal object was freed");
				return 0;
			}

			connect_resume(L, sig_obj, sig.get_name(), true);

			break;
		}

		default: {
			// nil (or omitted): wait one process frame
			connect_resume(L, tree, StringName("process_frame"), true);
			break;
		}
	}

	// Suspend the calling coroutine;
	return lua_yield(L, 0);
}

} // namespace

namespace godot {

void luau_register_await(lua_State *L) {
	lua_pushcfunction(L, luau_await, "await");
	lua_setglobal(L, "await");
}

void luau_track_suspended_thread(lua_State *ET, lua_State *owner_T, int stack_index) {
	SuspendedThread &suspended = g_suspended_threads[ET];
	suspended.owner = owner_T;
	suspended.ref = lua_ref(owner_T, stack_index);
}

void luau_release_suspended(lua_State *ET) {
	auto it = g_suspended_threads.find(ET);
	if (it == g_suspended_threads.end()) {
		return;
	}

	lua_settop(ET, 0);
	lua_unref(it->second.owner, it->second.ref);
	g_suspended_threads.erase(it);
}

void luau_resume_suspended(lua_State *ET) {
	auto it = g_suspended_threads.find(ET);
	if (it == g_suspended_threads.end()) {
		return; // unknown or already finished
	}

	// Reconcile the re-entrancy depth afterwards: errors unwind via longjmp
	// and skip RAII guards (see luau_reentry_guard.h). Without this, a
	// refused crossing inside an awaited continuation leaks depth and
	// permanently latches the overflow flag.
	int reentry_depth_before = LuauReentryGuard::depth();
	int status = lua_resume(ET, nullptr, 0);
	LuauReentryGuard::restore(reentry_depth_before);

	if (status == LUA_YIELD) {
		// Awaiting again; the existing pin keeps the thread alive.
		return;
	}

	// Finished (successfully or with an error): release the pin.
	if (status != LUA_OK) {
		const char *err = lua_tostring(ET, -1);
		UtilityFunctions::printerr(vformat("Luau coroutine error: %s", err ? err : "unknown"));
	}
	luau_release_suspended(ET);
}

} // namespace godot
