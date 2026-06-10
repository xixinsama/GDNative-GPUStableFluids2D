#include "GPU_stable_fluids_2D_init.h"

#include "core/fluid_debug_config.h"
#include "display/fluid_display_2d.h"
#include "godot_cpp/classes/camera2d.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/classes/viewport.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

#include <cstring>
#include <cmath>

namespace godot {

// ──────────────────────────────────────────────────────────
//  Constructor / Destructor
// ──────────────────────────────────────────────────────────

GPUStableFluids2D::GPUStableFluids2D() {}
GPUStableFluids2D::~GPUStableFluids2D() {
	_gpu_resources.terminate();
}

// ──────────────────────────────────────────────────────────
//  _bind_methods
// ──────────────────────────────────────────────────────────

void GPUStableFluids2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_resolution", "width", "height"), &GPUStableFluids2D::set_resolution);
	ClassDB::bind_method(D_METHOD("get_width"),  &GPUStableFluids2D::get_width);
	ClassDB::bind_method(D_METHOD("get_height"), &GPUStableFluids2D::get_height);
	ClassDB::bind_method(D_METHOD("set_width", "w"), &GPUStableFluids2D::set_width);
	ClassDB::bind_method(D_METHOD("set_height", "h"), &GPUStableFluids2D::set_height);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "width",  PROPERTY_HINT_RANGE, "32,2048"),  "set_width", "get_width");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "height", PROPERTY_HINT_RANGE, "32,2048"),  "set_height", "get_height");

	ClassDB::bind_method(D_METHOD("set_timestep", "timestep"), &GPUStableFluids2D::set_timestep);
	ClassDB::bind_method(D_METHOD("get_timestep"), &GPUStableFluids2D::get_timestep);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "timestep", PROPERTY_HINT_RANGE, "0.1,10.0,0.1"), "set_timestep", "get_timestep");

	ClassDB::bind_method(D_METHOD("set_grid_scale", "scale"), &GPUStableFluids2D::set_grid_scale);
	ClassDB::bind_method(D_METHOD("get_grid_scale"), &GPUStableFluids2D::get_grid_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "grid_scale", PROPERTY_HINT_RANGE, "0.1,100.0,0.1"), "set_grid_scale", "get_grid_scale");

	ClassDB::bind_method(D_METHOD("set_viscosity", "viscosity"), &GPUStableFluids2D::set_viscosity);
	ClassDB::bind_method(D_METHOD("get_viscosity"), &GPUStableFluids2D::get_viscosity);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "viscosity", PROPERTY_HINT_RANGE, "0.0,0.01,0.0001"), "set_viscosity", "get_viscosity");

	ClassDB::bind_method(D_METHOD("set_diffusion", "diffusion"), &GPUStableFluids2D::set_diffusion);
	ClassDB::bind_method(D_METHOD("get_diffusion"), &GPUStableFluids2D::get_diffusion);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "diffusion", PROPERTY_HINT_RANGE, "0.0,0.01,0.0001"), "set_diffusion", "get_diffusion");

	ClassDB::bind_method(D_METHOD("set_poisson_iterations", "iterations"), &GPUStableFluids2D::set_poisson_iterations);
	ClassDB::bind_method(D_METHOD("get_poisson_iterations"), &GPUStableFluids2D::get_poisson_iterations);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "poisson_iterations", PROPERTY_HINT_RANGE, "1,200"), "set_poisson_iterations", "get_poisson_iterations");

	ClassDB::bind_method(D_METHOD("set_ink_longevity", "longevity"), &GPUStableFluids2D::set_ink_longevity);
	ClassDB::bind_method(D_METHOD("get_ink_longevity"), &GPUStableFluids2D::get_ink_longevity);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ink_longevity", PROPERTY_HINT_RANGE, "0.9,1.0,0.001"), "set_ink_longevity", "get_ink_longevity");

	ClassDB::bind_method(D_METHOD("set_ink_color", "color"), &GPUStableFluids2D::set_ink_color);
	ClassDB::bind_method(D_METHOD("get_ink_color"), &GPUStableFluids2D::get_ink_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "ink_color"), "set_ink_color", "get_ink_color");

	ClassDB::bind_method(D_METHOD("set_clear_color", "color"), &GPUStableFluids2D::set_clear_color);
	ClassDB::bind_method(D_METHOD("get_clear_color"), &GPUStableFluids2D::get_clear_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "clear_color"), "set_clear_color", "get_clear_color");

	ClassDB::bind_method(D_METHOD("set_color_decay", "decay"), &GPUStableFluids2D::set_color_decay);
	ClassDB::bind_method(D_METHOD("get_color_decay"), &GPUStableFluids2D::get_color_decay);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "color_decay", PROPERTY_HINT_RANGE, "0.0,1.0,0.0001"), "set_color_decay", "get_color_decay");

	ClassDB::bind_method(D_METHOD("set_velocity_decay", "decay"), &GPUStableFluids2D::set_velocity_decay);
	ClassDB::bind_method(D_METHOD("get_velocity_decay"), &GPUStableFluids2D::get_velocity_decay);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "velocity_decay", PROPERTY_HINT_RANGE, "-1.0,1.0,0.001"), "set_velocity_decay", "get_velocity_decay");

	ClassDB::bind_method(D_METHOD("set_subtractive_mixing", "enabled"), &GPUStableFluids2D::set_subtractive_mixing);
	ClassDB::bind_method(D_METHOD("is_subtractive_mixing"), &GPUStableFluids2D::is_subtractive_mixing);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "subtractive_mixing"), "set_subtractive_mixing", "is_subtractive_mixing");

	ClassDB::bind_method(D_METHOD("set_vorticity_enabled", "enabled"), &GPUStableFluids2D::set_vorticity_enabled);
	ClassDB::bind_method(D_METHOD("is_vorticity_enabled"), &GPUStableFluids2D::is_vorticity_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "vorticity_enabled"), "set_vorticity_enabled", "is_vorticity_enabled");

	ClassDB::bind_method(D_METHOD("set_vorticity_scale", "scale"), &GPUStableFluids2D::set_vorticity_scale);
	ClassDB::bind_method(D_METHOD("get_vorticity_scale"), &GPUStableFluids2D::get_vorticity_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vorticity_scale", PROPERTY_HINT_RANGE, "0.0,5.0,0.001"), "set_vorticity_scale", "get_vorticity_scale");

	ClassDB::bind_method(D_METHOD("set_density_scale", "scale"), &GPUStableFluids2D::set_density_scale);
	ClassDB::bind_method(D_METHOD("get_density_scale"), &GPUStableFluids2D::get_density_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "density_scale", PROPERTY_HINT_RANGE, "0.1,10.0,0.1"), "set_density_scale", "get_density_scale");

	ClassDB::bind_method(D_METHOD("set_obstacle_force_strength", "strength"), &GPUStableFluids2D::set_obstacle_force_strength);
	ClassDB::bind_method(D_METHOD("get_obstacle_force_strength"), &GPUStableFluids2D::get_obstacle_force_strength);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "obstacle_force_strength", PROPERTY_HINT_RANGE, "0.0,100.0,0.1"), "set_obstacle_force_strength", "get_obstacle_force_strength");

	ClassDB::bind_method(D_METHOD("set_follow_mode", "mode"), &GPUStableFluids2D::set_follow_mode);
	ClassDB::bind_method(D_METHOD("get_follow_mode"), &GPUStableFluids2D::get_follow_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "follow_mode", PROPERTY_HINT_ENUM, "Camera2D,Node2D,None"), "set_follow_mode", "get_follow_mode");

	ClassDB::bind_method(D_METHOD("set_follow_path", "path"), &GPUStableFluids2D::set_follow_path);
	ClassDB::bind_method(D_METHOD("get_follow_path"), &GPUStableFluids2D::get_follow_path);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "follow_path"), "set_follow_path", "get_follow_path");

	ClassDB::bind_method(D_METHOD("set_fluid_world_size", "size"), &GPUStableFluids2D::set_fluid_world_size);
	ClassDB::bind_method(D_METHOD("get_fluid_world_size"), &GPUStableFluids2D::get_fluid_world_size);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "fluid_world_size"), "set_fluid_world_size", "get_fluid_world_size");

	ClassDB::bind_method(D_METHOD("set_domain_wrap_mode", "mode"), &GPUStableFluids2D::set_domain_wrap_mode);
	ClassDB::bind_method(D_METHOD("get_domain_wrap_mode"), &GPUStableFluids2D::get_domain_wrap_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "domain_wrap_mode", PROPERTY_HINT_ENUM, "Toroidal,Clamp"), "set_domain_wrap_mode", "get_domain_wrap_mode");

	ClassDB::bind_method(D_METHOD("add_impulse", "position", "strength", "radius", "add_ink"), &GPUStableFluids2D::add_impulse, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("queue_draw_request", "pos", "color", "vel", "color_radius", "velocity_radius"), &GPUStableFluids2D::queue_draw_request);
	ClassDB::bind_method(D_METHOD("clear_draw_requests"), &GPUStableFluids2D::clear_draw_requests);
	ClassDB::bind_method(D_METHOD("reset"), &GPUStableFluids2D::reset);
	ClassDB::bind_method(D_METHOD("get_output_texture"), &GPUStableFluids2D::get_output_texture);
	ClassDB::bind_method(D_METHOD("get_velocity_texture"), &GPUStableFluids2D::get_velocity_texture);
	ClassDB::bind_method(D_METHOD("get_pressure_texture"), &GPUStableFluids2D::get_pressure_texture);
	ClassDB::bind_method(D_METHOD("get_divergence_texture"), &GPUStableFluids2D::get_divergence_texture);
	ClassDB::bind_method(D_METHOD("world_to_fluid_pos", "world_pos"), &GPUStableFluids2D::world_to_fluid_pos);
	ClassDB::bind_method(D_METHOD("sample_velocity", "world_pos"), &GPUStableFluids2D::sample_velocity);
	ClassDB::bind_method(D_METHOD("get_domain_offset"), &GPUStableFluids2D::get_domain_offset);

	ADD_SIGNAL(MethodInfo("simulation_initialized"));
	ADD_SIGNAL(MethodInfo("simulation_reset"));
	ADD_SIGNAL(MethodInfo("obstacle_changed"));
}

