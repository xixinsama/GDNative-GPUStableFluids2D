#pragma once

#include "godot_cpp/classes/node2d.hpp"
#include "godot_cpp/variant/node_path.hpp"
#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/variant/color.hpp"
#include "godot_cpp/variant/vector2.hpp"

#include "core/fluid_types.h"

namespace godot {

// Fluid particle emitter. Configurable emission mode, shape, velocity pattern, and presets.
class FluidEmitter2D : public Node2D {
	GDCLASS(FluidEmitter2D, Node2D)

public:
	FluidEmitter2D() = default;
	~FluidEmitter2D() override = default;

	void set_active(bool v);           bool is_active() const;
	void set_emission_mode(int v);     int get_emission_mode() const;
	void set_emission_shape(int v);    int get_emission_shape() const;
	void set_emission_preset(int v);   int get_emission_preset() const;
	void set_emit_color(const Color &v); Color get_emit_color() const;
	void set_emit_velocity(const Vector2 &v); Vector2 get_emit_velocity() const;
	void set_color_radius(float v);    float get_color_radius() const;
	void set_velocity_radius(float v); float get_velocity_radius() const;
	void set_emit_interval(float v);   float get_emit_interval() const;
	void set_burst_count(int v);       int get_burst_count() const;
	void set_shape_size(const Vector2 &v); Vector2 get_shape_size() const;
	void set_velocity_pattern(int v);  int get_velocity_pattern() const;
	void set_swirl_strength(float v);  float get_swirl_strength() const;
	void set_lifetime(float v);        float get_lifetime() const;
	void set_auto_destroy(bool v);     bool is_auto_destroy() const;
	void set_sim_target(Object *p_obj); Object *get_sim_target() const;

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	bool _active = true;
	EmissionMode _emission_mode = EmissionMode::Continuous;
	EmissionShape _emission_shape = EmissionShape::Point;
	EmitterPreset _emission_preset = EmitterPreset::Custom;
	Color _emit_color = Color(0.5f, 0.5f, 0.5f, 0.5f);
	Vector2 _emit_velocity = Vector2(0, -1);
	float _color_radius = 0.5f;
	float _velocity_radius = 0.8f;
	float _emit_interval = 0.05f;
	int _burst_count = 16;
	Vector2 _shape_size = Vector2(1, 1);
	VelocityPattern _velocity_pattern = VelocityPattern::Directional;
	float _swirl_strength = 0.0f;
	float _lifetime = 0.0f;
	bool _auto_destroy = false;
	NodePath _sim_target;

	float _timer = 0.0f;
	float _life_timer = 0.0f;
	bool _has_emitted = false;

	class GPUStableFluids2D *_find_sim() const;
	void _apply_preset(EmitterPreset p);
	Vector2 _sample_shape_position() const;
	Vector2 _compute_velocity(const Vector2 &p_pos) const;
	void _emit_particles(int p_count);
};

} // namespace godot
