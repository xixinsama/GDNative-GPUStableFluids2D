#pragma once

#include "godot_cpp/variant/vector2.hpp"

namespace godot {

// Coordinate conversion utilities for the fluid simulation
// Maps world-space coordinates ↔ fluid pixel coordinates ↔ UV coordinates
struct FluidCoordUtils {
	// World position → fluid pixel coordinate
	static Vector2 world_to_pixel(Vector2 p_world_pos, Vector2 p_domain_center,
	                              Vector2 p_world_size, Vector2 p_resolution);

	// World position → UV coordinate [0,1] within the fluid domain
	static Vector2 world_to_uv(Vector2 p_world_pos, Vector2 p_domain_center,
	                           Vector2 p_world_size);

	// UV → world position
	static Vector2 uv_to_world(Vector2 p_uv, Vector2 p_domain_center,
	                           Vector2 p_world_size);

	// Pixel Y → world Y
	static float pixel_to_world_y(int p_py, Vector2 p_resolution,
	                              Vector2 p_world_size, Vector2 p_domain_center);

	// Pixel X → world X
	static float pixel_to_world_x(int p_px, Vector2 p_resolution,
	                              Vector2 p_world_size, Vector2 p_domain_center);

	// World bbox min → pixel min (floor)
	static Vector2 world_to_pixel_min(Vector2 p_world_min, Vector2 p_domain_center,
	                                  Vector2 p_world_size, Vector2 p_resolution);

	// World bbox max → pixel max (ceil)
	static Vector2 world_to_pixel_max(Vector2 p_world_max, Vector2 p_domain_center,
	                                  Vector2 p_world_size, Vector2 p_resolution);
};

} // namespace godot
