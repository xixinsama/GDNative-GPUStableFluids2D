#pragma once

#include "godot_cpp/classes/node2d.hpp"
#include "godot_cpp/classes/rd_pipeline_compute.hpp"
#include "godot_cpp/classes/rd_shader_spirv.hpp"
#include "godot_cpp/classes/rd_uniform.hpp"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/classes/texture2drd.hpp"
#include "godot_cpp/variant/color.hpp"
#include "godot_cpp/variant/rid.hpp"

namespace godot {

class GPUStableFluids2D : public Node2D {
	GDCLASS(GPUStableFluids2D, Node2D)

private:
	// Simulation parameters
	int width = 128;
	int height = 128;
	float timestep = 1.0f;
	float grid_scale = 1.0f;
	float viscosity = 0.0f;
	int poisson_iterations = 50;
	float ink_longevity = 0.995f;
	Color ink_color = Color(1, 1, 1);
	bool vorticity_enabled = true;
	float vorticity_scale = 0.035f;

	// Rendering resources
	Ref<RenderingDevice> rd;
	RID shader;
	RID pipeline;
	RID uniform_set;
	RID impulse_buffer; // Uniform buffer for impulse data

	// Texture RIDs for simulation
	RID velocity_tex;
	RID density_tex;
	RID pressure_tex;
	RID divergence_tex;
	RID vorticity_tex;
	RID velocity_offsets_tex;
	RID pressure_offsets_tex;

	// Texture wrappers for Godot (to be used by e.g. Sprite2D)
	Ref<Texture2DRD> density_texture_wrapper;
	Ref<Texture2DRD> velocity_texture_wrapper;

	// State flags
	bool initialized = false;
	bool needs_recreate = false;

	// Internal methods
	void _create_textures();
	void _create_compute_pipeline();
	void _run_compute_shader();

protected:
	static void _bind_methods();

public:
	GPUStableFluids2D();
	~GPUStableFluids2D();

	// Resolution
	void set_resolution(int p_width, int p_height);
	int get_width() const;
	int get_height() const;

	// Time and grid
	void set_timestep(float p_timestep);
	float get_timestep() const;
	void set_grid_scale(float p_scale);
	float get_grid_scale() const;

	// Viscosity
	void set_viscosity(float p_viscosity);
	float get_viscosity() const;

	// Poisson solver iterations
	void set_poisson_iterations(int p_iterations);
	int get_poisson_iterations() const;

	// Ink (density) properties
	void set_ink_longevity(float p_longevity);
	float get_ink_longevity() const;
	void set_ink_color(const Color &p_color);
	Color get_ink_color() const;

	// Vorticity confinement
	void set_vorticity_enabled(bool p_enabled);
	bool is_vorticity_enabled() const;
	void set_vorticity_scale(float p_scale);
	float get_vorticity_scale() const;

	// Interaction
	void add_impulse(Vector2 position, Vector2 strength, float radius, bool add_ink = true);

	// Reset simulation
	void reset();

	// Access to density texture for display
	Ref<Texture2DRD> get_density_texture() const;
	Ref<Texture2DRD> get_velocity_texture() const;

	// Godot lifecycle overrides
	virtual void _ready() override;
	virtual void _process(double delta) override;
};

} // namespace godot
