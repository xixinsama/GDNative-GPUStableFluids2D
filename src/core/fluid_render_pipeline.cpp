#include "fluid_render_pipeline.h"
#include "gpu_resource_manager.h"
#include "GPU_stable_fluids_2D_init.h"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include <cstring>

namespace godot {

using UT = RenderingDevice::UniformType;
enum { IMG = (int)UT::UNIFORM_TYPE_IMAGE, SMP = (int)UT::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE };

// Debug logging toggle — set to 1 for debugging, 0 for release
#define FLUID_DEBUG_LOG 1
#if FLUID_DEBUG_LOG
#define FLUID_LOG(...) UtilityFunctions::print(__VA_ARGS__)
#define FLUID_ERR(...) UtilityFunctions::printerr(__VA_ARGS__)
#else
#define FLUID_LOG(...)
#define FLUID_ERR(...)
#endif

// ── Init ──────────────────────────────────────────────

void FluidRenderPipeline::initialize(GPUResourceManager *p_gpu, int xg, int yg) {
	_gpu = p_gpu; _x_groups = xg; _y_groups = yg;
	_pc_adv.resize(16); _pc_jac.resize(16); _pc_div.resize(16); _pc_sub.resize(16);
	_pc_bv.resize(16); _pc_bp.resize(16); _pc_vor.resize(16); _pc_shift.resize(16);
	_pc_splat.resize(16);
}

// ── Swap helpers ────────────────────────────────────────

void FluidRenderPipeline::_swap_velocity() { std::swap(_gpu->tex_velocity, _gpu->tex_temp); }
void FluidRenderPipeline::_swap_pressure() { std::swap(_gpu->tex_pressure, _gpu->tex_temp); }
void FluidRenderPipeline::_swap_color()    { std::swap(_gpu->tex_color, _gpu->tex_temp); }

// ── Uniform set helper (no cache — clean each frame) ──

RID FluidRenderPipeline::_make_us(const ComputePipeline &pp, RID tex, uint32_t set_idx, int utype) {
	Ref<RDUniform> u; u.instantiate();
	u->set_uniform_type((UT)utype);
	u->set_binding(0);
	if (utype == SMP) { u->add_id(_gpu->sampler); u->add_id(tex); }
	else               { u->add_id(tex); }
	return _gpu->device->uniform_set_create(Array::make(u), pp.shader_id, set_idx);
}

// ── Main dispatch ─────────────────────────────────────

void FluidRenderPipeline::_dispatch(const ComputePipeline &pp, const PackedByteArray &pc,
                                     RID t0, int u0, RID t1, int u1, RID t2, int u2, RID t3, int u3) {
	RenderingDevice *rd = _gpu->device;
	if (!rd) {
		FLUID_ERR("[FluidPipeline] _dispatch: null device!");
		return;
	}
	if (!pp.is_valid()) {
		FLUID_ERR("[FluidPipeline] _dispatch: invalid pipeline for '", pp.name, "'");
		return;
	}

	FLUID_LOG("[FluidPipeline] _dispatch: '", pp.name, "' groups=", _x_groups, "x", _y_groups);
	int64_t cl = rd->compute_list_begin();
	rd->compute_list_bind_compute_pipeline(cl, pp.pipeline_id);

	RID us[4] = {};
	if (t0.is_valid()) { us[0] = _make_us(pp, t0, 0, u0); rd->compute_list_bind_uniform_set(cl, us[0], 0); }
	if (t1.is_valid()) { us[1] = _make_us(pp, t1, 1, u1); rd->compute_list_bind_uniform_set(cl, us[1], 1); }
	if (t2.is_valid()) { us[2] = _make_us(pp, t2, 2, u2); rd->compute_list_bind_uniform_set(cl, us[2], 2); }
	if (t3.is_valid()) { us[3] = _make_us(pp, t3, 3, u3); rd->compute_list_bind_uniform_set(cl, us[3], 3); }

	rd->compute_list_set_push_constant(cl, pc, pc.size());
	rd->compute_list_dispatch(cl, _x_groups, _y_groups, 1);
	rd->compute_list_end();

	// Free temp uniform sets
	for (int i = 0; i < 4; i++) if (us[i].is_valid()) rd->free_rid(us[i]);
	FLUID_LOG("[FluidPipeline] _dispatch: '", pp.name, "' DONE");
}

// ── Execute ────────────────────────────────────────────

void FluidRenderPipeline::execute(float dt, GPUStableFluids2D *sim) {
	if (!_gpu || !_gpu->device) {
		FLUID_ERR("[FluidPipeline] execute() aborted: gpu=", _gpu, " device=", (_gpu ? (void*)_gpu->device : nullptr));
		return;
	}
	// Verify all required pipelines are valid before dispatching
	if (!_gpu->advect_pipeline.is_valid() || !_gpu->jacobi_pipeline.is_valid() ||
	    !_gpu->divergence_pipeline.is_valid() || !_gpu->subtract_pipeline.is_valid() ||
	    !_gpu->boundary_pipeline.is_valid()) {
		FLUID_ERR("[FluidPipeline] execute() aborted: one or more critical pipelines invalid");
		return;
	}
	float rdx = 1.0f / sim->get_grid_scale();
	FLUID_LOG("[FluidPipeline] execute() START dt=", dt, " rdx=", rdx);

	// Correct Stam Stable Fluids order:
	// Splat input → obstacle forces → advect velocity → diffuse → vorticity →
	// divergence → pressure → subtract gradient → boundary → advect color
	FLUID_LOG("[FluidPipeline] step 1/11: splat_batch count=", sim->get_draw_request_count());
	_step_splat_batch(sim->get_draw_request_count(), dt);
	FLUID_LOG("[FluidPipeline] step 2/11: obstacle_force strength=", sim->get_obstacle_force_strength());
	_step_obstacle_force(dt, sim->get_obstacle_force_strength());
	FLUID_LOG("[FluidPipeline] step 3/11: advect_velocity");
	_step_advect_velocity(dt, rdx);
	if (sim->get_viscosity() > 0.000001f) {
		FLUID_LOG("[FluidPipeline] step 4/11: diffuse_velocity visc=", sim->get_viscosity());
		_step_diffuse_velocity(dt, sim->get_viscosity(), 10);
	} else { FLUID_LOG("[FluidPipeline] step 4/11: diffuse_velocity SKIP visc=0"); }
	if (sim->is_vorticity_enabled()) {
		FLUID_LOG("[FluidPipeline] step 5/11: vorticity scale=", sim->get_vorticity_scale());
		_step_vorticity(dt, sim->get_vorticity_scale());
	} else { FLUID_LOG("[FluidPipeline] step 5/11: vorticity SKIP"); }
	FLUID_LOG("[FluidPipeline] step 6/11: divergence");
	_step_divergence(rdx * 0.5f);
	FLUID_LOG("[FluidPipeline] step 7/11: solve_pressure iter=", sim->get_poisson_iterations());
	_step_solve_pressure(sim->get_poisson_iterations());
	FLUID_LOG("[FluidPipeline] step 8/11: subtract_gradient");
	_step_subtract_gradient(rdx * 0.5f);
	FLUID_LOG("[FluidPipeline] step 9/11: boundary");
	_step_boundary();
	FLUID_LOG("[FluidPipeline] step 10/11: advect_color");
	_step_advect_color(dt, rdx);
	FLUID_LOG("[FluidPipeline] step 11/11: copy_obstacle");
	_step_copy_obstacle();

	FLUID_LOG("[FluidPipeline] execute() DONE");
}

// ── Helpers to fill push constants ─────────────────────

static void _fill_pc(PackedByteArray &pba, const void *data, int sz) {
	std::memcpy(pba.ptrw(), data, sz);
}

// ── Steps — each uses correct uniform types for its shader layout ──
// Shader layouts:
//   splat_batch: set0=storage_buffer, set1=image2D, set2=image2D
//   advect:      set0=sampler2D, set1=sampler2D, set2=image2D
//   jacobi:      set0=image2D,   set1=image2D, set2=image2D
//   divergence:  set0=image2D,   set1=image2D
//   subtract:    set0=image2D,   set1=image2D, set2=image2D
//   boundary:    set0=image2D,   set1=image2D
//   vorticity:   set0=image2D,   set1=image2D
//   shift:       set0=sampler2D, set1=image2D
//   obstacle:    set0=image2D,   set1=image2D, set2=sampler2D, set3=sampler2D
//   copy_tex:    set0=image2D,   set1=image2D

void FluidRenderPipeline::_step_splat_batch(int point_count, float p_dt) {
	if (point_count <= 0) return;
	RenderingDevice *rd = _gpu->device;
	if (!rd) return;
	if (!_gpu->splat_batch_pipeline.is_valid()) {
		FLUID_ERR("[FluidPipeline] _step_splat_batch: splat_batch pipeline invalid, skipping");
		return;
	}

	// Push constants must match GLSL layout:
	//   vec2 resolution; int point_count; float dt;
	// Using memcpy on a properly aligned struct to avoid int→float bit-pattern mismatch
	struct {
		float resolution[2];
		int point_count;
		float dt;
	} pc_data;
	pc_data.resolution[0] = (float)_gpu->width;
	pc_data.resolution[1] = (float)_gpu->height;
	pc_data.point_count = point_count;
	pc_data.dt = p_dt;

	PackedByteArray pc;
	pc.resize(16);
	std::memcpy(pc.ptrw(), &pc_data, 16);

	int64_t cl = rd->compute_list_begin();
	rd->compute_list_bind_compute_pipeline(cl, _gpu->splat_batch_pipeline.pipeline_id);

	// Set 0: storage buffer (batch_buffer)
	{
		Ref<RDUniform> u; u.instantiate();
		u->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
		u->set_binding(0);
		u->add_id(_gpu->batch_buffer);
		RID us = rd->uniform_set_create(Array::make(u), _gpu->splat_batch_pipeline.shader_id, 0);
		rd->compute_list_bind_uniform_set(cl, us, 0);
		rd->free_rid(us);
	}

	// Set 1: velocity texture (image)
	{
		Ref<RDUniform> u; u.instantiate();
		u->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
		u->set_binding(0);
		u->add_id(_gpu->tex_velocity);
		RID us = rd->uniform_set_create(Array::make(u), _gpu->splat_batch_pipeline.shader_id, 1);
		rd->compute_list_bind_uniform_set(cl, us, 1);
		rd->free_rid(us);
	}

	// Set 2: color texture (image)
	{
		Ref<RDUniform> u; u.instantiate();
		u->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
		u->set_binding(0);
		u->add_id(_gpu->tex_color);
		RID us = rd->uniform_set_create(Array::make(u), _gpu->splat_batch_pipeline.shader_id, 2);
		rd->compute_list_bind_uniform_set(cl, us, 2);
		rd->free_rid(us);
	}

	rd->compute_list_set_push_constant(cl, pc, pc.size());
	uint32_t x_groups = ((uint32_t)point_count + 63) / 64;
	rd->compute_list_dispatch(cl, x_groups, 1, 1);
	rd->compute_list_end();
}

void FluidRenderPipeline::_step_advect_velocity(float dt, float rdx) {
	float d[4] = {(float)_gpu->width, (float)_gpu->height, dt, rdx};
	_fill_pc(_pc_adv, d, 16);
	// advect: set0=sampler(field), set1=sampler(velocity), set2=image(output)
	// Both set0 and set1 use SAMPLER to avoid Vulkan layout conflict when self-advecting
	_dispatch(_gpu->advect_pipeline, _pc_adv,
		_gpu->tex_velocity, SMP, _gpu->tex_velocity, SMP, _gpu->tex_temp, IMG);
	_swap_velocity();
}

void FluidRenderPipeline::_step_diffuse_velocity(float dt, float visc, int iter) {
	float dx = 1.0f, alpha = (dx * dx) / (visc * dt), rbeta = 1.0f / (4.0f + alpha);
	float d[4] = {(float)_gpu->width, (float)_gpu->height, alpha, rbeta};
	_fill_pc(_pc_jac, d, 16);
	for (int i = 0; i < iter; i++) {
		// set0=image(velocity x), set1=image(velocity b), set2=image(temp)
		_dispatch(_gpu->jacobi_pipeline, _pc_jac,
			_gpu->tex_velocity, IMG, _gpu->tex_velocity, IMG, _gpu->tex_temp, IMG);
		_swap_velocity();
	}
}

void FluidRenderPipeline::_step_advect_color(float dt, float rdx) {
	float d[4] = {(float)_gpu->width, (float)_gpu->height, dt, rdx};
	_fill_pc(_pc_adv, d, 16);
	// set0=sampler(color), set1=sampler(velocity), set2=image(temp write)
	_dispatch(_gpu->advect_pipeline, _pc_adv,
		_gpu->tex_color, SMP, _gpu->tex_velocity, SMP, _gpu->tex_temp, IMG);
	_swap_color();
}

void FluidRenderPipeline::_step_vorticity(float dt, float scale) {
	float d[4] = {(float)_gpu->width, (float)_gpu->height, dt, scale};
	_fill_pc(_pc_vor, d, 16);
	_dispatch(_gpu->vorticity_pipeline, _pc_vor,
		_gpu->tex_velocity, IMG, _gpu->tex_temp, IMG);
	_swap_velocity();
}

void FluidRenderPipeline::_step_divergence(float half_rdx) {
	float d[4] = {(float)_gpu->width, (float)_gpu->height, half_rdx, 0};
	_fill_pc(_pc_div, d, 16);
	_dispatch(_gpu->divergence_pipeline, _pc_div,
		_gpu->tex_velocity, IMG, _gpu->tex_divergence, IMG);
}

void FluidRenderPipeline::_step_solve_pressure(int iter) {
	float alpha = -1.0f, rbeta = 0.25f;
	float d[4] = {(float)_gpu->width, (float)_gpu->height, alpha, rbeta};
	_fill_pc(_pc_jac, d, 16);
	for (int i = 0; i < iter; i++) {
		_dispatch(_gpu->jacobi_pipeline, _pc_jac,
			_gpu->tex_pressure, IMG, _gpu->tex_divergence, IMG, _gpu->tex_temp, IMG);
		_swap_pressure();
	}
}

void FluidRenderPipeline::_step_subtract_gradient(float half_rdx) {
	float d[4] = {(float)_gpu->width, (float)_gpu->height, half_rdx, 0};
	_fill_pc(_pc_sub, d, 16);
	_dispatch(_gpu->subtract_pipeline, _pc_sub,
		_gpu->tex_pressure, IMG, _gpu->tex_velocity, IMG, _gpu->tex_temp, IMG);
	_swap_velocity();
}

void FluidRenderPipeline::_step_boundary() {
	// Velocity: scale=-1
	{ float d[4] = {(float)_gpu->width, (float)_gpu->height, -1.0f, 0};
	  _fill_pc(_pc_bv, d, 16);
	  _dispatch(_gpu->boundary_pipeline, _pc_bv,
		_gpu->tex_velocity, IMG, _gpu->tex_temp, IMG);
	  _swap_velocity(); }
	// Pressure: scale=+1
	{ float d[4] = {(float)_gpu->width, (float)_gpu->height, 1.0f, 0};
	  _fill_pc(_pc_bp, d, 16);
	  _dispatch(_gpu->boundary_pipeline, _pc_bp,
		_gpu->tex_pressure, IMG, _gpu->tex_temp, IMG);
	  _swap_pressure(); }
}

void FluidRenderPipeline::_step_obstacle_force(float dt, float strength) {
	float d[4] = {(float)_gpu->width, (float)_gpu->height, dt, strength};
	PackedByteArray pc; pc.resize(16); _fill_pc(pc, d, 16);
	_dispatch(_gpu->obstacle_force_pipeline, pc,
		_gpu->tex_velocity, IMG, _gpu->tex_temp, IMG,
		_gpu->tex_obstacle, SMP, _gpu->tex_obstacle_pre, SMP);
	_swap_velocity();
}

void FluidRenderPipeline::_step_copy_obstacle() {
	float d[4] = {(float)_gpu->width, (float)_gpu->height, 0, 0};
	PackedByteArray pc; pc.resize(16); _fill_pc(pc, d, 16);
	_dispatch(_gpu->copy_texture_pipeline, pc,
		_gpu->tex_obstacle, IMG, _gpu->tex_obstacle_pre, IMG);
}

} // namespace godot
