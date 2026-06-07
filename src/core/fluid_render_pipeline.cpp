#include "fluid_render_pipeline.h"
#include "gpu_resource_manager.h"
#include "GPU_stable_fluids_2D_init.h"

#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/variant/typed_array.hpp"

#include <cstring>

namespace godot {

// ──────────────────────────────────────────────────────────
//  Initialisation
// ──────────────────────────────────────────────────────────

void FluidRenderPipeline::initialize(GPUResourceManager *p_gpu, int p_x_groups, int p_y_groups) {
	_gpu     = p_gpu;
	_x_groups = p_x_groups;
	_y_groups = p_y_groups;

	// Pre-allocate all push-constant byte arrays
	auto make_bytes = [](size_t sz, const void *data) {
		PackedByteArray pba;
		pba.resize((int)sz);
		std::memcpy(pba.ptrw(), data, sz);
		return pba;
	};

	// Advection: vec2 resolution(8) + float dt(4) + float rdx(4) = 16 bytes
	_pc_advection.resize(16);
	// Jacobi: vec2 resolution(8) + float alpha(4) + float rbeta(4) = 16 bytes
	_pc_jacobi.resize(16);
	// Divergence: vec2 resolution(8) + float half_rdx(4) = 12 + pad → 16
	_pc_divergence.resize(16);
	// Subtract: same as divergence
	_pc_subtract.resize(16);
	// Boundary: vec2 resolution(8) + float scale(4) = 12 + pad → 16
	_pc_boundary_velocity.resize(16);
	_pc_boundary_pressure.resize(16);
	// Vorticity: vec2 resolution(8) + float dt(4) + float vorticity_scale(4) = 16
	_pc_vorticity.resize(16);
	// Shift texture: vec2 resolution(8) + vec2 offset(8) = 16
	_pc_shift.resize(16);
}

// ──────────────────────────────────────────────────────────
//  Main execute — called once per frame on render thread
// ──────────────────────────────────────────────────────────

void FluidRenderPipeline::execute(float p_dt, GPUStableFluids2D *p_sim) {
	if (!_gpu || !_gpu->device) return;
	if (!_gpu->advect_pipeline.is_valid()) return;

	const float rdx        = 1.0f / p_sim->get_grid_scale();
	const float half_rdx   = rdx * 0.5f;
	const float viscosity  = p_sim->get_viscosity();
	const int   pressure_iter = p_sim->get_poisson_iterations();

	// --- Step 1: Advect velocity ---
	_step_advect_velocity(p_dt, rdx);

	// --- Step 2: Diffuse velocity ---
	if (viscosity > 0.000001f) {
		_step_diffuse_velocity(p_dt, viscosity, 10);
	}

	// --- Step 3: Vorticity confinement ---
	if (p_sim->is_vorticity_enabled()) {
		_step_apply_vorticity(p_dt, p_sim->get_vorticity_scale());
	}

	// --- Step 4: Compute divergence ---
	_step_compute_divergence(half_rdx);

	// --- Step 5: Solve pressure (Jacobi) ---
	_step_solve_pressure(pressure_iter);

	// --- Step 6: Subtract pressure gradient ---
	_step_subtract_pressure_gradient(half_rdx);

	// --- Step 7: Apply boundary conditions ---
	_step_apply_boundary();

	// --- Step 8: Apply obstacle forces ---
	_step_apply_obstacle_force(p_dt, p_sim->get_obstacle_force_strength());

	// --- Step 9: Advect color ---
	_step_advect_color(p_dt, rdx);

	// --- Step 10: Copy obstacle current → previous ---
	_step_copy_obstacle_texture();
}

// ──────────────────────────────────────────────────────────
//  Ping-pong swap helpers
// ──────────────────────────────────────────────────────────

void FluidRenderPipeline::_swap_tex_velocity() {
	std::swap(_gpu->tex_velocity, _gpu->tex_temp);
}
void FluidRenderPipeline::_swap_tex_pressure() {
	std::swap(_gpu->tex_pressure, _gpu->tex_temp);
}
void FluidRenderPipeline::_swap_tex_color() {
	std::swap(_gpu->tex_color, _gpu->tex_temp);
}

// ──────────────────────────────────────────────────────────
//  Low-level dispatch with uniform-set bindings
// ──────────────────────────────────────────────────────────

void FluidRenderPipeline::_dispatch_compute_with_bindings(
		const ComputePipeline &p_pipeline,
		RID p_sampler_tex,      // set=0 sampler2D
		RID p_img0,             // set=1 restrict image2D
		RID p_img1,             // set=2 restrict image2D (may be invalid)
		RID p_img2,             // set=3 restrict image2D (may be invalid)
		const PackedByteArray &p_push_constant)
{
	RenderingDevice *rd = _gpu->device;
	const int64_t cl = rd->compute_list_begin();

	rd->compute_list_bind_compute_pipeline(cl, p_pipeline.pipeline_id);

	// Set 0 — sampler texture
	if (p_sampler_tex.is_valid()) {
		RID us0 = _gpu->get_or_create_cached_uniform_set(
			p_pipeline.shader_id, p_sampler_tex,
			0, RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
		rd->compute_list_bind_uniform_set(cl, us0, 0);
	}

	// Set 1 — first image
	if (p_img0.is_valid()) {
		RID us1 = _gpu->get_or_create_cached_uniform_set(
			p_pipeline.shader_id, p_img0,
			1, RenderingDevice::UNIFORM_TYPE_IMAGE);
		rd->compute_list_bind_uniform_set(cl, us1, 1);
	}

	// Set 2 — second image
	if (p_img1.is_valid()) {
		RID us2 = _gpu->get_or_create_cached_uniform_set(
			p_pipeline.shader_id, p_img1,
			2, RenderingDevice::UNIFORM_TYPE_IMAGE);
		rd->compute_list_bind_uniform_set(cl, us2, 2);
	}

	// Set 3 — third image
	if (p_img2.is_valid()) {
		RID us3 = _gpu->get_or_create_cached_uniform_set(
			p_pipeline.shader_id, p_img2,
			3, RenderingDevice::UNIFORM_TYPE_IMAGE);
		rd->compute_list_bind_uniform_set(cl, us3, 3);
	}

	// Push constants
	rd->compute_list_set_push_constant(cl, p_push_constant, p_push_constant.size());

	rd->compute_list_dispatch(cl, _x_groups, _y_groups, 1);
	rd->compute_list_end();
}

// ──────────────────────────────────────────────────────────
//  Pipeline steps
// ──────────────────────────────────────────────────────────

void FluidRenderPipeline::_step_advect_velocity(float p_dt, float p_rdx) {
	float *data = (float *)_pc_advection.ptrw();
	data[0] = (float)_gpu->width;   // resolution.x
	data[1] = (float)_gpu->height;  // resolution.y
	data[2] = p_dt;
	data[3] = p_rdx;

	// Read velocity via sampler, write to temp
	_dispatch_compute_with_bindings(_gpu->advect_pipeline,
		_gpu->tex_velocity, _gpu->tex_temp, RID(), RID(), _pc_advection);
	_swap_tex_velocity();
}

void FluidRenderPipeline::_step_diffuse_velocity(float p_dt, float p_viscosity, int p_iterations) {
	float dx   = 1.0f;
	float alpha = (dx * dx) / (p_viscosity * p_dt);
	float rbeta = 1.0f / (4.0f + alpha);

	float *data = (float *)_pc_jacobi.ptrw();
	data[0] = (float)_gpu->width;
	data[1] = (float)_gpu->height;
	data[2] = alpha;
	data[3] = rbeta;

	for (int i = 0; i < p_iterations; i++) {
		// Read velocity (previous solution), source=velocity, write to temp
		_dispatch_compute_with_bindings(_gpu->jacobi_pipeline,
			_gpu->tex_velocity, _gpu->tex_velocity, _gpu->tex_temp, RID(), _pc_jacobi);
		_swap_tex_velocity();
	}
}

void FluidRenderPipeline::_step_advect_color(float p_dt, float p_rdx) {
	float *data = (float *)_pc_advection.ptrw();
	data[0] = (float)_gpu->width;
	data[1] = (float)_gpu->height;
	data[2] = p_dt;
	data[3] = p_rdx;

	// Read color via sampler, use velocity as velocity field, write to temp
	_dispatch_compute_with_bindings(_gpu->advect_pipeline,
		_gpu->tex_color, _gpu->tex_velocity, _gpu->tex_temp, RID(), _pc_advection);
	_swap_tex_color();
}

void FluidRenderPipeline::_step_apply_vorticity(float p_dt, float p_vorticity_scale) {
	float *data = (float *)_pc_vorticity.ptrw();
	data[0] = (float)_gpu->width;
	data[1] = (float)_gpu->height;
	data[2] = p_dt;
	data[3] = p_vorticity_scale;

	// Vorticity modifies velocity in-place
	_dispatch_compute_with_bindings(_gpu->vorticity_pipeline,
		_gpu->tex_velocity, _gpu->tex_temp, RID(), RID(), _pc_vorticity);
	_swap_tex_velocity();
}

void FluidRenderPipeline::_step_compute_divergence(float p_half_rdx) {
	float *data = (float *)_pc_divergence.ptrw();
	data[0] = (float)_gpu->width;
	data[1] = (float)_gpu->height;
	data[2] = p_half_rdx;
	data[3] = 0.0f; // pad

	_dispatch_compute_with_bindings(_gpu->divergence_pipeline,
		_gpu->tex_velocity, _gpu->tex_divergence, RID(), RID(), _pc_divergence);
}

void FluidRenderPipeline::_step_solve_pressure(int p_iterations) {
	float alpha = -1.0f;
	float rbeta = 0.25f;

	float *data = (float *)_pc_jacobi.ptrw();
	data[0] = (float)_gpu->width;
	data[1] = (float)_gpu->height;
	data[2] = alpha;
	data[3] = rbeta;

	for (int i = 0; i < p_iterations; i++) {
		// Solve: pressure_new = (sum_neighbors + alpha*divergence) * rbeta
		// Read pressure (x), source=divergence (b), write to temp
		_dispatch_compute_with_bindings(_gpu->jacobi_pipeline,
			_gpu->tex_pressure, _gpu->tex_divergence, _gpu->tex_temp, RID(), _pc_jacobi);
		_swap_tex_pressure();
	}
}

void FluidRenderPipeline::_step_subtract_pressure_gradient(float p_half_rdx) {
	float *data = (float *)_pc_subtract.ptrw();
	data[0] = (float)_gpu->width;
	data[1] = (float)_gpu->height;
	data[2] = p_half_rdx;
	data[3] = 0.0f; // pad

	// velocity_new = velocity - grad(pressure)
	_dispatch_compute_with_bindings(_gpu->subtract_pipeline,
		_gpu->tex_pressure, _gpu->tex_velocity, _gpu->tex_temp, RID(), _pc_subtract);
	_swap_tex_velocity();
}

void FluidRenderPipeline::_step_apply_boundary() {
	// Velocity boundary: scale = -1.0 (no-slip)
	{
		float *data = (float *)_pc_boundary_velocity.ptrw();
		data[0] = (float)_gpu->width;
		data[1] = (float)_gpu->height;
		data[2] = -1.0f;
		data[3] = 0.0f; // pad
		_dispatch_compute_with_bindings(_gpu->boundary_pipeline,
			_gpu->tex_velocity, _gpu->tex_temp, RID(), RID(), _pc_boundary_velocity);
		_swap_tex_velocity();
	}

	// Pressure boundary: scale = 1.0 (Neumann)
	{
		float *data = (float *)_pc_boundary_pressure.ptrw();
		data[0] = (float)_gpu->width;
		data[1] = (float)_gpu->height;
		data[2] = 1.0f;
		data[3] = 0.0f; // pad
		_dispatch_compute_with_bindings(_gpu->boundary_pipeline,
			_gpu->tex_pressure, _gpu->tex_temp, RID(), RID(), _pc_boundary_pressure);
		_swap_tex_pressure();
	}
}

void FluidRenderPipeline::_step_apply_obstacle_force(float p_dt, float p_force_strength) {
	float pc_data[4] = {
		(float)_gpu->width, (float)_gpu->height,
		p_dt, p_force_strength
	};
	PackedByteArray pc;
	pc.resize(16);
	std::memcpy(pc.ptrw(), pc_data, 16);

	RenderingDevice *rd = _gpu->device;
	int64_t cl = rd->compute_list_begin();
	rd->compute_list_bind_compute_pipeline(cl, _gpu->obstacle_force_pipeline.pipeline_id);

	RID us0 = _gpu->get_or_create_cached_uniform_set(
		_gpu->obstacle_force_pipeline.shader_id, _gpu->tex_velocity,
		0, RenderingDevice::UNIFORM_TYPE_IMAGE);
	rd->compute_list_bind_uniform_set(cl, us0, 0);

	RID us1 = _gpu->get_or_create_cached_uniform_set(
		_gpu->obstacle_force_pipeline.shader_id, _gpu->tex_temp,
		1, RenderingDevice::UNIFORM_TYPE_IMAGE);
	rd->compute_list_bind_uniform_set(cl, us1, 1);

	RID us2 = _gpu->get_or_create_cached_uniform_set(
		_gpu->obstacle_force_pipeline.shader_id, _gpu->tex_obstacle,
		2, RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
	rd->compute_list_bind_uniform_set(cl, us2, 2);

	RID us3 = _gpu->get_or_create_cached_uniform_set(
		_gpu->obstacle_force_pipeline.shader_id, _gpu->tex_obstacle_pre,
		3, RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
	rd->compute_list_bind_uniform_set(cl, us3, 3);

	rd->compute_list_set_push_constant(cl, pc, pc.size());
	rd->compute_list_dispatch(cl, _x_groups, _y_groups, 1);
	rd->compute_list_end();

	_swap_tex_velocity();
}

void FluidRenderPipeline::_step_copy_obstacle_texture() {
	float pc_data[2] = { (float)_gpu->width, (float)_gpu->height };
	PackedByteArray pc;
	pc.resize(8);
	std::memcpy(pc.ptrw(), pc_data, 8);

	RenderingDevice *rd = _gpu->device;
	int64_t cl = rd->compute_list_begin();
	rd->compute_list_bind_compute_pipeline(cl, _gpu->copy_texture_pipeline.pipeline_id);

	RID us0 = _gpu->get_or_create_cached_uniform_set(
		_gpu->copy_texture_pipeline.shader_id, _gpu->tex_obstacle,
		0, RenderingDevice::UNIFORM_TYPE_IMAGE);
	rd->compute_list_bind_uniform_set(cl, us0, 0);

	RID us1 = _gpu->get_or_create_cached_uniform_set(
		_gpu->copy_texture_pipeline.shader_id, _gpu->tex_obstacle_pre,
		1, RenderingDevice::UNIFORM_TYPE_IMAGE);
	rd->compute_list_bind_uniform_set(cl, us1, 1);

	rd->compute_list_set_push_constant(cl, pc, pc.size());
	rd->compute_list_dispatch(cl, _x_groups, _y_groups, 1);
	rd->compute_list_end();
}

} // namespace godot
