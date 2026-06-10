#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/variant/vector2.hpp"
#include "godot_cpp/variant/color.hpp"

namespace godot {

// Saveable resource that encapsulates fluid simulation domain configuration.
// Allows sharing the same config across multiple simulation nodes.
class FluidDomain2D : public RefCounted {
	GDCLASS(FluidDomain2D, RefCounted)

public:
	FluidDomain2D() = default;
	~FluidDomain2D() override = default;

	void set_resolution(const Vector2 &v); Vector2 get_resolution() const;
	void set_timestep(float v);           float get_timestep() const;
	void set_grid_scale(float v);         float get_grid_scale() const;
	void set_viscosity(float v);          float get_viscosity() const;
	void set_diffusion(float v);          float get_diffusion() const;
	void set_jacobi_pressure_iterations(int v); int get_jacobi_pressure_iterations() const;
	void set_jacobi_velocity_iterations(int v); int get_jacobi_velocity_iterations() const;
	void set_vorticity_enabled(bool v);   bool is_vorticity_enabled() const;
	void set_vorticity_scale(float v);    float get_vorticity_scale() const;
	void set_color_decay(float v);        float get_color_decay() const;
	void set_velocity_decay(float v);     float get_velocity_decay() const;
	void set_clear_color(const Color &v); Color get_clear_color() const;

	// Apply these settings to a GPUStableFluids2D node
	void apply_to(class GPUStableFluids2D *p_sim) const;

protected:
	static void _bind_methods();

private:
	Vector2 _resolution = Vector2(512, 512);
	float _timestep = 1.0f;
	float _grid_scale = 1.0f;
	float _viscosity = 0.0f;
	float _diffusion = 0.0f;
	int _jacobi_pressure_iterations = 40;
	int _jacobi_velocity_iterations = 10;
	bool _vorticity_enabled = true;
	float _vorticity_scale = 0.4f;
	float _color_decay = 0.0f;
	float _velocity_decay = -1.0f;
	Color _clear_color = Color(0, 0, 0, 0);
};

} // namespace godot
