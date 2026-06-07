#include "fluid_render_pipeline.h"
#include "gpu_resource_manager.h"
#include "GPU_stable_fluids_2D_init.h"
#include "godot_cpp/classes/rendering_device.hpp"
#include <cstring>

namespace godot {

using UT = RenderingDevice::UniformType;
enum { IMG = (int)UT::UNIFORM_TYPE_IMAGE, SMP = (int)UT::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE };

// ── Init ──────────────────────────────────────────────

void FluidRenderPipeline::initialize(GPUResourceManager *p_gpu, int xg, int yg) {
	_gpu = p_gpu; _x_groups = xg; _y_groups = yg;
	_pc_adv.resize(16); _pc_jac.resize(16); _pc_div.resize(16); _pc_sub.resize(16);
	_pc_bv.resize(16); _pc_bp.resize(16); _pc_vor.resize(16); _pc_shift.resize(16);
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
}

// ── Execute ────────────────────────────────────────────

void FluidRenderPipeline::execute(float dt, GPUStableFluids2D *sim) {
	if (!_gpu || !_gpu->device || !_gpu->advect_pipeline.is_valid()) return;
	float rdx = 1.0f / sim->get_grid_scale();
	_step_advect_velocity(dt, rdx);
	if (sim->get_viscosity() > 0.000001f) _step_diffuse_velocity(dt, sim->get_viscosity(), 10);
	if (sim->is_vorticity_enabled()) _step_vorticity(dt, sim->get_vorticity_scale());
	_step_divergence(rdx * 0.5f);
	_step_solve_pressure(sim->get_poisson_iterations());
	_step_subtract_gradient(rdx * 0.5f);
	_step_boundary();
	_step_obstacle_force(dt, sim->get_obstacle_force_strength());
	_step_advect_color(dt, rdx);
	_step_copy_obstacle();
}

// ── Helpers to fill push constants ─────────────────────

static void _fill_pc(PackedByteArray &pba, const void *data, int sz) {
	std::memcpy(pba.ptrw(), data, sz);
}

// ── Steps — each uses correct uniform types for its shader layout ──
// Shader layouts:
//   advect:    set0=sampler2D, set1=image2D, set2=image2D
//   jacobi:    set0=image2D,   set1=image2D, set2=image2D
//   divergence:set0=image2D,   set1=image2D
//   subtract:  set0=image2D,   set1=image2D, set2=image2D
//   boundary:  set0=image2D,   set1=image2D
//   vorticity: set0=image2D,   set1=image2D
//   shift:     set0=sampler2D, set1=image2D
//   obstacle:  set0=image2D,   set1=image2D, set2=sampler2D, set3=sampler2D
//   copy_tex:  set0=image2D,   set1=image2D

void FluidRenderPipeline::_step_advect_velocity(float dt, float rdx) {
	float d[4] = {(float)_gpu->width, (float)_gpu->height, dt, rdx};
	_fill_pc(_pc_adv, d, 16);
	// advect: set0=sampler(field), set1=image(vel_readonly), set2=image(output)
	_dispatch(_gpu->advect_pipeline, _pc_adv,
		_gpu->tex_velocity, SMP, _gpu->tex_velocity, IMG, _gpu->tex_temp, IMG);
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
	// set0=sampler(color), set1=image(velocity as field), set2=image(temp write)
	_dispatch(_gpu->advect_pipeline, _pc_adv,
		_gpu->tex_color, SMP, _gpu->tex_velocity, IMG, _gpu->tex_temp, IMG);
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
