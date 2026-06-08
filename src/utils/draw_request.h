#pragma once

#include "godot_cpp/variant/vector2.hpp"
#include "godot_cpp/variant/color.hpp"

namespace godot {

// CPU-side draw request (queued from emitters / mouse interaction)
struct DrawRequest {
	Vector2 position;
	Vector2 velocity;
	Color   color;
	float   color_radius    = 1.0f;
	float   velocity_radius = 1.0f;
	int     velocity_type   = 0; // 0 = Direct, 1 = Explore
};

// GPU-side batch point layout (48 bytes with std430 padding, must match GLSL struct)
// Packed into a Storage Buffer for single-dispatch batch splat
// std430 struct array stride = ceil(40/16)*16 = 48 bytes (vec4 alignment)
struct BatchPoint {
	float pos[2];         // 8 bytes  — UV-space position [0,1]
	float vel[2];         // 8 bytes  — injected velocity
	float color[4];       // 16 bytes — RGBA color
	float color_radius;   // 4 bytes  — color spread radius
	float vel_radius;     // 4 bytes  — velocity spread radius
	float _pad[2];        // 8 bytes  — std430 array padding to 48 bytes
};

// GPU-side force emitter layout (48 bytes, must match GLSL struct)
struct ForceEmitterData {
	float center[2];       // offset 0  — vec2 center position
	float force[2];        // offset 8  — vec2 force direction/magnitude
	float shape_size[2];   // offset 16 — vec2 shape dimensions
	float force_radius;    // offset 24
	float falloff_exponent;// offset 28
	float swirl_strength;  // offset 32
	int   force_pattern;   // offset 36
	int   emission_shape;  // offset 40
	float _pad;            // offset 44 — padding to 48 bytes
};

} // namespace godot