// ──────────────────────────────────────────────────────────
//  Getters / Setters
// ──────────────────────────────────────────────────────────

void  GPUStableFluids2D::set_resolution(int p_width, int p_height) { width = p_width; height = p_height; _needs_recreate = true; }
void  GPUStableFluids2D::set_width(int p_width)   { width = p_width; _needs_recreate = true; }
void  GPUStableFluids2D::set_height(int p_height)  { height = p_height; _needs_recreate = true; }
int   GPUStableFluids2D::get_width()  const { return width; }
int   GPUStableFluids2D::get_height() const { return height; }
void  GPUStableFluids2D::set_timestep(float v)          { timestep = v; }
float GPUStableFluids2D::get_timestep()           const { return timestep; }
void  GPUStableFluids2D::set_grid_scale(float v)         { grid_scale = v; }
float GPUStableFluids2D::get_grid_scale()          const { return grid_scale; }
void  GPUStableFluids2D::set_viscosity(float v)          { viscosity = v; }
float GPUStableFluids2D::get_viscosity()           const { return viscosity; }
void  GPUStableFluids2D::set_diffusion(float v)          { diffusion = v; }
float GPUStableFluids2D::get_diffusion()           const { return diffusion; }
void  GPUStableFluids2D::set_poisson_iterations(int v)   { poisson_iterations = v; }
int   GPUStableFluids2D::get_poisson_iterations()  const { return poisson_iterations; }
void  GPUStableFluids2D::set_ink_longevity(float v)      { ink_longevity = v; }
float GPUStableFluids2D::get_ink_longevity()       const { return ink_longevity; }
void  GPUStableFluids2D::set_ink_color(const Color &v)   { ink_color = v; }
Color GPUStableFluids2D::get_ink_color()           const { return ink_color; }
void  GPUStableFluids2D::set_clear_color(const Color &v) { clear_color = v; }
Color GPUStableFluids2D::get_clear_color()         const { return clear_color; }
void  GPUStableFluids2D::set_color_decay(float v)        { color_decay = v; }
float GPUStableFluids2D::get_color_decay()         const { return color_decay; }
void  GPUStableFluids2D::set_velocity_decay(float v)     { velocity_decay = v; }
float GPUStableFluids2D::get_velocity_decay()      const { return velocity_decay; }
void  GPUStableFluids2D::set_vorticity_enabled(bool v)   { vorticity_enabled = v; }
bool  GPUStableFluids2D::is_vorticity_enabled()    const { return vorticity_enabled; }
void  GPUStableFluids2D::set_vorticity_scale(float v)    { vorticity_scale = v; }
float GPUStableFluids2D::get_vorticity_scale()     const { return vorticity_scale; }
void  GPUStableFluids2D::set_density_scale(float v)      { density_scale = v; }
float GPUStableFluids2D::get_density_scale()       const { return density_scale; }
void  GPUStableFluids2D::set_obstacle_force_strength(float v) { obstacle_force_strength = v; }
float GPUStableFluids2D::get_obstacle_force_strength()  const { return obstacle_force_strength; }
void  GPUStableFluids2D::set_subtractive_mixing(bool v)  { subtractive_mixing = v; }
bool  GPUStableFluids2D::is_subtractive_mixing()   const { return subtractive_mixing; }
void  GPUStableFluids2D::set_follow_mode(int v) { follow_mode = (FollowMode)v; }
int   GPUStableFluids2D::get_follow_mode() const { return (int)follow_mode; }
void  GPUStableFluids2D::set_follow_path(const NodePath &v) { follow_path = v; }
NodePath GPUStableFluids2D::get_follow_path()      const { return follow_path; }
void  GPUStableFluids2D::set_fluid_world_size(Vector2 v)  { fluid_world_size = v; }
Vector2 GPUStableFluids2D::get_fluid_world_size()  const { return fluid_world_size; }
void  GPUStableFluids2D::set_domain_wrap_mode(int v) { domain_wrap_mode = (DomainWrapMode)v; }
int   GPUStableFluids2D::get_domain_wrap_mode() const { return (int)domain_wrap_mode; }

