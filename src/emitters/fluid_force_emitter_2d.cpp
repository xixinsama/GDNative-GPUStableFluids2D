#include "fluid_force_emitter_2d.h"
#include "GPU_stable_fluids_2D_init.h"

#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/scene_tree.hpp"

namespace godot {

void FluidForceEmitter2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_active", "active"), &FluidForceEmitter2D::set_active);
	ClassDB::bind_method(D_METHOD("is_active"), &FluidForceEmitter2D::is_active);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");

	ClassDB::bind_method(D_METHOD("set_force_pattern", "pattern"), &FluidForceEmitter2D::set_force_pattern);
	ClassDB::bind_method(D_METHOD("get_force_pattern"), &FluidForceEmitter2D::get_force_pattern);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "force_pattern", PROPERTY_HINT_ENUM, "Directional,PointRadial,Swirl,Centripetal,Centrifugal"), "set_force_pattern", "get_force_pattern");

	ClassDB::bind_method(D_METHOD("set_force", "force"), &FluidForceEmitter2D::set_force);
	ClassDB::bind_method(D_METHOD("get_force"), &FluidForceEmitter2D::get_force);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "force"), "set_force", "get_force");

	ClassDB::bind_method(D_METHOD("set_force_radius", "radius"), &FluidForceEmitter2D::set_force_radius);
	ClassDB::bind_method(D_METHOD("get_force_radius"), &FluidForceEmitter2D::get_force_radius);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "force_radius", PROPERTY_HINT_RANGE, "1.0,1000.0,1.0"), "set_force_radius", "get_force_radius");

	ClassDB::bind_method(D_METHOD("set_emission_shape", "shape"), &FluidForceEmitter2D::set_emission_shape);
	ClassDB::bind_method(D_METHOD("get_emission_shape"), &FluidForceEmitter2D::get_emission_shape);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "emission_shape", PROPERTY_HINT_ENUM, "Point,Circle,Rect,LineSegment,TextureMask"), "set_emission_shape", "get_emission_shape");

	ClassDB::bind_method(D_METHOD("set_shape_size", "size"), &FluidForceEmitter2D::set_shape_size);
	ClassDB::bind_method(D_METHOD("get_shape_size"), &FluidForceEmitter2D::get_shape_size);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "shape_size"), "set_shape_size", "get_shape_size");

	ClassDB::bind_method(D_METHOD("set_falloff_exponent", "exponent"), &FluidForceEmitter2D::set_falloff_exponent);
	ClassDB::bind_method(D_METHOD("get_falloff_exponent"), &FluidForceEmitter2D::get_falloff_exponent);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "falloff_exponent", PROPERTY_HINT_RANGE, "0.0,5.0,0.1"), "set_falloff_exponent", "get_falloff_exponent");

	ClassDB::bind_method(D_METHOD("set_swirl_strength", "strength"), &FluidForceEmitter2D::set_swirl_strength);
	ClassDB::bind_method(D_METHOD("get_swirl_strength"), &FluidForceEmitter2D::get_swirl_strength);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "swirl_strength", PROPERTY_HINT_RANGE, "0.0,10.0,0.1"), "set_swirl_strength", "get_swirl_strength");

	ClassDB::bind_method(D_METHOD("set_force_preset", "preset"), &FluidForceEmitter2D::set_force_preset);
	ClassDB::bind_method(D_METHOD("get_force_preset"), &FluidForceEmitter2D::get_force_preset);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "force_preset", PROPERTY_HINT_ENUM, "Custom,Wind,Gravity,Vortex,Explosion,Magnet"), "set_force_preset", "get_force_preset");

	ClassDB::bind_method(D_METHOD("set_lifetime", "lifetime"), &FluidForceEmitter2D::set_lifetime);
	ClassDB::bind_method(D_METHOD("get_lifetime"), &FluidForceEmitter2D::get_lifetime);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lifetime", PROPERTY_HINT_RANGE, "0.0,60.0,0.1"), "set_lifetime", "get_lifetime");

	ClassDB::bind_method(D_METHOD("set_auto_destroy", "enable"), &FluidForceEmitter2D::set_auto_destroy);
	ClassDB::bind_method(D_METHOD("is_auto_destroy"), &FluidForceEmitter2D::is_auto_destroy);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_destroy"), "set_auto_destroy", "is_auto_destroy");

	ClassDB::bind_method(D_METHOD("set_sim_target", "target"), &FluidForceEmitter2D::set_sim_target);
	ClassDB::bind_method(D_METHOD("get_sim_target"), &FluidForceEmitter2D::get_sim_target);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sim_target", PROPERTY_HINT_NODE_TYPE, "GPUStableFluids2D"), "set_sim_target", "get_sim_target");
}

