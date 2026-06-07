#include "fluid_coord_utils.h"

#include <cmath>

namespace godot {

Vector2 FluidCoordUtils::world_to_pixel(Vector2 p_world_pos, Vector2 p_domain_center,
                                         Vector2 p_world_size, Vector2 p_resolution) {
	Vector2 local = p_world_pos - p_domain_center;
	float u = local.x / p_world_size.x + 0.5f;
	float v = local.y / p_world_size.y + 0.5f;
	return Vector2(u * p_resolution.x, v * p_resolution.y);
}

Vector2 FluidCoordUtils::world_to_uv(Vector2 p_world_pos, Vector2 p_domain_center,
                                      Vector2 p_world_size) {
	Vector2 local = p_world_pos - p_domain_center;
	return Vector2(local.x / p_world_size.x + 0.5f, local.y / p_world_size.y + 0.5f);
}

Vector2 FluidCoordUtils::uv_to_world(Vector2 p_uv, Vector2 p_domain_center,
                                      Vector2 p_world_size) {
	return Vector2(
		(p_uv.x - 0.5f) * p_world_size.x + p_domain_center.x,
		(p_uv.y - 0.5f) * p_world_size.y + p_domain_center.y);
}

float FluidCoordUtils::pixel_to_world_y(int p_py, Vector2 p_resolution,
                                         Vector2 p_world_size, Vector2 p_domain_center) {
	float v = (float)p_py / p_resolution.y;
	return (v - 0.5f) * p_world_size.y + p_domain_center.y;
}

float FluidCoordUtils::pixel_to_world_x(int p_px, Vector2 p_resolution,
                                         Vector2 p_world_size, Vector2 p_domain_center) {
	float u = (float)p_px / p_resolution.x;
	return (u - 0.5f) * p_world_size.x + p_domain_center.x;
}

Vector2 FluidCoordUtils::world_to_pixel_min(Vector2 p_world_min, Vector2 p_domain_center,
                                             Vector2 p_world_size, Vector2 p_resolution) {
	Vector2 px = world_to_pixel(p_world_min, p_domain_center, p_world_size, p_resolution);
	return Vector2(std::floor(px.x), std::floor(px.y));
}

Vector2 FluidCoordUtils::world_to_pixel_max(Vector2 p_world_max, Vector2 p_domain_center,
                                             Vector2 p_world_size, Vector2 p_resolution) {
	Vector2 px = world_to_pixel(p_world_max, p_domain_center, p_world_size, p_resolution);
	return Vector2(std::ceil(px.x), std::ceil(px.y));
}

} // namespace godot
