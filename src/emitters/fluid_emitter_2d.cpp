#include "fluid_emitter_2d.h"
#include "GPU_stable_fluids_2D_init.h"

#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/scene_tree.hpp"

#include <cmath>
#include <cstdlib>

namespace godot {

void FluidEmitter2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_active", "active"), &FluidEmitter2D::set_active);
	ClassDB::bind_method(D_METHOD("is_active"), &FluidEmitter2D::is_active);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");

	ClassDB::bind_method(D_METHOD("set_emission_mode", "mode"), &FluidEmitter2D::set_emission_mode);
	ClassDB::bind_method(D_METHOD("get_emission_mode"), &FluidEmitter2D::get_emission_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "emission_mode", PROPERTY_HINT_ENUM, "Continuous,Burst,Periodic"), "set_emission_mode", "get_emission_mode");

	ClassDB::bind_method(D_METHOD("set_emission_shape", "shape"), &FluidEmitter2D::set_emission_shape);
	ClassDB::bind_method(D_METHOD("get_emission_shape"), &FluidEmitter2D::get_emission_shape);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "emission_shape", PROPERTY_HINT_ENUM, "Point,Circle,Rect,LineSegment,TextureMask"), "set_emission_shape", "get_emission_shape");

	ClassDB::bind_method(D_METHOD("set_emission_preset", "preset"), &FluidEmitter2D::set_emission_preset);
	ClassDB::bind_method(D_METHOD("get_emission_preset"), &FluidEmitter2D::get_emission_preset);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "emission_preset", PROPERTY_HINT_ENUM, "Custom,Explosion,Mist,Smoke,Fountain,Steam,Fire"), "set_emission_preset", "get_emission_preset");

	ClassDB::bind_method(D_METHOD("set_emit_color", "color"), &FluidEmitter2D::set_emit_color);
	ClassDB::bind_method(D_METHOD("get_emit_color"), &FluidEmitter2D::get_emit_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "emit_color"), "set_emit_color", "get_emit_color");

	ClassDB::bind_method(D_METHOD("set_emit_velocity", "velocity"), &FluidEmitter2D::set_emit_velocity);
	ClassDB::bind_method(D_METHOD("get_emit_velocity"), &FluidEmitter2D::get_emit_velocity);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "emit_velocity"), "set_emit_velocity", "get_emit_velocity");

	ClassDB::bind_method(D_METHOD("set_color_radius", "radius"), &FluidEmitter2D::set_color_radius);
	ClassDB::bind_method(D_METHOD("get_color_radius"), &FluidEmitter2D::get_color_radius);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "color_radius", PROPERTY_HINT_RANGE, "0.1,10.0,0.1"), "set_color_radius", "get_color_radius");

	ClassDB::bind_method(D_METHOD("set_velocity_radius", "radius"), &FluidEmitter2D::set_velocity_radius);
	ClassDB::bind_method(D_METHOD("get_velocity_radius"), &FluidEmitter2D::get_velocity_radius);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "velocity_radius", PROPERTY_HINT_RANGE, "0.1,10.0,0.1"), "set_velocity_radius", "get_velocity_radius");

	ClassDB::bind_method(D_METHOD("set_emit_interval", "interval"), &FluidEmitter2D::set_emit_interval);
	ClassDB::bind_method(D_METHOD("get_emit_interval"), &FluidEmitter2D::get_emit_interval);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "emit_interval", PROPERTY_HINT_RANGE, "0.01,1.0,0.01"), "set_emit_interval", "get_emit_interval");

	ClassDB::bind_method(D_METHOD("set_burst_count", "count"), &FluidEmitter2D::set_burst_count);
	ClassDB::bind_method(D_METHOD("get_burst_count"), &FluidEmitter2D::get_burst_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "burst_count", PROPERTY_HINT_RANGE, "1,512"), "set_burst_count", "get_burst_count");

	ClassDB::bind_method(D_METHOD("set_shape_size", "size"), &FluidEmitter2D::set_shape_size);
	ClassDB::bind_method(D_METHOD("get_shape_size"), &FluidEmitter2D::get_shape_size);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "shape_size"), "set_shape_size", "get_shape_size");

	ClassDB::bind_method(D_METHOD("set_velocity_pattern", "pattern"), &FluidEmitter2D::set_velocity_pattern);
	ClassDB::bind_method(D_METHOD("get_velocity_pattern"), &FluidEmitter2D::get_velocity_pattern);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "velocity_pattern", PROPERTY_HINT_ENUM, "Directional,Radial,Random"), "set_velocity_pattern", "get_velocity_pattern");

	ClassDB::bind_method(D_METHOD("set_swirl_strength", "strength"), &FluidEmitter2D::set_swirl_strength);
	ClassDB::bind_method(D_METHOD("get_swirl_strength"), &FluidEmitter2D::get_swirl_strength);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "swirl_strength", PROPERTY_HINT_RANGE, "0.0,5.0,0.1"), "set_swirl_strength", "get_swirl_strength");

	ClassDB::bind_method(D_METHOD("set_lifetime", "lifetime"), &FluidEmitter2D::set_lifetime);
	ClassDB::bind_method(D_METHOD("get_lifetime"), &FluidEmitter2D::get_lifetime);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lifetime", PROPERTY_HINT_RANGE, "0.0,60.0,0.1"), "set_lifetime", "get_lifetime");

	ClassDB::bind_method(D_METHOD("set_auto_destroy", "enable"), &FluidEmitter2D::set_auto_destroy);
	ClassDB::bind_method(D_METHOD("is_auto_destroy"), &FluidEmitter2D::is_auto_destroy);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_destroy"), "set_auto_destroy", "is_auto_destroy");

	ClassDB::bind_method(D_METHOD("set_sim_target", "target"), &FluidEmitter2D::set_sim_target);
	ClassDB::bind_method(D_METHOD("get_sim_target"), &FluidEmitter2D::get_sim_target);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sim_target", PROPERTY_HINT_NODE_TYPE, "GPUStableFluids2D"), "set_sim_target", "get_sim_target");

	ADD_SIGNAL(MethodInfo("emission_finished"));
}

