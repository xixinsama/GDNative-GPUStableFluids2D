#pragma once

#include "godot_cpp/variant/rid.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"

namespace godot {

class GPUResourceManager;
class GPUStableFluids2D;
struct ComputePipeline;

struct AdvectionPushConstant   { float resolution[2]; float dt; float rdx; };
struct JacobiPushConstant      { float resolution[2]; float alpha; float rbeta; };
struct DivergencePushConstant  { float resolution[2]; float half_rdx; };
struct SubtractPushConstant     { float resolution[2]; float half_rdx; };
struct BoundaryPushConstant    { float resolution[2]; float scale; };
struct VorticityPushConstant   { float resolution[2]; float dt; float vorticity_scale; };
struct ShiftTexturePushConstant{ float resolution[2]; float offset[2]; };

class FluidRenderPipeline {
public:
	FluidRenderPipeline() = default;
	void initialize(GPUResourceManager *p_gpu, int p_x_groups, int p_y_groups);
	void execute(float p_dt, GPUStableFluids2D *p_sim);

private:
	// Flexible dispatch: takes list of (texture_rid, set_index, uniform_type) triples
	void _dispatch(const ComputePipeline &pp,
	               const PackedByteArray &pc,
	               RID t0, int u0,  // set 0
	               RID t1 = RID(), int u1 = 0,
	               RID t2 = RID(), int u2 = 0,
	               RID t3 = RID(), int u3 = 0);

	RID _make_us(const ComputePipeline &pp, RID tex, uint32_t set_idx, int utype);

	void _swap_velocity();
	void _swap_pressure();
	void _swap_color();

	void _step_advect_velocity(float dt, float rdx);
	void _step_diffuse_velocity(float dt, float visc, int iter);
	void _step_advect_color(float dt, float rdx);
	void _step_vorticity(float dt, float scale);
	void _step_divergence(float half_rdx);
	void _step_solve_pressure(int iter);
	void _step_subtract_gradient(float half_rdx);
	void _step_boundary();
	void _step_obstacle_force(float dt, float strength);
	void _step_copy_obstacle();

	GPUResourceManager *_gpu = nullptr;
	int _x_groups = 0, _y_groups = 0;

	PackedByteArray _pc_adv, _pc_jac, _pc_div, _pc_sub, _pc_bv, _pc_bp, _pc_vor, _pc_shift;
};

} // namespace godot
