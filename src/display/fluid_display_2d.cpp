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
	ClassDB::bind_method(D_METHOD("set_display_mode", "mode"), &FluidDisplay2D::set_display_mode);
	ClassDB::bind_method(D_METHOD("get_display_mode"), &FluidDisplay2D::get_display_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "display_mode", PROPERTY_HINT_ENUM,
		"Density,Velocity,Pressure,Divergence,Vorticity"), "set_display_mode", "get_display_mode");

	ClassDB::bind_method(D_METHOD("set_show_debug_bounds", "show"), &FluidDisplay2D::set_show_debug_bounds);
	ClassDB::bind_method(D_METHOD("is_showing_debug_bounds"), &FluidDisplay2D::is_showing_debug_bounds);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_debug_bounds"), "set_show_debug_bounds", "is_showing_debug_bounds");

	ClassDB::bind_method(D_METHOD("bind_to_sim", "sim"), &FluidDisplay2D::bind_to_sim);
}

void FluidDisplay2D::set_display_mode(int p_mode) { _display_mode = (DisplayMode)p_mode; }
int FluidDisplay2D::get_display_mode() const { return (int)_display_mode; }
void FluidDisplay2D::set_show_debug_bounds(bool p_show) { _show_debug_bounds = p_show; }
bool FluidDisplay2D::is_showing_debug_bounds() const { return _show_debug_bounds; }

// ── Called by the parent GPUStableFluids2D after GPU init ──

void FluidDisplay2D::bind_to_sim(GPUStableFluids2D *p_sim) {
	_sim = p_sim;
	if (!_sim) return;

	// Create our own Texture2DRD wrapper — do NOT share the sim's
	// object, otherwise other display nodes would be affected when we
	// switch display_mode via set_texture_rd_rid().
	RID display_rid = _sim->get_output_texture()->get_texture_rd_rid();
	if (display_rid.is_valid()) {
		Ref<Texture2DRD> tex;
		tex.instantiate();
		tex->set_texture_rd_rid(display_rid);
		set_texture(tex);
	}

	// Auto-size: match fluid world dimensions
	Vector2 ws = _sim->get_fluid_world_size();
	set_scale(Vector2(
		ws.x / (float)_sim->get_width(),
		ws.y / (float)_sim->get_height()));
}

// ── Per-frame: update texture when display_mode changes ────

void FluidDisplay2D::_update_texture() {
	if (!_sim) return;

	Ref<Texture2DRD> out_tex;
	switch (_display_mode) {
		case DisplayMode::Velocity:   out_tex = _sim->get_velocity_texture();   break;
		case DisplayMode::Pressure:   out_tex = _sim->get_pressure_texture();   break;
		case DisplayMode::Divergence: out_tex = _sim->get_divergence_texture(); break;
		case DisplayMode::Vorticity:
		case DisplayMode::Density:
		default:                      out_tex = _sim->get_output_texture();      break;
	}

	if (!out_tex.is_valid()) {
		if (!_texture_warned) {
			FLUID_PRINTERR("[FluidDisplay2D] Sim output texture is invalid!");
			_texture_warned = true;
		}
		return;
	}

	Ref<Texture2DRD> tex = get_texture();
	if (tex.is_valid()) {
		tex->set_texture_rd_rid(out_tex->get_texture_rd_rid());
	}
}

// ── Godot lifecycle ───────────────────────────────────────

void FluidDisplay2D::_ready() {
	// Nothing to do — parent GPUStableFluids2D will call bind_to_sim()
	// after its own GPU initialization is complete.
}

void FluidDisplay2D::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;
	_update_texture();
}

} // namespace godot
