#include "register_types.h"

#include <luau_script/luau_engine.h>

using namespace godot;

void initialize_luaugdext_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
		return;

    LuauEngine::init();
}

void uninitialize_luaugdext_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
		return;

    LuauEngine::deinit();
}

// Initialization.
extern "C" {
    GDExtensionBool GDE_EXPORT luaugdext_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address, 
        const GDExtensionClassLibraryPtr p_library, 
        GDExtensionInitialization *r_initialization
    ) {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_luaugdext_module);
        init_obj.register_terminator(uninitialize_luaugdext_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
