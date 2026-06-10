#pragma once

#include "godot_cpp/classes/node2d.hpp"
#include "godot_cpp/variant/vector2.hpp"
#include "godot_cpp/variant/node_path.hpp"
#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/variant/color.hpp"
#include "godot_cpp/classes/texture2d.hpp"
#include "godot_cpp/classes/shape2d.hpp"

namespace godot {

// Interface for obstacles to expose velocity to the fluid simulator
class IFluidObstacle {
public:
	virtual ~IFluidObstacle() = default;
	virtual Vector2 get_object_linear_velocity()  const = 0;
	virtual float   get_object_angular_velocity() const = 0;
	virtual Vector2 get_object_center()           const = 0;
};

// Fluid obstacle node — similar to CollisionShape2D for PhysicsBody2D.
// Defines a rigid barrier that fluid cannot enter.  Moving obstacles push fluid.
// Place as child of a RigidBody2D / CharacterBody2D or standalone with manual velocity.
// Auto-detected by FluidObstacleDrawer via the "fluid_obstacles" group.
class FluidObstacle2D : public Node2D, public IFluidObstacle {
	GDCLASS(FluidObstacle2D, Node2D)

public:
	FluidObstacle2D() = default;
	~FluidObstacle2D() override = default;

	void set_auto_detect_velocity(bool p_enable);
	bool is_auto_detect_velocity() const;

	void set_velocity(const Vector2 &p_vel);
	Vector2 get_velocity() const;

	void set_angular_velocity(float p_av);
	float get_angular_velocity() const;

	void set_obstacle_texture(const Ref<Texture2D> &p_tex);
	Ref<Texture2D> get_obstacle_texture() const;

	void set_sim_target(Object *p_obj);
	Object *get_sim_target() const;

	void set_shape(const Ref<Shape2D> &p_shape);
	Ref<Shape2D> get_shape() const;

	void set_one_way_collision(bool p_enable);
	bool is_one_way_collision() const;

	void set_debug_color(const Color &p_color);
	Color get_debug_color() const;

	// IFluidObstacle interface
	Vector2 get_object_linear_velocity()  const override;
	float   get_object_angular_velocity() const override;
	Vector2 get_object_center()           const override;

	void _ready() override;
	void _physics_process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	bool _auto_detect = true;
	Vector2 _velocity;
	float _angular_velocity = 0.0f;
	Ref<Texture2D> _obstacle_texture;
	NodePath _sim_target;
	Ref<Shape2D> _shape;
	bool _one_way_collision = false;
	Color _debug_color = Color(1.0f, 0.2f, 0.2f, 0.4f);

	Vector2 _cached_lin_vel;
	float   _cached_ang_vel;
};

} // namespace godot
