#include "fluid_mouse_interactor_2d.h"
#include "GPU_stable_fluids_2D_init.h"

#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/classes/viewport.hpp"
#include "godot_cpp/classes/camera2d.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/core/class_db.hpp"

namespace godot {

void FluidMouseInteractor2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sim_target", "path"), &FluidMouseInteractor2D::set_sim_target);
	ClassDB::bind_method(D_METHOD("get_sim_target"), &FluidMouseInteractor2D::get_sim_target);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sim_target", PROPERTY_HINT_NODE_TYPE, "GPUStableFluids2D"), "set_sim_target", "get_sim_target");

	ClassDB::bind_method(D_METHOD("set_mouse_button", "button"), &FluidMouseInteractor2D::set_mouse_button);
	ClassDB::bind_method(D_METHOD("get_mouse_button"), &FluidMouseInteractor2D::get_mouse_button);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mouse_button", PROPERTY_HINT_ENUM, "Left:1,Right:2,Middle:3"), "set_mouse_button", "get_mouse_button");

	ClassDB::bind_method(D_METHOD("set_draw_color", "color"), &FluidMouseInteractor2D::set_draw_color);
	ClassDB::bind_method(D_METHOD("get_draw_color"), &FluidMouseInteractor2D::get_draw_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "draw_color"), "set_draw_color", "get_draw_color");

	ClassDB::bind_method(D_METHOD("set_brush_size", "size"), &FluidMouseInteractor2D::set_brush_size);
	ClassDB::bind_method(D_METHOD("get_brush_size"), &FluidMouseInteractor2D::get_brush_size);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "brush_size", PROPERTY_HINT_RANGE, "1.0,100.0,0.5"), "set_brush_size", "get_brush_size");

	ClassDB::bind_method(D_METHOD("set_velocity_scale", "scale"), &FluidMouseInteractor2D::set_velocity_scale);
	ClassDB::bind_method(D_METHOD("get_velocity_scale"), &FluidMouseInteractor2D::get_velocity_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "velocity_scale", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_velocity_scale", "get_velocity_scale");

	ClassDB::bind_method(D_METHOD("set_continuous_draw", "enable"), &FluidMouseInteractor2D::set_continuous_draw);
	ClassDB::bind_method(D_METHOD("is_continuous_draw"), &FluidMouseInteractor2D::is_continuous_draw);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "continuous_draw"), "set_continuous_draw", "is_continuous_draw");

	ClassDB::bind_method(D_METHOD("set_interaction_mode", "mode"), &FluidMouseInteractor2D::set_interaction_mode);
	ClassDB::bind_method(D_METHOD("get_interaction_mode"), &FluidMouseInteractor2D::get_interaction_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "interaction_mode", PROPERTY_HINT_ENUM, "Draw,Push,Pull,Swirl"),
		"set_interaction_mode", "get_interaction_mode");
}

void FluidMouseInteractor2D::set_sim_target(const NodePath &p) { _sim_target = p; }
NodePath FluidMouseInteractor2D::get_sim_target() const { return _sim_target; }
void FluidMouseInteractor2D::set_mouse_button(int b) { _mouse_button = b; }
int FluidMouseInteractor2D::get_mouse_button() const { return _mouse_button; }
void FluidMouseInteractor2D::set_draw_color(const Color &c) { _draw_color = c; }
Color FluidMouseInteractor2D::get_draw_color() const { return _draw_color; }
void FluidMouseInteractor2D::set_brush_size(float s) { _brush_size = s; }
float FluidMouseInteractor2D::get_brush_size() const { return _brush_size; }
void FluidMouseInteractor2D::set_velocity_scale(float s) { _velocity_scale = s; }
float FluidMouseInteractor2D::get_velocity_scale() const { return _velocity_scale; }
void FluidMouseInteractor2D::set_continuous_draw(bool e) { _continuous_draw = e; }
bool FluidMouseInteractor2D::is_continuous_draw() const { return _continuous_draw; }
void FluidMouseInteractor2D::set_interaction_mode(int m) { _interaction_mode = (InteractionMode)m; }
int FluidMouseInteractor2D::get_interaction_mode() const { return (int)_interaction_mode; }

GPUStableFluids2D *FluidMouseInteractor2D::_find_sim() const {
	if (!_sim_target.is_empty()) {
		Node *n = get_node_or_null(_sim_target);
		if (n) return Object::cast_to<GPUStableFluids2D>(n);
	}
	// Fall back to parent
	return Object::cast_to<GPUStableFluids2D>(get_parent());
}

void FluidMouseInteractor2D::_ready() {
	_prev_mouse_pos = Vector2(0, 0);
}

void FluidMouseInteractor2D::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;

	GPUStableFluids2D *sim = _find_sim();
	if (!sim) return;

	Input *input = Input::get_singleton();
	if (!input) return;

	bool pressed = input->is_mouse_button_pressed((MouseButton)_mouse_button);

	if (!pressed && !_was_pressed) {
		_prev_mouse_pos = Vector2(0, 0);
		_was_pressed = false;
		return;
	}

	// Get mouse position in world space
	Viewport *vp = get_viewport();
	if (!vp) return;
	Vector2 mpos = vp->get_mouse_position();

	Camera2D *cam = vp->get_camera_2d();
	Vector2 world_pos;
	if (cam) {
		world_pos = cam->get_screen_center_position() +
		            (mpos - vp->get_visible_rect().size * 0.5f) / cam->get_zoom();
	} else {
		world_pos = mpos;
	}

	// Determine velocity
	Vector2 vel(0, 0);
	if (_prev_mouse_pos != Vector2(0, 0)) {
		vel = (world_pos - _prev_mouse_pos) * _velocity_scale;
	}

	switch (_interaction_mode) {
		case InteractionMode::Draw:
			sim->queue_draw_request(world_pos, _draw_color, vel, _brush_size, _brush_size * 2.0f);
			break;
		case InteractionMode::Push:
			sim->queue_draw_request(world_pos, Color(0,0,0,0), vel * 10.0f, 0.0f, _brush_size * 3.0f);
			break;
		case InteractionMode::Pull:
			sim->queue_draw_request(world_pos, Color(0,0,0,0), -vel * 10.0f, 0.0f, _brush_size * 3.0f);
			break;
		case InteractionMode::Swirl:
			sim->queue_draw_request(world_pos, _draw_color,
				Vector2(-vel.y, vel.x) * 10.0f, _brush_size, _brush_size * 2.0f);
			break;
	}

	_prev_mouse_pos = world_pos;
	_was_pressed = pressed;
}
} // namespace godot
