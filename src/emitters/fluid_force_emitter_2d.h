#pragma once

#include "godot_cpp/classes/node2d.hpp"
#include "godot_cpp/variant/node_path.hpp"
#include "godot_cpp/variant/vector2.hpp"

#include "core/fluid_types.h"

namespace godot {

class GPUStableFluids2D;

// Force field emitter. Applies directional force patterns to the fluid simulation.
// Non-mask shapes are computed on GPU for zero CPU overhead.
class FluidForceEmitter2D : public Node2D {
	GDCLASS(FluidForceEmitter2D, Node2D)

public:
	FluidForceEmitter2D() = default;
	~FluidForceEmitter2D() override = default;

	void set_active(bool v);            bool is_active() const;
	void set_force_pattern(int v);      int get_force_pattern() const;
	void set_force(const Vector2 &v);   Vector2 get_force() const;
	void set_force_radius(float v);     float get_force_radius() const;
	void set_emission_shape(int v);     int get_emission_shape() const;
	void set_shape_size(const Vector2 &v); Vector2 get_shape_size() const;
	void set_falloff_exponent(float v); float get_falloff_exponent() const;
	void set_swirl_strength(float v);   float get_swirl_strength() const;
	void set_force_preset(int v);       int get_force_preset() const;
	void set_lifetime(float v);         float get_lifetime() const;
	void set_auto_destroy(bool v);      bool is_auto_destroy() const;
	void set_sim_target(const NodePath &v); NodePath get_sim_target() const;

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	bool _active = true;
	ForcePattern _force_pattern = ForcePattern::Directional;
	Vector2 _force = Vector2(0, -1);
	float _force_radius = 100.0f;
	EmissionShape _emission_shape = EmissionShape::Circle;
	Vector2 _shape_size = Vector2(1, 1);
	float _falloff_exponent = 2.0f;
	float _swirl_strength = 1.0f;
	ForcePreset _force_preset = ForcePreset::Custom;
	float _lifetime = 0.0f;
	bool _auto_destroy = false;
	NodePath _sim_target;
	float _life_timer = 0.0f;

	GPUStableFluids2D *_find_sim() const;
	void _apply_preset(ForcePreset p);
};

} // namespace godot
