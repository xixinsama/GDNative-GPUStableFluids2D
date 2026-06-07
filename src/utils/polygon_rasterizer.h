#pragma once

#include "godot_cpp/variant/vector2.hpp"
#include "godot_cpp/variant/packed_vector2_array.hpp"

#include <cstdint>

namespace godot {

// Software rasterizer for filling polygons into the obstacle byte buffer.
// Each pixel is 16 bytes (4 floats: velocity.x, velocity.y, 0, alpha).
class PolygonRasterizer {
public:
	// Fill a rectangle (axis-aligned).
	static void fill_rect(Vector2 p_world_min, Vector2 p_world_max,
	                      uint8_t *p_data, int p_w, int p_h,
	                      Vector2 p_domain_center, Vector2 p_world_size,
	                      Vector2 p_resolution);

	// Fill a rectangle with velocity encoding.
	static void fill_rect_velocity(Vector2 p_world_min, Vector2 p_world_max,
	                               uint8_t *p_data, int p_w, int p_h,
	                               Vector2 p_domain_center, Vector2 p_world_size,
	                               Vector2 p_resolution,
	                               Vector2 p_linear_vel, float p_angular_vel,
	                               Vector2 p_obj_center);

	// Fill a convex polygon.
	static void fill_polygon(const PackedVector2Array &p_points,
	                         uint8_t *p_data, int p_w, int p_h,
	                         Vector2 p_domain_center, Vector2 p_world_size,
	                         Vector2 p_resolution);

	// Fill a convex polygon with velocity encoding.
	static void fill_polygon_velocity(const PackedVector2Array &p_points,
	                                  uint8_t *p_data, int p_w, int p_h,
	                                  Vector2 p_domain_center, Vector2 p_world_size,
	                                  Vector2 p_resolution,
	                                  Vector2 p_linear_vel, float p_angular_vel,
	                                  Vector2 p_obj_center);

private:
	static constexpr int BYTES_PER_PIXEL = 16; // rgba32f = 16 bytes

	static inline void write_obstacle_pixel(uint8_t *p_data, int p_offset,
	                                        float p_vx, float p_vy);
	static inline void write_obstacle_pixel_static(uint8_t *p_data, int p_offset);
};

} // namespace godot
