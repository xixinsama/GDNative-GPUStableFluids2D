#include "fluid_mouse_interactor_2d.h"
#include "GPU_stable_fluids_2D_init.h"

#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/core/class_db.hpp"

namespace godot {

void FluidMouseInteractor2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sim_target", "target"), &FluidMouseInteractor2D::set_sim_target);
	ClassDB::bind_method(D_METHOD("get_sim_target"), &FluidMouseInteractor2D::get_sim_target);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sim_target", PROPERTY_HINT_NODE_TYPE, "GPUStableFluids2D"),
		"set_sim_target", "get_sim_target");

	ClassDB::bind_method(D_METHOD("set_mouse_button", "button"), &FluidMouseInteractor2D::set_mouse_button);
	ClassDB::bind_method(D_METHOD("get_mouse_button"), &FluidMouseInteractor2D::get_mouse_button);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mouse_button", PROPERTY_HINT_ENUM,
		"Left:1,Right:2,Middle:3"), "set_mouse_button", "get_mouse_button");

	ClassDB::bind_method(D_METHOD("set_draw_color", "color"), &FluidMouseInteractor2D::set_draw_color);
	ClassDB::bind_method(D_METHOD("get_draw_color"), &FluidMouseInteractor2D::get_draw_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "draw_color"), "set_draw_color", "get_draw_color");

	ClassDB::bind_method(D_METHOD("set_brush_size", "size"), &FluidMouseInteractor2D::set_brush_size);
	ClassDB::bind_method(D_METHOD("get_brush_size"), &FluidMouseInteractor2D::get_brush_size);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "brush_size", PROPERTY_HINT_RANGE, "1.0,100.0,0.5"),
		"set_brush_size", "get_brush_size");

	ClassDB::bind_method(D_METHOD("set_velocity_scale", "scale"), &FluidMouseInteractor2D::set_velocity_scale);
	ClassDB::bind_method(D_METHOD("get_velocity_scale"), &FluidMouseInteractor2D::get_velocity_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "velocity_scale", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
		"set_velocity_scale", "get_velocity_scale");

	ClassDB::bind_method(D_METHOD("set_interaction_mode", "mode"), &FluidMouseInteractor2D::set_interaction_mode);
	ClassDB::bind_method(D_METHOD("get_interaction_mode"), &FluidMouseInteractor2D::get_interaction_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "interaction_mode", PROPERTY_HINT_ENUM, "Draw,Push,Pull,Swirl"),
		"set_interaction_mode", "get_interaction_mode");
}

void FluidMouseInteractor2D::set_mouse_button(int b) { _mouse_button = b; }
int FluidMouseInteractor2D::get_mouse_button() const { return _mouse_button; }
void FluidMouseInteractor2D::set_draw_color(const Color &c) { _draw_color = c; }
Color FluidMouseInteractor2D::get_draw_color() const { return _draw_color; }
void FluidMouseInteractor2D::set_brush_size(float s) { _brush_size = s; }
float FluidMouseInteractor2D::get_brush_size() const { return _brush_size; }
void FluidMouseInteractor2D::set_velocity_scale(float s) { _velocity_scale = s; }
float FluidMouseInteractor2D::get_velocity_scale() const { return _velocity_scale; }
void FluidMouseInteractor2D::set_interaction_mode(int m) { _interaction_mode = (InteractionMode)m; }
int FluidMouseInteractor2D::get_interaction_mode() const { return (int)_interaction_mode; }

void FluidMouseInteractor2D::set_sim_target(Object *p_obj) {
	if (p_obj) {
		Node *node = Object::cast_to<Node>(p_obj);
		if (node) {
			_sim_target = get_path_to(node);
		} else {
			_sim_target = NodePath();
		}
	} else {
		_sim_target = NodePath();
	}
}
Object *FluidMouseInteractor2D::get_sim_target() const {
	if (_sim_target.is_empty()) return nullptr;
	return get_node_or_null(_sim_target);
}

void FluidMouseInteractor2D::_resolve_sim() {
	// 1) Explicit sim_target
	if (!_sim_target.is_empty()) {
		Node *n = get_node_or_null(_sim_target);
		if (n) {
			_sim = Object::cast_to<GPUStableFluids2D>(n);
			if (_sim) return;
		}
	}
	// 2) Walk up the parent chain
	Node *p = get_parent();
	while (p) {
		_sim = Object::cast_to<GPUStableFluids2D>(p);
		if (_sim) return;
		p = p->get_parent();
	}
}

void FluidMouseInteractor2D::_ready() {
	_resolve_sim();
	_prev_mouse_pos = Vector2(0, 0);
}

void FluidMouseInteractor2D::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;
	if (!_sim) return;

	Input *input = Input::get_singleton();
	if (!input) return;

	bool pressed = input->is_mouse_button_pressed((MouseButton)_mouse_button);

	if (!pressed) {
		// Mouse released — reset state for next stroke
		_was_pressed = false;
		_has_prev_pos = false;
		return;
	}

	// ── World-space mouse position ─────────────────────────
	// get_global_mouse_position() returns the mouse position in global
	// (world) coordinates, correctly accounting for Camera2D zoom / offset.
	// This is the same coordinate space used by GPUStableFluids2D's
	// queue_draw_request → world_to_UV conversion.
	Vector2 world_pos = get_global_mouse_position();

	// ── Compute velocity from mouse delta ──────────────────
	Vector2 vel(0, 0);
	if (_has_prev_pos && _was_pressed) {
		vel = (world_pos - _prev_mouse_pos) * _velocity_scale / Math::max((float)p_delta, 0.001f);
	}

	// ── Dispatch based on interaction mode ─────────────────
	switch (_interaction_mode) {
		case InteractionMode::Draw:
			_sim->queue_draw_request(world_pos, _draw_color, vel,
				_brush_size, _brush_size * 2.0f);
			break;
		case InteractionMode::Push:
			_sim->queue_draw_request(world_pos, Color(0, 0, 0, 0), vel * 10.0f,
				0.0f, _brush_size * 3.0f);
			break;
		case InteractionMode::Pull:
			_sim->queue_draw_request(world_pos, Color(0, 0, 0, 0), -vel * 10.0f,
				0.0f, _brush_size * 3.0f);
			break;
		case InteractionMode::Swirl:
			_sim->queue_draw_request(world_pos, _draw_color,
				Vector2(-vel.y, vel.x) * 10.0f, _brush_size, _brush_size * 2.0f);
			break;
	}

	_prev_mouse_pos = world_pos;
	_was_pressed = pressed;
	_has_prev_pos = true;
}

} // namespace godot
