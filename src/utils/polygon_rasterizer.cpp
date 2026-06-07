#include "polygon_rasterizer.h"
#include "fluid_coord_utils.h"

#include <cmath>
#include <algorithm>
#include <cstring>

namespace godot {

// ──────────────────────────────────────────────────────────
//  Pixel write helpers
// ──────────────────────────────────────────────────────────

inline void PolygonRasterizer::write_obstacle_pixel(uint8_t *p_data, int p_offset,
                                                     float p_vx, float p_vy) {
	float *f = reinterpret_cast<float *>(p_data + p_offset);
	f[0] = p_vx;
	f[1] = p_vy;
	f[2] = 0.0f;
	f[3] = 1.0f;
}

inline void PolygonRasterizer::write_obstacle_pixel_static(uint8_t *p_data, int p_offset) {
	float *f = reinterpret_cast<float *>(p_data + p_offset);
	f[0] = 0.0f;
	f[1] = 0.0f;
	f[2] = 0.0f;
	f[3] = 1.0f;
}

// ──────────────────────────────────────────────────────────
//  Rect fill (static, no velocity)
// ──────────────────────────────────────────────────────────

void PolygonRasterizer::fill_rect(Vector2 p_world_min, Vector2 p_world_max,
                                   uint8_t *p_data, int p_w, int p_h,
                                   Vector2 p_domain_center, Vector2 p_world_size,
                                   Vector2 p_resolution) {
	Vector2 px_min = FluidCoordUtils::world_to_pixel_min(p_world_min, p_domain_center, p_world_size, p_resolution);
	Vector2 px_max = FluidCoordUtils::world_to_pixel_max(p_world_max, p_domain_center, p_world_size, p_resolution);

	int x0 = std::max(0, (int)px_min.x);
	int y0 = std::max(0, (int)px_min.y);
	int x1 = std::min(p_w, (int)px_max.x);
	int y1 = std::min(p_h, (int)px_max.y);

	for (int py = y0; py < y1; py++) {
		int row_base = py * p_w * BYTES_PER_PIXEL;
		for (int px = x0; px < x1; px++) {
			write_obstacle_pixel_static(p_data, row_base + px * BYTES_PER_PIXEL);
		}
	}
}

// ──────────────────────────────────────────────────────────
//  Rect fill (with velocity)
// ──────────────────────────────────────────────────────────

void PolygonRasterizer::fill_rect_velocity(Vector2 p_world_min, Vector2 p_world_max,
                                            uint8_t *p_data, int p_w, int p_h,
                                            Vector2 p_domain_center, Vector2 p_world_size,
                                            Vector2 p_resolution,
                                            Vector2 p_linear_vel, float p_angular_vel,
                                            Vector2 p_obj_center) {
	Vector2 px_min = FluidCoordUtils::world_to_pixel_min(p_world_min, p_domain_center, p_world_size, p_resolution);
	Vector2 px_max = FluidCoordUtils::world_to_pixel_max(p_world_max, p_domain_center, p_world_size, p_resolution);

	int x0 = std::max(0, (int)px_min.x);
	int y0 = std::max(0, (int)px_min.y);
	int x1 = std::min(p_w, (int)px_max.x);
	int y1 = std::min(p_h, (int)px_max.y);

	for (int py = y0; py < y1; py++) {
		int row_base = py * p_w * BYTES_PER_PIXEL;
		float world_y = FluidCoordUtils::pixel_to_world_y(py, p_resolution, p_world_size, p_domain_center);
		for (int px = x0; px < x1; px++) {
			float world_x = FluidCoordUtils::pixel_to_world_x(px, p_resolution, p_world_size, p_domain_center);
			// v = v_linear + ω × r
			float rx = world_x - p_obj_center.x;
			float ry = world_y - p_obj_center.y;
			float vx = p_linear_vel.x - p_angular_vel * ry;
			float vy = p_linear_vel.y + p_angular_vel * rx;
			write_obstacle_pixel(p_data, row_base + px * BYTES_PER_PIXEL, vx, vy);
		}
	}
}

// ──────────────────────────────────────────────────────────
//  Polygon fill — scanline rasterizer for convex polys
// ──────────────────────────────────────────────────────────

void PolygonRasterizer::fill_polygon(const PackedVector2Array &p_points,
                                      uint8_t *p_data, int p_w, int p_h,
                                      Vector2 p_domain_center, Vector2 p_world_size,
                                      Vector2 p_resolution) {
	int n = (int)p_points.size();
	if (n < 3) return;

	// Transform all points to pixel space and find bounding box
	PackedVector2Array px_pts;
	px_pts.resize(n);
	float min_x = 1e9f, max_x = -1e9f, min_y = 1e9f, max_y = -1e9f;

	for (int i = 0; i < n; i++) {
		Vector2 p = FluidCoordUtils::world_to_pixel(p_points[i], p_domain_center, p_world_size, p_resolution);
		px_pts.set(i, p);
		min_x = std::min(min_x, p.x);
		max_x = std::max(max_x, p.x);
		min_y = std::min(min_y, p.y);
		max_y = std::max(max_y, p.y);
	}

	int x0 = std::max(0, (int)std::floor(min_x));
	int y0 = std::max(0, (int)std::floor(min_y));
	int x1 = std::min(p_w, (int)std::ceil(max_x));
	int y1 = std::min(p_h, (int)std::ceil(max_y));

	// Simple even-odd test for each pixel in the bbox
	for (int py = y0; py < y1; py++) {
		int row_base = py * p_w * BYTES_PER_PIXEL;
		for (int px = x0; px < x1; px++) {
			// Point-in-polygon via ray casting (even-odd rule)
			bool inside = false;
			for (int i = 0, j = n - 1; i < n; j = i++) {
				float yi = px_pts[i].y, yj = px_pts[j].y;
				float xi = px_pts[i].x, xj = px_pts[j].x;
				if (((yi > (float)py) != (yj > (float)py)) &&
				    ((float)px < (xj - xi) * ((float)py - yi) / (yj - yi) + xi)) {
					inside = !inside;
				}
			}
			if (inside) {
				write_obstacle_pixel_static(p_data, row_base + px * BYTES_PER_PIXEL);
			}
		}
	}
}

void PolygonRasterizer::fill_polygon_velocity(const PackedVector2Array &p_points,
                                               uint8_t *p_data, int p_w, int p_h,
                                               Vector2 p_domain_center, Vector2 p_world_size,
                                               Vector2 p_resolution,
                                               Vector2 p_linear_vel, float p_angular_vel,
                                               Vector2 p_obj_center) {
	int n = (int)p_points.size();
	if (n < 3) return;

	PackedVector2Array px_pts;
	px_pts.resize(n);
	float min_x = 1e9f, max_x = -1e9f, min_y = 1e9f, max_y = -1e9f;

	for (int i = 0; i < n; i++) {
		Vector2 p = FluidCoordUtils::world_to_pixel(p_points[i], p_domain_center, p_world_size, p_resolution);
		px_pts.set(i, p);
		min_x = std::min(min_x, p.x);
		max_x = std::max(max_x, p.x);
		min_y = std::min(min_y, p.y);
		max_y = std::max(max_y, p.y);
	}

	int x0 = std::max(0, (int)std::floor(min_x));
	int y0 = std::max(0, (int)std::floor(min_y));
	int x1 = std::min(p_w, (int)std::ceil(max_x));
	int y1 = std::min(p_h, (int)std::ceil(max_y));

	for (int py = y0; py < y1; py++) {
		int row_base = py * p_w * BYTES_PER_PIXEL;
		float world_y = FluidCoordUtils::pixel_to_world_y(py, p_resolution, p_world_size, p_domain_center);
		for (int px = x0; px < x1; px++) {
			bool inside = false;
			for (int i = 0, j = n - 1; i < n; j = i++) {
				if (((px_pts[i].y > (float)py) != (px_pts[j].y > (float)py)) &&
				    ((float)px < (px_pts[j].x - px_pts[i].x) * ((float)py - px_pts[i].y) /
				                 (px_pts[j].y - px_pts[i].y) + px_pts[i].x)) {
					inside = !inside;
				}
			}
			if (inside) {
				float world_x = FluidCoordUtils::pixel_to_world_x(px, p_resolution, p_world_size, p_domain_center);
				float rx = world_x - p_obj_center.x;
				float ry = world_y - p_obj_center.y;
				float vx = p_linear_vel.x - p_angular_vel * ry;
				float vy = p_linear_vel.y + p_angular_vel * rx;
				write_obstacle_pixel(p_data, row_base + px * BYTES_PER_PIXEL, vx, vy);
			}
		}
	}
}

} // namespace godot
