#ifndef LUAUGDEXT_REGISTER_H
#define LUAUGDEXT_REGISTER_H

#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>

void initialize_luau_module(godot::ModuleInitializationLevel p_level);
void uninitialize_luau_module(godot::ModuleInitializationLevel p_level);

#endif
