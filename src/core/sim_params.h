#pragma once

#include "godot_cpp/variant/vector2.hpp"

namespace godot {

// Simulation parameters uniform buffer (must match GLSL std430 layout exactly)
// Total size: 64 bytes, 16-byte alignment
struct SimParams {
	float resolution[2];        // offset 0,  size 8
	float dt;                   // offset 8,  size 4
	float rdx;                  // offset 12, size 4  — 1.0 / grid_scale
	float viscosity;            // offset 16, size 4
	float diffusion;            // offset 20, size 4
	float color_decay;          // offset 24, size 4
	float velocity_decay;       // offset 28, size 4
	float density_scale;        // offset 32, size 4
	float obstacle_force;       // offset 36, size 4
	float vorticity_scale;      // offset 40, size 4
	int   jacobi_pressure_iter; // offset 44, size 4
	int   jacobi_velocity_iter; // offset 48, size 4
	float subtractive_val;      // offset 52, size 4  — 0=additive, 1=subtractive(CMY)
	float _pad[2];              // offset 56, size 8  — padding to 64 bytes
};

} // namespace godot