void FluidForceEmitter2D::set_active(bool v) { _active = v; }
bool FluidForceEmitter2D::is_active() const { return _active; }
void FluidForceEmitter2D::set_force_pattern(int v) { _force_pattern = (ForcePattern)v; }
int FluidForceEmitter2D::get_force_pattern() const { return (int)_force_pattern; }
void FluidForceEmitter2D::set_force(const Vector2 &v) { _force = v; }
Vector2 FluidForceEmitter2D::get_force() const { return _force; }
void FluidForceEmitter2D::set_force_radius(float v) { _force_radius = v; }
float FluidForceEmitter2D::get_force_radius() const { return _force_radius; }
void FluidForceEmitter2D::set_emission_shape(int v) { _emission_shape = (EmissionShape)v; }
int FluidForceEmitter2D::get_emission_shape() const { return (int)_emission_shape; }
void FluidForceEmitter2D::set_shape_size(const Vector2 &v) { _shape_size = v; }
Vector2 FluidForceEmitter2D::get_shape_size() const { return _shape_size; }
void FluidForceEmitter2D::set_falloff_exponent(float v) { _falloff_exponent = v; }
float FluidForceEmitter2D::get_falloff_exponent() const { return _falloff_exponent; }
void FluidForceEmitter2D::set_swirl_strength(float v) { _swirl_strength = v; }
float FluidForceEmitter2D::get_swirl_strength() const { return _swirl_strength; }
void FluidForceEmitter2D::set_force_preset(int v) { _force_preset = (ForcePreset)v; _apply_preset(_force_preset); }
int FluidForceEmitter2D::get_force_preset() const { return (int)_force_preset; }
void FluidForceEmitter2D::set_lifetime(float v) { _lifetime = v; }
float FluidForceEmitter2D::get_lifetime() const { return _lifetime; }
void FluidForceEmitter2D::set_auto_destroy(bool v) { _auto_destroy = v; }
bool FluidForceEmitter2D::is_auto_destroy() const { return _auto_destroy; }
void FluidForceEmitter2D::set_sim_target(Object *p_obj) {
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
Object *FluidForceEmitter2D::get_sim_target() const {
	if (_sim_target.is_empty()) return nullptr;
	return get_node_or_null(_sim_target);
}

GPUStableFluids2D *FluidForceEmitter2D::_find_sim() const {
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

void FluidForceEmitter2D::_apply_preset(ForcePreset p) {
	switch (p) {
		case ForcePreset::Wind:      _force_pattern = ForcePattern::Directional; _force = Vector2(1, 0); _force_radius = 500; _falloff_exponent = 0.0f; break;
		case ForcePreset::Gravity:   _force_pattern = ForcePattern::Directional; _force = Vector2(0, 9.8f); _force_radius = 1000; _falloff_exponent = 0.0f; break;
		case ForcePreset::Vortex:    _force_pattern = ForcePattern::Swirl; _force = Vector2(0, -5); _force_radius = 200; _falloff_exponent = 2.0f; _swirl_strength = 3.0f; break;
		case ForcePreset::Explosion: _force_pattern = ForcePattern::PointRadial; _force = Vector2(0, -20); _force_radius = 150; _falloff_exponent = 3.0f; break;
		case ForcePreset::Magnet:    _force_pattern = ForcePattern::Centripetal; _force = Vector2(0, -3); _force_radius = 200; _falloff_exponent = 1.0f; break;
		default: break;
	}
}

void FluidForceEmitter2D::_ready() {
	_life_timer = 0;
	_apply_preset(_force_preset);
}

void FluidForceEmitter2D::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;
	if (!_active) return;

	_life_timer += (float)p_delta;
	if (_lifetime > 0.0f && _life_timer >= _lifetime) {
		_active = false;
		if (_auto_destroy) queue_free();
		return;
	}

	GPUStableFluids2D *sim = _find_sim();
	if (!sim) return;

	// Queue a draw request with force-only (zero color alpha)
	// The main sim rendering pipeline handles force application
	Vector2 pos = get_global_position();
	Vector2 vel = _force;
	switch (_force_pattern) {
		case ForcePattern::Swirl:       vel = Vector2(-_force.y, _force.x) * _swirl_strength; break;
		case ForcePattern::Centripetal: vel = -pos.normalized() * _force.length(); break;
		case ForcePattern::Centrifugal: vel = pos.normalized() * _force.length(); break;
		case ForcePattern::PointRadial: vel = _force; break;
		default: break;
	}
	sim->queue_draw_request(pos, Color(0, 0, 0, 0), vel * 10.0f, 0.0f, _force_radius * 0.1f);
}
} // namespace godot
