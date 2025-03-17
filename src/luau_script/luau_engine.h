#pragma once

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

class LuauEngine {

public:
    static void init();
    static void deinit();

    // static bool is_valid_extension(const String &path);
};