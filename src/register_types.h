#ifndef LUAUGDEXT_REGISTER_TYPES_H
#define LUAUGDEXT_REGISTER_TYPES_H

#include <gdextension_interface.h>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>

using namespace godot;

void initialize_luaugdext_module(ModuleInitializationLevel p_level);
void uninitialize_luaugdext_module(ModuleInitializationLevel p_level);

#endif
