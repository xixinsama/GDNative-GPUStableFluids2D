#pragma once

#include "godot_cpp/classes/node2d.hpp"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/texture2drd.hpp"
#include "godot_cpp/variant/color.hpp"
#include "godot_cpp/variant/rid.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"

#include "core/sim_params.h"
#include "core/fluid_types.h"
#include "core/gpu_resource_manager.h"
#include "core/fluid_render_pipeline.h"
#include "utils/draw_request.h"
#include "obstacles/fluid_obstacle_drawer.h"

namespace godot {

// ──────────────────────────────────────────────────────────
//  GPUStableFluids2D — Godot Node2D that runs a 2D
//  Navier-Stokes fluid simulation on the GPU via
//  RenderingDevice compute shaders.
// ──────────────────────────────────────────────────────────
class GPUStableFluids2D : public Node2D {
	GDCLASS(GPUStableFluids2D, Node2D)

public:
	GPUStableFluids2D();
	~GPUStableFluids2D() override;

	// ---- Property accessors ----
	void  set_resolution(int p_width, int p_height);
	void  set_width(int p_width);
	void  set_height(int p_height);
	int   get_width()  const;
	int   get_height() const;

	void  set_timestep(float p_timestep);
	float get_timestep() const;

	void  set_grid_scale(float p_scale);
	float get_grid_scale() const;

	void  set_viscosity(float p_viscosity);
	float get_viscosity() const;

	void  set_diffusion(float p_diffusion);
	float get_diffusion() const;

	void  set_poisson_iterations(int p_iterations);
	int   get_poisson_iterations() const;

	void  set_ink_longevity(float p_longevity);
	float get_ink_longevity() const;

	void  set_ink_color(const Color &p_color);
	Color get_ink_color() const;

	void  set_clear_color(const Color &p_color);
	Color get_clear_color() const;

	void  set_color_decay(float p_decay);
	float get_color_decay() const;

	void  set_velocity_decay(float p_decay);
	float get_velocity_decay() const;

	void  set_vorticity_enabled(bool p_enabled);
	bool  is_vorticity_enabled() const;

	void  set_vorticity_scale(float p_scale);
	float get_vorticity_scale() const;

	void  set_density_scale(float p_scale);
	float get_density_scale() const;

	void  set_obstacle_force_strength(float p_strength);
	float get_obstacle_force_strength() const;

	void  set_subtractive_mixing(bool p_enabled);
	bool  is_subtractive_mixing() const;

	void  set_follow_mode(int p_mode);
	int   get_follow_mode() const;

	void  set_follow_path(const NodePath &p_path);
	NodePath get_follow_path() const;

	void  set_fluid_world_size(Vector2 p_size);
	Vector2 get_fluid_world_size() const;

	void  set_domain_wrap_mode(int p_mode);
	int   get_domain_wrap_mode() const;

	// ---- Public API ----
	void add_impulse(Vector2 p_position, Vector2 p_strength, float p_radius, bool p_add_ink = true);
	void queue_draw_request(Vector2 p_pos, Color p_color, Vector2 p_vel,
	                        float p_color_radius, float p_velocity_radius);
	void clear_draw_requests();
	void reset();

	Ref<Texture2DRD> get_output_texture() const;
	Ref<Texture2DRD> get_velocity_texture() const;
	Ref<Texture2DRD> get_pressure_texture() const;
	Ref<Texture2DRD> get_divergence_texture() const;
	Vector2          world_to_fluid_pos(Vector2 p_world_pos) const;
	Vector2          sample_velocity(Vector2 p_world_pos) const;
	Vector2          get_domain_offset() const;
	GPUResourceManager *get_gpu_resources() { return &_gpu_resources; }
		const DrawRequest *get_draw_requests() const { return _draw_requests; }
		int get_draw_request_count() const { return _draw_request_count; }

	void _ready() override;
	void _process(double p_delta) override;

	PackedStringArray _get_configuration_warnings() const;

protected:
	static void _bind_methods();

private:
	void _initialise_gpu();
	void _recreate_gpu_resources();
	void _upload_batch_data();
	void _gpu_process_on_render_thread(double p_delta, Vector2 p_domain_offset);
	void _deferred_clear_textures(const Color &p_color);

	// ---- Simulation parameters ----
	int   width           = 512;
	int   height          = 512;
	float timestep        = 1.0f;
	float grid_scale      = 1.0f;
	float viscosity       = 0.0f;
	float diffusion       = 0.0f;
	int   poisson_iterations = 40;
	float ink_longevity   = 0.995f;
	Color ink_color       = Color(1, 1, 1, 1);
	Color clear_color     = Color(0, 0, 0, 0);
	float color_decay     = 0.0005f;
	float velocity_decay  = -1.0f;
	bool  vorticity_enabled = true;
	float vorticity_scale = 0.4f;
	float density_scale   = 1.0f;
	float obstacle_force_strength = 5.0f;
	bool  subtractive_mixing = false;

	FollowMode     follow_mode = FollowMode::Camera2D;
	NodePath       follow_path;
	Vector2        fluid_world_size = Vector2(1920, 1080);
	DomainWrapMode domain_wrap_mode = DomainWrapMode::Toroidal;

	// ---- GPU state ----
	GPUResourceManager  _gpu_resources;
	FluidRenderPipeline _render_pipeline;
	FluidObstacleDrawer _obstacle_drawer;
	Ref<Texture2DRD>    _output_texture;
	Ref<Texture2DRD>    _output_velocity_texture;
	Ref<Texture2DRD>    _output_pressure_texture;
	Ref<Texture2DRD>    _output_divergence_texture;

	// ---- Domain following ----
	Node2D *_follow_node = nullptr;
	Vector2 _previous_follow_pos;

	// ---- Draw request ring buffer ----
	static constexpr int MAX_DRAW_REQUESTS = 4096;
	DrawRequest _draw_requests[MAX_DRAW_REQUESTS];
	int         _draw_request_count = 0;

	// ---- Flags ----
	bool _initialized    = false;
	bool _needs_recreate = false;

	int _frame_count = 0;
	int _x_groups = 0;
	int _y_groups = 0;
};

} // namespace godot