// ──────────────────────────────────────────────────────────
//  Godot lifecycle
// ──────────────────────────────────────────────────────────

void GPUStableFluids2D::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) return;

	// Resolve follow node
	if (follow_mode == FollowMode::Node2D && !follow_path.is_empty()) {
		_follow_node = Object::cast_to<Node2D>(get_node_or_null(follow_path));
		if (_follow_node) _previous_follow_pos = _follow_node->get_global_position();
	}

	_initialise_gpu();
	add_to_group("fluid_sim_nodes");

	// Push output textures to any FluidDisplay2D children (GPU is ready now)
	_bind_display_children();
}

void GPUStableFluids2D::_bind_display_children() {
	TypedArray<Node> children = get_children();
	for (int i = 0; i < children.size(); i++) {
		FluidDisplay2D *disp = Object::cast_to<FluidDisplay2D>(children[i]);
		if (disp) {
			disp->bind_to_sim(this);
		}
	}
}

PackedStringArray GPUStableFluids2D::_get_configuration_warnings() const {
	// Check for FluidDisplay2D child node — required to render the simulation output.
	// Mirrors the pattern used by PhysicsBody2D → CollisionShape2D.
	TypedArray<Node> children = get_children();
	for (int i = 0; i < children.size(); i++) {
		if (Object::cast_to<FluidDisplay2D>(children[i])) {
			return PackedStringArray(); // Display child found — no warning.
		}
	}
	PackedStringArray warnings;
	warnings.append(String("This node has no FluidDisplay2D child, so the fluid simulation output will not be visible.\nAdd a FluidDisplay2D child node to display the fluid."));
	return warnings;
}

