#include "fluid_vorticity_visualizer_2d.h"
#include "GPU_stable_fluids_2D_init.h"
#include "core/fluid_debug_config.h"

#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/core/class_db.hpp"

namespace godot {

FluidVorticityVisualizer2D::FluidVorticityVisualizer2D() {
	set_centered(false);
}

void FluidVorticityVisualizer2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sim_target", "target"), &FluidVorticityVisualizer2D::set_sim_target);
	ClassDB::bind_method(D_METHOD("get_sim_target"), &FluidVorticityVisualizer2D::get_sim_target);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sim_target", PROPERTY_HINT_NODE_TYPE, "GPUStableFluids2D"),
		"set_sim_target", "get_sim_target");

	ClassDB::bind_method(D_METHOD("set_display_mode", "mode"), &FluidVorticityVisualizer2D::set_display_mode);
	ClassDB::bind_method(D_METHOD("get_display_mode"), &FluidVorticityVisualizer2D::get_display_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "display_mode", PROPERTY_HINT_ENUM,
		"Density,Velocity,Pressure,Divergence,Vorticity"), "set_display_mode", "get_display_mode");

	ClassDB::bind_method(D_METHOD("set_auto_update", "enable"), &FluidVorticityVisualizer2D::set_auto_update);
	ClassDB::bind_method(D_METHOD("is_auto_update"), &FluidVorticityVisualizer2D::is_auto_update);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_update"), "set_auto_update", "is_auto_update");
}

void FluidVorticityVisualizer2D::set_sim_target(const NodePath &v) { _sim_target = v; }
NodePath FluidVorticityVisualizer2D::get_sim_target() const { return _sim_target; }
void FluidVorticityVisualizer2D::set_display_mode(int v) { _display_mode = (DisplayMode)v; }
int FluidVorticityVisualizer2D::get_display_mode() const { return (int)_display_mode; }
void FluidVorticityVisualizer2D::set_auto_update(bool v) { _auto_update = v; }
bool FluidVorticityVisualizer2D::is_auto_update() const { return _auto_update; }

GPUStableFluids2D *FluidVorticityVisualizer2D::_find_sim() const {
	if (!_sim_target.is_empty()) {
		Node *n = get_node_or_null(_sim_target);
		if (n) return Object::cast_to<GPUStableFluids2D>(n);
	}
	SceneTree *tree = get_tree();
	if (tree) {
		TypedArray<Node> nodes = tree->get_nodes_in_group("fluid_sim_nodes");
		if (nodes.size() > 0) return Object::cast_to<GPUStableFluids2D>(nodes[0]);
	}
	return nullptr;
}

void FluidVorticityVisualizer2D::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) return;
}

void FluidVorticityVisualizer2D::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;
	if (!_auto_update) return;

	GPUStableFluids2D *sim = _find_sim();
	if (!sim) return;

	// Select the internal field texture based on display_mode
	Ref<Texture2DRD> out_tex;
	switch (_display_mode) {
		case DisplayMode::Velocity:   out_tex = sim->get_velocity_texture();   break;
		case DisplayMode::Pressure:   out_tex = sim->get_pressure_texture();   break;
		case DisplayMode::Divergence: out_tex = sim->get_divergence_texture(); break;
		case DisplayMode::Vorticity:  // Vorticity field not directly stored; fall back to velocity
		case DisplayMode::Density:
		default:                      out_tex = sim->get_output_texture();      break;
	}

	if (!out_tex.is_valid()) {
		if (!_texture_warned) {
			FLUID_PRINTERR("[FluidVorticityVisualizer2D] _process: sim output texture is invalid!");
			_texture_warned = true;
		}
		return;
	}

	Ref<Texture2DRD> tex = get_texture();
	if (tex.is_valid()) {
		tex->set_texture_rd_rid(out_tex->get_texture_rd_rid());
	} else {
		set_texture(out_tex);
	}
}

} // namespace godot
