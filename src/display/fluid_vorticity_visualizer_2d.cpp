#include "fluid_vorticity_visualizer_2d.h"
#include "GPU_stable_fluids_2D_init.h"

#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/core/class_db.hpp"

namespace godot {

void FluidVorticityVisualizer2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sim_source_path", "path"), &FluidVorticityVisualizer2D::set_sim_source_path);
	ClassDB::bind_method(D_METHOD("get_sim_source_path"), &FluidVorticityVisualizer2D::get_sim_source_path);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "sim_source_path"), "set_sim_source_path", "get_sim_source_path");

	ClassDB::bind_method(D_METHOD("set_display_mode", "mode"), &FluidVorticityVisualizer2D::set_display_mode);
	ClassDB::bind_method(D_METHOD("get_display_mode"), &FluidVorticityVisualizer2D::get_display_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "display_mode", PROPERTY_HINT_ENUM, "Density,Velocity,Pressure,Divergence,Vorticity"), "set_display_mode", "get_display_mode");

	ClassDB::bind_method(D_METHOD("set_auto_update", "enable"), &FluidVorticityVisualizer2D::set_auto_update);
	ClassDB::bind_method(D_METHOD("is_auto_update"), &FluidVorticityVisualizer2D::is_auto_update);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_update"), "set_auto_update", "is_auto_update");
}

void FluidVorticityVisualizer2D::set_sim_source_path(const NodePath &v) { _sim_source_path = v; }
NodePath FluidVorticityVisualizer2D::get_sim_source_path() const { return _sim_source_path; }
void FluidVorticityVisualizer2D::set_display_mode(int v) { _display_mode = (DisplayMode)v; }
int FluidVorticityVisualizer2D::get_display_mode() const { return (int)_display_mode; }
void FluidVorticityVisualizer2D::set_auto_update(bool v) { _auto_update = v; }
bool FluidVorticityVisualizer2D::is_auto_update() const { return _auto_update; }

void FluidVorticityVisualizer2D::_ready() {
	// Debug node — no heavy initialization needed
}

void FluidVorticityVisualizer2D::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;
	if (!_auto_update) return;
	// Debug visualization: could draw debug lines/rects based on field values
	// Placeholder for now — the node serves as an API anchor
}

} // namespace godot
