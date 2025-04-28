#ifndef LUAUGDEXT_REGISTER_H
#define LUAUGDEXT_REGISTER_H

#include <godot_cpp/godot.hpp>

namespace godot {
    void initialize_luau_module(ModuleInitializationLevel p_level);
    void uninitialize_luau_module(ModuleInitializationLevel p_level);
}

#endif
