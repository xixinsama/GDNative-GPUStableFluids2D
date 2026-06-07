#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "GPU_stable_fluids_2D_init.h"
#include "display/fluid_display_2d.h"
#include "display/fluid_mouse_interactor_2d.h"
#include "obstacles/fluid_obstacle_2d.h"

using namespace godot;

void initialize_fluid_sim_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<GPUStableFluids2D>();
	ClassDB::register_class<FluidDisplay2D>();
	ClassDB::register_class<FluidMouseInteractor2D>();
	ClassDB::register_class<FluidObstacle2D>();
}

void uninitialize_fluid_sim_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C" {
GDExtensionBool GDE_EXPORT fluid_sim_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                                    GDExtensionClassLibraryPtr p_library,
                                                    GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_fluid_sim_module);
	init_obj.register_terminator(uninitialize_fluid_sim_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
