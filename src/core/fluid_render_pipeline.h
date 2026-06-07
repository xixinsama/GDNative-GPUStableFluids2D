#pragma once

#include "godot_cpp/variant/rid.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"

namespace godot {

// Forward declarations
class GPUResourceManager;
class GPUStableFluids2D;

// Pod constants for push-constant dispatches
struct AdvectionPushConstant {
	float resolution[2];
	float dt;
	float rdx;
};

struct JacobiPushConstant {
	float resolution[2];
	float alpha;
	float rbeta;
};

struct DivergencePushConstant {
	float resolution[2];
	float half_rdx;
};

struct SubtractPushConstant {
	float resolution[2];
	float half_rdx;
};

struct BoundaryPushConstant {
	float resolution[2];
	float scale;
};

struct VorticityPushConstant {
	float resolution[2];
	float dt;
	float vorticity_scale;
};

struct ShiftTexturePushConstant {
	float resolution[2];
	float offset[2];
};

// Orchestrates the full 10-step fluid simulation compute shader sequence.
// All dispatch calls happen on the render thread.
class FluidRenderPipeline {
public:
	FluidRenderPipeline() = default;

	void initialize(GPUResourceManager *p_gpu, int p_x_groups, int p_y_groups);

	// Main entry point — called from GPUStableFluids2D::_process on the render thread
	void execute(float p_dt, GPUStableFluids2D *p_sim);

private:
	// --- Dispatch helpers ---
	void _dispatch_compute(const class ComputePipeline &p_pipeline,
	                       const PackedByteArray &p_push_constant);

	// As above but also binds one sampler texture (set 0) and up to 3 image textures (sets 1-3)
	void _dispatch_compute_with_bindings(const class ComputePipeline &p_pipeline,
	                                     RID p_sampler_tex,
	                                     RID p_img0, RID p_img1, RID p_img2,
	                                     const PackedByteArray &p_push_constant);

	// --- Ping-pong swap helpers ---
	void _swap_tex_velocity();
	void _swap_tex_pressure();
	void _swap_tex_color();

	// --- Pipeline steps ---
	void _step_advect_velocity(float p_dt, float p_rdx);
	void _step_diffuse_velocity(float p_dt, float p_viscosity, int p_iterations);
	void _step_advect_color(float p_dt, float p_rdx);
	void _step_apply_vorticity(float p_dt, float p_vorticity_scale);
	void _step_compute_divergence(float p_half_rdx);
	void _step_solve_pressure(int p_iterations);
	void _step_subtract_pressure_gradient(float p_half_rdx);
	void _step_apply_boundary();

	// --- Resources ---
	GPUResourceManager *_gpu = nullptr;
	int _x_groups = 0;
	int _y_groups = 0;

	// --- Pre-allocated byte arrays for push constants ---
	PackedByteArray _pc_advection;
	PackedByteArray _pc_jacobi;
	PackedByteArray _pc_divergence;
	PackedByteArray _pc_subtract;
	PackedByteArray _pc_boundary_velocity;
	PackedByteArray _pc_boundary_pressure;
	PackedByteArray _pc_vorticity;
	PackedByteArray _pc_shift;
};

} // namespace godot