// Getters/Setters
void FluidEmitter2D::set_active(bool v) { _active = v; }
bool FluidEmitter2D::is_active() const { return _active; }
void FluidEmitter2D::set_emission_mode(int v) { _emission_mode = (EmissionMode)v; }
int FluidEmitter2D::get_emission_mode() const { return (int)_emission_mode; }
void FluidEmitter2D::set_emission_shape(int v) { _emission_shape = (EmissionShape)v; }
int FluidEmitter2D::get_emission_shape() const { return (int)_emission_shape; }
void FluidEmitter2D::set_emission_preset(int v) { _emission_preset = (EmitterPreset)v; _apply_preset(_emission_preset); }
int FluidEmitter2D::get_emission_preset() const { return (int)_emission_preset; }
void FluidEmitter2D::set_emit_color(const Color &v) { _emit_color = v; }
Color FluidEmitter2D::get_emit_color() const { return _emit_color; }
void FluidEmitter2D::set_emit_velocity(const Vector2 &v) { _emit_velocity = v; }
Vector2 FluidEmitter2D::get_emit_velocity() const { return _emit_velocity; }
void FluidEmitter2D::set_color_radius(float v) { _color_radius = v; }
float FluidEmitter2D::get_color_radius() const { return _color_radius; }
void FluidEmitter2D::set_velocity_radius(float v) { _velocity_radius = v; }
float FluidEmitter2D::get_velocity_radius() const { return _velocity_radius; }
void FluidEmitter2D::set_emit_interval(float v) { _emit_interval = v; }
float FluidEmitter2D::get_emit_interval() const { return _emit_interval; }
void FluidEmitter2D::set_burst_count(int v) { _burst_count = v; }
int FluidEmitter2D::get_burst_count() const { return _burst_count; }
void FluidEmitter2D::set_shape_size(const Vector2 &v) { _shape_size = v; }
Vector2 FluidEmitter2D::get_shape_size() const { return _shape_size; }
void FluidEmitter2D::set_velocity_pattern(int v) { _velocity_pattern = (VelocityPattern)v; }
int FluidEmitter2D::get_velocity_pattern() const { return (int)_velocity_pattern; }
void FluidEmitter2D::set_swirl_strength(float v) { _swirl_strength = v; }
float FluidEmitter2D::get_swirl_strength() const { return _swirl_strength; }
void FluidEmitter2D::set_lifetime(float v) { _lifetime = v; }
float FluidEmitter2D::get_lifetime() const { return _lifetime; }
void FluidEmitter2D::set_auto_destroy(bool v) { _auto_destroy = v; }
bool FluidEmitter2D::is_auto_destroy() const { return _auto_destroy; }
void FluidEmitter2D::set_sim_target(Object *p_obj) {
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
Object *FluidEmitter2D::get_sim_target() const {
	if (_sim_target.is_empty()) return nullptr;
	return get_node_or_null(_sim_target);
}

GPUStableFluids2D *FluidEmitter2D::_find_sim() const {
	if (!_sim_target.is_empty()) {
		Node *n = get_node_or_null(_sim_target);
		if (n) return Object::cast_to<GPUStableFluids2D>(n);
	}
	// Auto-find in scene
	SceneTree *tree = get_tree();
	if (tree) {
		TypedArray<Node> nodes = tree->get_nodes_in_group("fluid_sim_nodes");
		if (nodes.size() > 0) return Object::cast_to<GPUStableFluids2D>(nodes[0]);
	}
	return nullptr;
}

void FluidEmitter2D::_apply_preset(EmitterPreset p) {
	switch (p) {
		case EmitterPreset::Explosion: _emit_velocity = Vector2(0, -3); _color_radius = 1.5f; _velocity_radius = 2.0f; _burst_count = 64; _emission_mode = EmissionMode::Burst; _emit_color = Color(1, 0.6f, 0.2f, 0.8f); break;
		case EmitterPreset::Mist:      _emit_velocity = Vector2(0, -0.2f); _color_radius = 1.0f; _velocity_radius = 0.3f; _emit_interval = 0.1f; _emit_color = Color(0.9f, 0.9f, 0.9f, 0.15f); break;
		case EmitterPreset::Smoke:     _emit_velocity = Vector2(0, -0.5f); _color_radius = 2.0f; _velocity_radius = 1.0f; _emit_interval = 0.08f; _emit_color = Color(0.3f, 0.3f, 0.3f, 0.4f); _swirl_strength = 0.3f; break;
		case EmitterPreset::Fountain:  _emit_velocity = Vector2(0, -4); _color_radius = 2.0f; _velocity_radius = 1.5f; _emit_interval = 0.02f; _emit_color = Color(0.4f, 0.7f, 1.0f, 0.6f); break;
		case EmitterPreset::Steam:     _emit_velocity = Vector2(0, -1); _color_radius = 2.5f; _velocity_radius = 0.8f; _emit_interval = 0.05f; _emit_color = Color(0.95f, 0.95f, 0.95f, 0.2f); break;
		case EmitterPreset::Fire:      _emit_velocity = Vector2(0, -2); _color_radius = 1.5f; _velocity_radius = 2.0f; _emit_interval = 0.03f; _emit_color = Color(1, 0.4f, 0.1f, 0.7f); _swirl_strength = 0.5f; break;
		default: break;
	}
}

Vector2 FluidEmitter2D::_sample_shape_position() const {
	Vector2 center = get_global_position();
	switch (_emission_shape) {
		case EmissionShape::Point:
			return center;
		case EmissionShape::Circle: {
			float angle = (float)rand() / (float)RAND_MAX * 6.283185f;
			float r = (float)rand() / (float)RAND_MAX * _shape_size.x;
			return center + Vector2(cosf(angle) * r, sinf(angle) * r);
		}
		case EmissionShape::Rect: {
			float rx = ((float)rand() / (float)RAND_MAX - 0.5f) * _shape_size.x;
			float ry = ((float)rand() / (float)RAND_MAX - 0.5f) * _shape_size.y;
			return center + Vector2(rx, ry);
		}
		case EmissionShape::LineSegment: {
			float t = (float)rand() / (float)RAND_MAX;
			float angle = atan2f(_shape_size.y, _shape_size.x);
			float len = _shape_size.length();
			return center + Vector2(cosf(angle) * len * (t - 0.5f), sinf(angle) * len * (t - 0.5f));
		}
		default:
			return center;
	}
}

Vector2 FluidEmitter2D::_compute_velocity(const Vector2 &p_pos) const {
	Vector2 vel = _emit_velocity;
	switch (_velocity_pattern) {
		case VelocityPattern::Directional:
			break;
		case VelocityPattern::Radial: {
			Vector2 dir = p_pos - get_global_position();
			float len = dir.length();
			if (len > 0.001f) vel = dir.normalized() * _emit_velocity.length();
			break;
		}
		case VelocityPattern::Random: {
			float angle = (float)rand() / (float)RAND_MAX * 6.283185f;
			vel = Vector2(cosf(angle), sinf(angle)) * _emit_velocity.length();
			break;
		}
	}
	if (_swirl_strength > 0.001f) {
		Vector2 dir = p_pos - get_global_position();
		vel += Vector2(-dir.y, dir.x) * _swirl_strength;
	}
	return vel;
}

void FluidEmitter2D::_emit_particles(int p_count) {
	GPUStableFluids2D *sim = _find_sim();
	if (!sim) return;

	for (int i = 0; i < p_count; i++) {
		Vector2 pos = _sample_shape_position();
		Vector2 vel = _compute_velocity(pos);
		sim->queue_draw_request(pos, _emit_color, vel, _color_radius, _velocity_radius);
	}
}

void FluidEmitter2D::_ready() {
	_timer = 0;
	_life_timer = 0;
	_has_emitted = false;
	// Note: _apply_preset() is NOT called here because it was already called
	// by set_emission_preset() during scene property application. Calling it
	// again would overwrite any individual property overrides set in the scene.
}

void FluidEmitter2D::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;
	if (!_active) return;

	_life_timer += (float)p_delta;
	if (_lifetime > 0.0f && _life_timer >= _lifetime) {
		_active = false;
		emit_signal("emission_finished");
		if (_auto_destroy) queue_free();
		return;
	}

	_timer += (float)p_delta;

	switch (_emission_mode) {
		case EmissionMode::Continuous:
			if (_timer >= _emit_interval) {
				_emit_particles(1);
				_timer -= _emit_interval;
			}
			break;
		case EmissionMode::Burst:
			if (!_has_emitted) {
				_emit_particles(_burst_count);
				_has_emitted = true;
				emit_signal("emission_finished");
			}
			break;
		case EmissionMode::Periodic:
			if (_timer >= _emit_interval) {
				_emit_particles(_burst_count);
				_timer -= _emit_interval;
			}
			break;
	}
}
} // namespace godot