void GPUStableFluids2D::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;
	if (!_initialized) return;

		// Bind any FluidDisplay2D children that appeared after GPU init
		_bind_display_children();

	if (_needs_recreate) {
		_recreate_gpu_resources();
		_needs_recreate = false;
	}

	// ── Domain following (main-thread-safe: reads node/camera transforms) ──
	Vector2 domain_offset(0, 0);

	if (follow_mode == FollowMode::Camera2D) {
		Viewport *vp = get_viewport();
		if (vp) {
			Camera2D *cam = vp->get_camera_2d();
			if (cam) {
				Vector2 current = cam->get_global_position();
				domain_offset = (current - _previous_follow_pos) / fluid_world_size;
				_previous_follow_pos = current;
			}
		}
	} else if (follow_mode == FollowMode::Node2D && _follow_node) {
		Vector2 current = _follow_node->get_global_position();
		domain_offset = (current - _previous_follow_pos) / fluid_world_size;
		_previous_follow_pos = current;
	}

	// ── GPU work: ALL RenderingDevice command submission MUST run on the ──
	// ── render thread. compute_list_begin, texture_copy, buffer_update,  ──
	// ── etc. from the main thread cause Vulkan command-buffer corruption  ──
	// ── and instant crashes (see Godot 4 RenderingDevice documentation).  ──
	RenderingServer::get_singleton()->call_on_render_thread(
		callable_mp(this, &GPUStableFluids2D::_gpu_process_on_render_thread).bind(p_delta, domain_offset)
	);
}

