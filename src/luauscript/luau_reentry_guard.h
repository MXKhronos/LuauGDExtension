#ifndef LUAU_REENTRY_GUARD_H
#define LUAU_REENTRY_GUARD_H

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#ifndef LUAU_MAX_REENTRY_DEPTH
#define LUAU_MAX_REENTRY_DEPTH 64
#endif

class LuauReentryGuard {
public:
	static int depth() {
		return counter();
	}

	static bool would_exceed_limit() {
		return overflow() || counter() >= LUAU_MAX_REENTRY_DEPTH;
	}

	static void raise_overflow(const godot::String &p_message) {
		if (overflow())
			return;
		overflow_message() = p_message;
		overflow() = true;
		godot::UtilityFunctions::push_error(p_message);
	}

	static bool has_overflow() {
		return overflow();
	}

	static bool peek_overflow_message(godot::String *r_message) {
		if (overflow() && r_message != nullptr)
			*r_message = overflow_message();
		return overflow();
	}

	static void restore(int p_depth_before) {
		counter() = p_depth_before;
		if (p_depth_before == 0)
			clear_overflow();
	}

	// RAII token: counts one bridge crossing for its lifetime.
	LuauReentryGuard() {
		if (counter() == 0)
			clear_overflow(); // stale flags must not leak into a new top-level call
		++counter();
	}
	~LuauReentryGuard() {
		--counter();
	}

	LuauReentryGuard(const LuauReentryGuard &) = delete;
	LuauReentryGuard &operator=(const LuauReentryGuard &) = delete;

private:
	static int &counter() {
		static thread_local int s_depth = 0;
		return s_depth;
	}

	static bool &overflow() {
		static thread_local bool s_overflow = false;
		return s_overflow;
	}

	static godot::String &overflow_message() {
		static thread_local godot::String s_message;
		return s_message;
	}

	static void clear_overflow() {
		overflow() = false;
		overflow_message() = godot::String();
	}
};

#endif // LUAU_REENTRY_GUARD_H
