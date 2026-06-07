#include "fluid_domain.h"
#include "GPU_stable_fluids_2D_init.h"

#include "godot_cpp/core/class_db.hpp"

namespace godot {

void FluidDomain2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_resolution", "resolution"), &FluidDomain2D::set_resolution);
	ClassDB::bind_method(D_METHOD("get_resolution"), &FluidDomain2D::get_resolution);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "resolution"), "set_resolution", "get_resolution");

	ClassDB::bind_method(D_METHOD("set_timestep", "ts"), &FluidDomain2D::set_timestep);
	ClassDB::bind_method(D_METHOD("get_timestep"), &FluidDomain2D::get_timestep);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "timestep"), "set_timestep", "get_timestep");

	ClassDB::bind_method(D_METHOD("set_grid_scale", "scale"), &FluidDomain2D::set_grid_scale);
	ClassDB::bind_method(D_METHOD("get_grid_scale"), &FluidDomain2D::get_grid_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "grid_scale"), "set_grid_scale", "get_grid_scale");

	ClassDB::bind_method(D_METHOD("set_viscosity", "v"), &FluidDomain2D::set_viscosity);
	ClassDB::bind_method(D_METHOD("get_viscosity"), &FluidDomain2D::get_viscosity);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "viscosity"), "set_viscosity", "get_viscosity");

	ClassDB::bind_method(D_METHOD("set_diffusion", "v"), &FluidDomain2D::set_diffusion);
	ClassDB::bind_method(D_METHOD("get_diffusion"), &FluidDomain2D::get_diffusion);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "diffusion"), "set_diffusion", "get_diffusion");

	ClassDB::bind_method(D_METHOD("set_jacobi_pressure_iterations", "n"), &FluidDomain2D::set_jacobi_pressure_iterations);
	ClassDB::bind_method(D_METHOD("get_jacobi_pressure_iterations"), &FluidDomain2D::get_jacobi_pressure_iterations);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "jacobi_pressure_iterations"), "set_jacobi_pressure_iterations", "get_jacobi_pressure_iterations");

	ClassDB::bind_method(D_METHOD("set_jacobi_velocity_iterations", "n"), &FluidDomain2D::set_jacobi_velocity_iterations);
	ClassDB::bind_method(D_METHOD("get_jacobi_velocity_iterations"), &FluidDomain2D::get_jacobi_velocity_iterations);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "jacobi_velocity_iterations"), "set_jacobi_velocity_iterations", "get_jacobi_velocity_iterations");

	ClassDB::bind_method(D_METHOD("set_vorticity_enabled", "v"), &FluidDomain2D::set_vorticity_enabled);
	ClassDB::bind_method(D_METHOD("is_vorticity_enabled"), &FluidDomain2D::is_vorticity_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "vorticity_enabled"), "set_vorticity_enabled", "is_vorticity_enabled");

	ClassDB::bind_method(D_METHOD("set_vorticity_scale", "v"), &FluidDomain2D::set_vorticity_scale);
	ClassDB::bind_method(D_METHOD("get_vorticity_scale"), &FluidDomain2D::get_vorticity_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vorticity_scale"), "set_vorticity_scale", "get_vorticity_scale");

	ClassDB::bind_method(D_METHOD("set_color_decay", "v"), &FluidDomain2D::set_color_decay);
	ClassDB::bind_method(D_METHOD("get_color_decay"), &FluidDomain2D::get_color_decay);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "color_decay"), "set_color_decay", "get_color_decay");

	ClassDB::bind_method(D_METHOD("set_velocity_decay", "v"), &FluidDomain2D::set_velocity_decay);
	ClassDB::bind_method(D_METHOD("get_velocity_decay"), &FluidDomain2D::get_velocity_decay);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "velocity_decay"), "set_velocity_decay", "get_velocity_decay");

	ClassDB::bind_method(D_METHOD("set_clear_color", "v"), &FluidDomain2D::set_clear_color);
	ClassDB::bind_method(D_METHOD("get_clear_color"), &FluidDomain2D::get_clear_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "clear_color"), "set_clear_color", "get_clear_color");

	ClassDB::bind_method(D_METHOD("apply_to", "sim"), &FluidDomain2D::apply_to);
}

void FluidDomain2D::set_resolution(const Vector2 &v) { _resolution = v; }
Vector2 FluidDomain2D::get_resolution() const { return _resolution; }
void FluidDomain2D::set_timestep(float v) { _timestep = v; }
float FluidDomain2D::get_timestep() const { return _timestep; }
void FluidDomain2D::set_grid_scale(float v) { _grid_scale = v; }
float FluidDomain2D::get_grid_scale() const { return _grid_scale; }
void FluidDomain2D::set_viscosity(float v) { _viscosity = v; }
float FluidDomain2D::get_viscosity() const { return _viscosity; }
void FluidDomain2D::set_diffusion(float v) { _diffusion = v; }
float FluidDomain2D::get_diffusion() const { return _diffusion; }
void FluidDomain2D::set_jacobi_pressure_iterations(int v) { _jacobi_pressure_iterations = v; }
int FluidDomain2D::get_jacobi_pressure_iterations() const { return _jacobi_pressure_iterations; }
void FluidDomain2D::set_jacobi_velocity_iterations(int v) { _jacobi_velocity_iterations = v; }
int FluidDomain2D::get_jacobi_velocity_iterations() const { return _jacobi_velocity_iterations; }
void FluidDomain2D::set_vorticity_enabled(bool v) { _vorticity_enabled = v; }
bool FluidDomain2D::is_vorticity_enabled() const { return _vorticity_enabled; }
void FluidDomain2D::set_vorticity_scale(float v) { _vorticity_scale = v; }
float FluidDomain2D::get_vorticity_scale() const { return _vorticity_scale; }
void FluidDomain2D::set_color_decay(float v) { _color_decay = v; }
float FluidDomain2D::get_color_decay() const { return _color_decay; }
void FluidDomain2D::set_velocity_decay(float v) { _velocity_decay = v; }
float FluidDomain2D::get_velocity_decay() const { return _velocity_decay; }
void FluidDomain2D::set_clear_color(const Color &v) { _clear_color = v; }
Color FluidDomain2D::get_clear_color() const { return _clear_color; }

void FluidDomain2D::apply_to(GPUStableFluids2D *p_sim) const {
	if (!p_sim) return;
	p_sim->set_resolution((int)_resolution.x, (int)_resolution.y);
	p_sim->set_timestep(_timestep);
	p_sim->set_grid_scale(_grid_scale);
	p_sim->set_viscosity(_viscosity);
	p_sim->set_diffusion(_diffusion);
	p_sim->set_poisson_iterations(_jacobi_pressure_iterations);
	p_sim->set_vorticity_enabled(_vorticity_enabled);
	p_sim->set_vorticity_scale(_vorticity_scale);
	p_sim->set_color_decay(_color_decay);
	p_sim->set_velocity_decay(_velocity_decay);
	p_sim->set_clear_color(_clear_color);
}

} // namespace godot