// ── Render-thread callback: all RenderingDevice commands run here ──

void GPUStableFluids2D::_gpu_process_on_render_thread(double p_delta, Vector2 domain_offset) {
	RenderingDevice *rd = _gpu_resources.device;
	if (!rd) return;

	if (_frame_count == 0) {
		FLUID_PRINT("[GPUStableFluids2D] _gpu_process_on_render_thread() first frame START");
	}

	// --- Domain shift (wrapping / camera following) ---
	if (domain_offset.length_squared() > 0.000001f) {
		ShiftTexturePushConstant shift;
		shift.resolution[0] = (float)width;
		shift.resolution[1] = (float)height;
		shift.offset[0] = domain_offset.x;
		shift.offset[1] = domain_offset.y;

		PackedByteArray pc;
		pc.resize(sizeof(ShiftTexturePushConstant));
		std::memcpy(pc.ptrw(), &shift, sizeof(ShiftTexturePushConstant));

		// Shift color
		{
			int64_t cl = rd->compute_list_begin();
			rd->compute_list_bind_compute_pipeline(cl, _gpu_resources.shift_texture_pipeline.pipeline_id);

			RID us0 = _gpu_resources.get_or_create_cached_uniform_set(
				_gpu_resources.shift_texture_pipeline.shader_id,
				_gpu_resources.tex_color, 0,
				RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
			rd->compute_list_bind_uniform_set(cl, us0, 0);

			RID us1 = _gpu_resources.get_or_create_cached_uniform_set(
				_gpu_resources.shift_texture_pipeline.shader_id,
				_gpu_resources.tex_temp, 1,
				RenderingDevice::UNIFORM_TYPE_IMAGE);
			rd->compute_list_bind_uniform_set(cl, us1, 1);

			rd->compute_list_set_push_constant(cl, pc, pc.size());
			rd->compute_list_dispatch(cl, _x_groups, _y_groups, 1);
			rd->compute_list_end();
		}
		std::swap(_gpu_resources.tex_color, _gpu_resources.tex_temp);

		// Shift velocity
		{
			int64_t cl = rd->compute_list_begin();
			rd->compute_list_bind_compute_pipeline(cl, _gpu_resources.shift_texture_pipeline.pipeline_id);

			RID us0 = _gpu_resources.get_or_create_cached_uniform_set(
				_gpu_resources.shift_texture_pipeline.shader_id,
				_gpu_resources.tex_velocity, 0,
				RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
			rd->compute_list_bind_uniform_set(cl, us0, 0);

			RID us1 = _gpu_resources.get_or_create_cached_uniform_set(
				_gpu_resources.shift_texture_pipeline.shader_id,
				_gpu_resources.tex_temp, 1,
				RenderingDevice::UNIFORM_TYPE_IMAGE);
			rd->compute_list_bind_uniform_set(cl, us1, 1);

			rd->compute_list_set_push_constant(cl, pc, pc.size());
			rd->compute_list_dispatch(cl, _x_groups, _y_groups, 1);
			rd->compute_list_end();
		}
		std::swap(_gpu_resources.tex_velocity, _gpu_resources.tex_temp);
	}

	// --- Run the full simulation pipeline ---
	// Upload draw requests to GPU before pipeline
	_upload_batch_data();

	// Process obstacles (CPU rasterization → GPU upload)
	_obstacle_drawer.process_frame();

	_render_pipeline.execute((float)p_delta, this);

	// --- Copy compute output to display texture ---
	// tex_color has STORAGE_BIT → GENERAL layout after compute.
	// tex_display has no STORAGE_BIT → SHADER_READ_ONLY_OPTIMAL layout.
	// texture_copy() handles Vulkan layout transitions for both source and dest.
	rd->texture_copy(
		_gpu_resources.tex_color,   // source
		_gpu_resources.tex_display, // dest
		Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3((float)width, (float)height, 1),
		0, 0, 0, 0);

		// Copy internal fields to display textures for alternative visualization modes.
		rd->texture_copy(
			_gpu_resources.tex_velocity, _gpu_resources.tex_display_velocity,
			Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3((float)width, (float)height, 1),
			0, 0, 0, 0);
		rd->texture_copy(
			_gpu_resources.tex_pressure, _gpu_resources.tex_display_pressure,
			Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3((float)width, (float)height, 1),
			0, 0, 0, 0);
		rd->texture_copy(
			_gpu_resources.tex_divergence, _gpu_resources.tex_display_divergence,
			Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3((float)width, (float)height, 1),
			0, 0, 0, 0);

		// Computation complete -- clear this frame's draw requests
		_draw_request_count = 0;
		if (_frame_count == 0) {
			FLUID_PRINT('[GPUStableFluids2D] _gpu_process_on_render_thread() first frame DONE');
		}
		_frame_count++;
}

// ──────────────────────────────────────────────────────────
//  GPU initialisation
// ──────────────────────────────────────────────────────────

void GPUStableFluids2D::_upload_batch_data() {
	if (_draw_request_count == 0) return;
	RenderingDevice *rd = _gpu_resources.device;
	if (!rd) return;

	// Compute domain center for world-to-UV conversion
	Vector2 domain_center(0, 0);
	if (follow_mode == FollowMode::Node2D && _follow_node) {
		domain_center = _follow_node->get_global_position();
	} else if (follow_mode == FollowMode::Camera2D) {
		domain_center = _previous_follow_pos;
	}

	// Pack draw requests into a single byte array with UV coordinates
	PackedByteArray pba;
	pba.resize(_draw_request_count * sizeof(BatchPoint));
	uint8_t *dst = pba.ptrw();

	for (int i = 0; i < _draw_request_count; i++) {
		const DrawRequest &req = _draw_requests[i];

		// Convert world position to UV [0,1]
		Vector2 local = req.position - domain_center;
		float u = local.x / fluid_world_size.x + 0.5f;
		float v = local.y / fluid_world_size.y + 0.5f;

		BatchPoint bp;
		bp.pos[0] = u;
		bp.pos[1] = v;
		bp.vel[0] = req.velocity.x;
		bp.vel[1] = req.velocity.y;
		bp.color[0] = req.color.r;
		bp.color[1] = req.color.g;
		bp.color[2] = req.color.b;
		bp.color[3] = req.color.a;
		bp.color_radius = req.color_radius;
		bp.vel_radius = req.velocity_radius;
		bp._pad[0] = 0.0f;
		bp._pad[1] = 0.0f;

		std::memcpy(dst + i * sizeof(BatchPoint), &bp, sizeof(BatchPoint));
	}

	rd->buffer_update(_gpu_resources.batch_buffer, 0, pba.size(), pba);
}

// Wrapper so callable_mp works — GPUResourceManager is not a Godot Object,
// so we can't create a method pointer callable directly on it.
void GPUStableFluids2D::_deferred_clear_textures(const Color &p_color) {
	_gpu_resources.clear_textures(p_color);
}

void GPUStableFluids2D::_initialise_gpu() {
	RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();
	if (!rd) {
		FLUID_PRINTERR("[GPUStableFluids2D] ERROR: No RenderingDevice available! GPU init skipped. "
			"This is expected in Compatibility renderer. Switch to Forward+ or Mobile for GPU compute support.");
		return;
	}
	FLUID_PRINT("[GPUStableFluids2D] RenderingDevice obtained: ", rd->get_device_name());

	_gpu_resources.initialize(
		rd,
		width, height, subtractive_mixing, clear_color, 4096, 32);

	// Verify critical pipelines loaded successfully
	bool all_ok = true;
	auto check_pipeline = [&](const ComputePipeline &p, const char *name) {
		if (!p.is_valid()) {
			FLUID_PRINTERR("[GPUStableFluids2D] ERROR: Pipeline '", name, "' failed to load!");
			all_ok = false;
		}
	};
	check_pipeline(_gpu_resources.advect_pipeline, "advect");
	check_pipeline(_gpu_resources.jacobi_pipeline, "jacobi");
	check_pipeline(_gpu_resources.divergence_pipeline, "divergence");
	check_pipeline(_gpu_resources.subtract_pipeline, "subtract");
	check_pipeline(_gpu_resources.boundary_pipeline, "boundary");
	check_pipeline(_gpu_resources.vorticity_pipeline, "vorticity");
	check_pipeline(_gpu_resources.splat_batch_pipeline, "splat_batch");
	check_pipeline(_gpu_resources.obstacle_force_pipeline, "obstacle_force");
	check_pipeline(_gpu_resources.shift_texture_pipeline, "shift_texture");
	check_pipeline(_gpu_resources.copy_texture_pipeline, "copy_texture");
	check_pipeline(_gpu_resources.color_decay_pipeline, "color_decay");
	check_pipeline(_gpu_resources.apply_force_emitter_pipeline, "apply_force_emitter");

	if (!all_ok) {
		FLUID_PRINTERR("[GPUStableFluids2D] ERROR: Some GPU pipelines failed to load. Simulation will be disabled.");
		_gpu_resources.terminate();
		return;
	}

	_x_groups = (width  + 7) / 8;
	_y_groups = (height + 7) / 8;

	_render_pipeline.initialize(&_gpu_resources, _x_groups, _y_groups);

	_output_texture.instantiate();
	_output_texture->set_texture_rd_rid(_gpu_resources.tex_display);

	_output_velocity_texture.instantiate();
	_output_velocity_texture->set_texture_rd_rid(_gpu_resources.tex_display_velocity);

	_output_pressure_texture.instantiate();
	_output_pressure_texture->set_texture_rd_rid(_gpu_resources.tex_display_pressure);

	_output_divergence_texture.instantiate();
	_output_divergence_texture->set_texture_rd_rid(_gpu_resources.tex_display_divergence);

	_obstacle_drawer.initialize(this);

	// Defer initial texture clear to the render thread.
	// texture_clear must NOT be called from the main thread — it submits GPU
	// commands and would corrupt the Vulkan command buffer.
	RenderingServer::get_singleton()->call_on_render_thread(
		callable_mp(this, &GPUStableFluids2D::_deferred_clear_textures).bind(clear_color)
	);

	_initialized = true;
	FLUID_PRINT("[GPUStableFluids2D] GPU initialization complete. All pipelines ready.");
	emit_signal("simulation_initialized");
}

void GPUStableFluids2D::_recreate_gpu_resources() {
	_initialized = false;
	_gpu_resources.terminate();
	_initialise_gpu();
}

// ──────────────────────────────────────────────────────────
//  Public API
// ──────────────────────────────────────────────────────────

void GPUStableFluids2D::add_impulse(Vector2 p_position, Vector2 p_strength, float p_radius, bool p_add_ink) {
	queue_draw_request(p_position, ink_color, p_strength, p_radius, p_radius * 1.5f);
}

void GPUStableFluids2D::queue_draw_request(Vector2 p_pos, Color p_color, Vector2 p_vel,
                                            float p_color_radius, float p_velocity_radius) {
	if (_draw_request_count >= MAX_DRAW_REQUESTS) return;
	_draw_requests[_draw_request_count++] = {p_pos, p_vel, p_color, p_color_radius, p_velocity_radius, 0};
}

void GPUStableFluids2D::clear_draw_requests() {
	_draw_request_count = 0;
}

void GPUStableFluids2D::reset() {
	clear_draw_requests();
	if (_gpu_resources.device) {
		_gpu_resources.clear_textures(clear_color);
	}
	emit_signal("simulation_reset");
}

Ref<Texture2DRD> GPUStableFluids2D::get_output_texture() const {
	return _output_texture;
}

Ref<Texture2DRD> GPUStableFluids2D::get_velocity_texture() const {
	return _output_velocity_texture;
}

Ref<Texture2DRD> GPUStableFluids2D::get_pressure_texture() const {
	return _output_pressure_texture;
}

Ref<Texture2DRD> GPUStableFluids2D::get_divergence_texture() const {
	return _output_divergence_texture;
}

Vector2 GPUStableFluids2D::world_to_fluid_pos(Vector2 p_world_pos) const {
	Vector2 domain_center(0, 0);
	if (_follow_node) {
		domain_center = _follow_node->get_global_position();
	} else if (follow_mode == FollowMode::Camera2D) {
		Viewport *vp = get_viewport();
		if (vp) {
			Camera2D *cam = vp->get_camera_2d();
			if (cam) domain_center = cam->get_global_position();
		}
	}
	Vector2 local = p_world_pos - domain_center;
	return Vector2(
		(local.x / fluid_world_size.x + 0.5f) * (float)width,
		(local.y / fluid_world_size.y + 0.5f) * (float)height);
}

Vector2 GPUStableFluids2D::sample_velocity(Vector2 p_world_pos) const {
	return Vector2(0, 0);
}

Vector2 GPUStableFluids2D::get_domain_offset() const {
	return _previous_follow_pos; // Return last known position as simple indicator
}

} // namespace godot
