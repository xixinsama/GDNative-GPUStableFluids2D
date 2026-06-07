#include "fluid_obstacle_2d.h"

#include "godot_cpp/classes/rigid_body2d.hpp"
#include "godot_cpp/classes/character_body2d.hpp"
#include "godot_cpp/core/class_db.hpp"

namespace godot {

void FluidObstacle2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_auto_detect_velocity", "enable"), &FluidObstacle2D::set_auto_detect_velocity);
	ClassDB::bind_method(D_METHOD("is_auto_detect_velocity"), &FluidObstacle2D::is_auto_detect_velocity);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_detect_velocity"), "set_auto_detect_velocity", "is_auto_detect_velocity");

	ClassDB::bind_method(D_METHOD("set_velocity", "velocity"), &FluidObstacle2D::set_velocity);
	ClassDB::bind_method(D_METHOD("get_velocity"), &FluidObstacle2D::get_velocity);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "velocity"), "set_velocity", "get_velocity");

	ClassDB::bind_method(D_METHOD("set_angular_velocity", "angular_velocity"), &FluidObstacle2D::set_angular_velocity);
	ClassDB::bind_method(D_METHOD("get_angular_velocity"), &FluidObstacle2D::get_angular_velocity);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "angular_velocity"), "set_angular_velocity", "get_angular_velocity");

	ClassDB::bind_method(D_METHOD("set_obstacle_texture", "texture"), &FluidObstacle2D::set_obstacle_texture);
	ClassDB::bind_method(D_METHOD("get_obstacle_texture"), &FluidObstacle2D::get_obstacle_texture);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "obstacle_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"),
		"set_obstacle_texture", "get_obstacle_texture");
}

void FluidObstacle2D::set_auto_detect_velocity(bool v) { _auto_detect = v; }
bool FluidObstacle2D::is_auto_detect_velocity() const { return _auto_detect; }
void FluidObstacle2D::set_velocity(const Vector2 &v) { _velocity = v; }
Vector2 FluidObstacle2D::get_velocity() const { return _velocity; }
void FluidObstacle2D::set_angular_velocity(float v) { _angular_velocity = v; }
float FluidObstacle2D::get_angular_velocity() const { return _angular_velocity; }
void FluidObstacle2D::set_obstacle_texture(const Ref<Texture2D> &t) { _obstacle_texture = t; }
Ref<Texture2D> FluidObstacle2D::get_obstacle_texture() const { return _obstacle_texture; }

Vector2 FluidObstacle2D::get_object_linear_velocity()  const { return _cached_lin_vel; }
float   FluidObstacle2D::get_object_angular_velocity() const { return _cached_ang_vel; }
Vector2 FluidObstacle2D::get_object_center()           const { return get_global_position(); }

void FluidObstacle2D::_ready() {
	add_to_group("fluid_obstacles");
}

void FluidObstacle2D::_physics_process(double p_delta) {
	if (!_auto_detect) {
		_cached_lin_vel = _velocity;
		_cached_ang_vel = _angular_velocity;
		return;
	}

	// Auto-detect velocity from parent physics body
	Node *p = get_parent();
	if (p) {
		if (RigidBody2D *rb = Object::cast_to<RigidBody2D>(p)) {
			_cached_lin_vel = rb->get_linear_velocity();
			_cached_ang_vel = rb->get_angular_velocity();
			return;
		}
		if (CharacterBody2D *cb = Object::cast_to<CharacterBody2D>(p)) {
			_cached_lin_vel = cb->get_velocity();
			_cached_ang_vel = 0.0f;
			return;
		}
	}
	_cached_lin_vel = Vector2(0, 0);
	_cached_ang_vel = 0.0f;
}

} // namespace godot
