#include "fluid_display_2d.h"
#include "GPU_stable_fluids_2D_init.h"
#include "core/fluid_debug_config.h"

#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

namespace godot {

FluidDisplay2D::FluidDisplay2D() {
	set_centered(false);
}

void FluidDisplay2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sim_source_path", "path"), &FluidDisplay2D::set_sim_source_path);
	ClassDB::bind_method(D_METHOD("get_sim_source_path"), &FluidDisplay2D::get_sim_source_path);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "sim_source_path"), "set_sim_source_path", "get_sim_source_path");

	ClassDB::bind_method(D_METHOD("set_display_mode", "mode"), &FluidDisplay2D::set_display_mode);
	ClassDB::bind_method(D_METHOD("get_display_mode"), &FluidDisplay2D::get_display_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "display_mode", PROPERTY_HINT_ENUM,
		"Density,Velocity,Pressure,Divergence,Vorticity"), "set_display_mode", "get_display_mode");

	ClassDB::bind_method(D_METHOD("set_auto_size", "enable"), &FluidDisplay2D::set_auto_size);
	ClassDB::bind_method(D_METHOD("is_auto_size"), &FluidDisplay2D::is_auto_size);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_size"), "set_auto_size", "is_auto_size");
}

void FluidDisplay2D::set_sim_source_path(const NodePath &p_path) { _sim_source_path = p_path; }
NodePath FluidDisplay2D::get_sim_source_path() const { return _sim_source_path; }
void FluidDisplay2D::set_display_mode(int p_mode) { _display_mode = (DisplayMode)p_mode; }
int FluidDisplay2D::get_display_mode() const { return (int)_display_mode; }
void FluidDisplay2D::set_auto_size(bool p_enable) { _auto_size = p_enable; }
bool FluidDisplay2D::is_auto_size() const { return _auto_size; }

void FluidDisplay2D::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) return;

	if (_sim_source_path.is_empty()) return;

	Node *src = get_node_or_null(_sim_source_path);
	if (!src) return;

	GPUStableFluids2D *sim = Object::cast_to<GPUStableFluids2D>(src);
	if (!sim) return;

	Ref<Texture2DRD> tex = sim->get_output_texture();
	if (tex.is_valid()) {
		set_texture(tex);
		if (_auto_size) {
			Vector2 ws = sim->get_fluid_world_size();
			set_scale(Vector2(ws.x / (float)sim->get_width(),
			                  ws.y / (float)sim->get_height()));
		}
	}
}

void FluidDisplay2D::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;

	if (_sim_source_path.is_empty()) return;
	Node *src = get_node_or_null(_sim_source_path);
	if (!src) return;
	GPUStableFluids2D *sim = Object::cast_to<GPUStableFluids2D>(src);
	if (!sim) return;

	Ref<Texture2DRD> out_tex = sim->get_output_texture();
	if (!out_tex.is_valid()) {
		if (!_texture_warned) {
			FLUID_PRINTERR("[FluidDisplay2D] _process: get_output_texture() returned invalid ref! The simulation may not be initialized yet.");
			_texture_warned = true;
		}
		return;
	}

	Ref<Texture2DRD> tex = get_texture();
	if (tex.is_valid()) {
		tex->set_texture_rd_rid(out_tex->get_texture_rd_rid());
	} else {
		// Texture was never set in _ready() (sim wasn't ready yet), set it now.
		set_texture(out_tex);
		if (_auto_size) {
			Vector2 ws = sim->get_fluid_world_size();
			set_scale(Vector2(ws.x / (float)sim->get_width(),
			                  ws.y / (float)sim->get_height()));
		}
	}
}

} // namespace godot
